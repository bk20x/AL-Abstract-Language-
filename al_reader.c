#include "al_reader.h"
#include <stdlib.h>
#include <assert.h>
#include "al_alloc.h"
#include "al_assert.h"
#define NODE_SIZE         sizeof(AST_Node)
#define NODES_PER_CHUNK   64
#define INITIAL_CHUNKS    32

static AST_Node* parse_expr(Reader* reader, u32 min_prec);
static AST_Node* parse_basic(Reader* reader);
static AST_Node* parse_funcall(Reader*, AST_Node*);
static AST_Node* parse_funcall_param_list(Reader*);
static AST_Node* parse_var_declaration(Reader*);
// static AST_Node* parse_immu_declaration(Reader*);

Reader* init_reader() {
    Reader* result = new(Reader);
    Lexer*  lexer  = init_lexer();
    assert(result && lexer);
    result->interned_symbols = create_symbol_table(32);
    result->lexer = lexer;
    result->ast_pool = create_elastic_fixed_size_pool(NODE_SIZE, NODES_PER_CHUNK, INITIAL_CHUNKS);
    return result;
}

Reader* init_reader_from_file(const cstring filename) {
    Lexer* lexer = init_lexer_from_file(filename);
    if (!lexer) return nullptr;

    Reader* result = new(Reader);
    if (!result) return nullptr;

    *result = (Reader) {
        .lexer            = lexer,
        .interned_symbols = create_symbol_table(32),
        .ast_pool         = create_elastic_fixed_size_pool(NODE_SIZE, NODES_PER_CHUNK, INITIAL_CHUNKS)
    };
    next_token(lexer); // prime
    return result;
}

void prime_reader_from_file(const Reader* reader, const char* filename) {
    prime_lexer_from_file(reader->lexer, filename);
    next_token(reader->lexer);
}


void deinit_reader(Reader* reader) {
    deinit_lexer(reader->lexer);
    st_destroy(reader->interned_symbols);
    destroy_elastic_fixed_size_pool_with_dtor(&reader->ast_pool, (Free_Function)destroy_pooled_ast);
    dealloc(reader);
}



Symbol* get_symbol_if_interned_or_alloc_and_intern_it(const Reader* reader, const String_View* sym_view) {
    const u64 hash = symhash(sym_view->buf, sym_view->len);
    if (!st_contains_hash(reader->interned_symbols, hash)) {
        Symbol* new_symbol = sym_new(sym_view->buf, sym_view->len);
        new_symbol->hash = hash;
        st_put(reader->interned_symbols, new_symbol, new_symbol);
        return new_symbol;
    }
    return st_gethash(reader->interned_symbols, hash);
}

static AST_Node* parse_postfix_chains(Reader* reader, AST_Node* left) {
    while (true) {
        if (reader->lexer->token.kind == tkDot) {
            next_token(reader->lexer); // eat '.'

            assert(reader->lexer->token.kind == tkSymbol && "Expected member identifier after '.'");
            const String_View member_view = reader->lexer->token.view_val;
            Symbol* member_sym = get_symbol_if_interned_or_alloc_and_intern_it(reader, &member_view);
            next_token(reader->lexer); // eat member symbol

            AST_Node* dot_node = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
            *dot_node = (AST_Node) {
                .kind = nkDotAccess,
                .as_dot_access = {
                    .receiver = left,
                    .message  = member_sym
                }
            };
            left = dot_node; // output is new base for next iteration
        }
        // its a funcall (probably)
        else if (reader->lexer->token.kind == tkLPar) {
            left = parse_funcall(reader, left);
        }
        else {
            break;
        }
    }
    return left;
}

static AST_Node* parse_funcall(Reader* reader, AST_Node* callee) {
    assert(reader->lexer->token.kind == tkLPar && "Expected '(' at start of function call parameter block.");
    next_token(reader->lexer); // eat '(' <--- THIS WAS MISSING AND CAUSING THE MISALIGNMENT

    AST_Node* call_node = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    *call_node = (AST_Node) {
        .kind = nkFuncall,
        .as_funcall = {
            .func_name   = callee,
            .func_params = parse_funcall_param_list(reader)
        }
    };
    return call_node;
}

static AST_Node* parse_expr(Reader* reader, const u32 min_prec) {
    AST_Node* left = parse_basic(reader);

    while (reader->lexer->token.kind == tkOperator) {
        const Operator_Kind op_kind = reader->lexer->token.opr_val.kind;
        const u32 prec = Operator_Precedences[op_kind];

        if (prec < min_prec) {
            break;
        }
        next_token(reader->lexer); // eat operator

        const bool is_range = (op_kind == opRange);
        AST_Node* right = parse_expr(reader, is_range ? prec : prec + 1);

        AST_Node* result_node = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);

        if (is_range) {
            *result_node = (AST_Node) {
                .kind = nkRangeExpr,
                .as_range_expr = {
                    .start = left,
                    .end   = right
                }
            };
        } else {
            *result_node = (AST_Node) {
                .kind = nkBinaryExpr,
                .as_binary_expr = {
                    .left  = left,
                    .op    = op_kind,
                    .right = right
                }
            };
        }
        left = result_node;
    }
    return left;
}

static AST_Node* parse_var_declaration(Reader* reader) {
    next_token(reader->lexer); // eat tkVar
    assert(reader->lexer->token.kind == tkSymbol && "Expected symbol after `var`");

    const String_View name_view = reader->lexer->token.view_val;
    next_token(reader->lexer); // eat symbol

    assert(reader->lexer->token.kind == tkOperator && reader->lexer->token.opr_val.kind == opAssign);
    next_token(reader->lexer);

    Symbol*   var_name = get_symbol_if_interned_or_alloc_and_intern_it(reader, &name_view);
    AST_Node* val_node = read(reader);
    AST_Node* result   = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    *result = (AST_Node){
        .kind = nkVarDecl,
        .as_vardecl = {
            .lhs = var_name,
            .rhs = val_node
        }
    };
    return result;
}

/*
static AST_Node* parse_immu_declaration(Reader* reader) {
    next_token(reader->lexer); // eat define
    assert(reader->lexer->token.kind == tkSymbol && "Expected symbol after `define`");

    const String_View name_view = reader->lexer->token.view_val;
    next_token(reader->lexer); // eat symbol

    assert(reader->lexer->token.kind == tkOperator && reader->lexer->token.opr_val.kind == opAssign);
    next_token(reader->lexer);

    Symbol*   var_name = get_symbol_if_interned_or_alloc_and_intern_it(reader, &name_view);
    AST_Node* val_node = read(reader);
    AST_Node* result   = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    *result = (AST_Node){
        .kind = nkDefine,
        .as_define = {
            .lhs = var_name,
            .rhs = val_node
        }
    };
    return result;
}*/


static AST_Node* parse_funcall_param_list(Reader* reader) {
    AST_Node* result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    *result = (AST_Node) {
        .kind          = nkFuncallParamList,
        .as_param_list = nullptr
    };

    if (reader->lexer->token.kind == tkRPar) {
        next_token(reader->lexer);
        return result;
    }

    Vector* const params = vec_new_of_cap(8);
    assert(params);

    while (true) {
        AST_Node* param = parse_expr(reader, 0);
        vec_append(params, param);

        if (reader->lexer->token.kind == tkRPar) {
            next_token(reader->lexer); // eat ')'
            result->as_param_list = params;
            return result;
        }

        if (reader->lexer->token.kind == tkComma) {
            next_token(reader->lexer); // eat ','
            continue;
        }

        if (reader->lexer->token.kind == tkEof) {
            fprintf(stderr, "Fatal Unexpected EOF in argument list\n");
            exit(EXIT_FAILURE);
        }

        const String* kind_str = string_of_token_kind(reader->lexer->token.kind);
        fprintf(stderr, "Unexpected token kind in parse_funcall_param_list: ");
        str_write(kind_str, stderr);
        fprintf(stderr, " (Last token was kind: ");
        str_write(string_of_token_kind(reader->lexer->last_token.kind), stderr);
        fprintf(stderr, ")\n");
        exit(EXIT_FAILURE);
    }
}



static AST_Node* parse_routine_definition(Reader* reader) {
    next_token(reader->lexer); // eat '%'

    assert(reader->lexer->token.kind == tkSymbol && "Expected routine identifier name after '%'");
    const String_View name_view = reader->lexer->token.view_val;
    Symbol* rout_name = get_symbol_if_interned_or_alloc_and_intern_it(reader, &name_view);
    next_token(reader->lexer); // eat routine name sym

    assert(reader->lexer->token.kind == tkLPar && "Expected '(' after routine name");
    next_token(reader->lexer); // eat '('

    Vector* const params = vec_new_of_cap(4);
    assert(params);

    while (reader->lexer->token.kind != tkRPar) {
        assert(reader->lexer->token.kind == tkSymbol && "Routine parameters must be valid symbol names");
        const String_View param_view = reader->lexer->token.view_val;
        Symbol* param_sym = get_symbol_if_interned_or_alloc_and_intern_it(reader, &param_view);
        vec_append(params, param_sym);
        next_token(reader->lexer); // eat param sym

        if (reader->lexer->token.kind == tkComma) {
            next_token(reader->lexer); // eat ','
        }
    }

    assert(reader->lexer->token.kind == tkRPar);
    next_token(reader->lexer); // eat ')'

    assert(reader->lexer->token.kind == tkLBrace && "Expected '{' to start routine body block");

    AST_Node* body_node = read(reader);
    AST_Node* result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    *result = (AST_Node) {
        .kind = nkFuncDef,
        .as_func_def = {
            .rout_name = rout_name,
            .params    = params,
            .body      = body_node
        }
    };
    return result;
}
static AST_Node* parse_if_expression(Reader* reader) {
    next_token(reader->lexer); // eat 'if'

    AST_Node* cond_node = parse_expr(reader, 0);
    assert(cond_node && "Expected valid conditional expression after `if`.");

    // try to read first block
    assert(reader->lexer->token.kind == tkLBrace && "Expected '{' to start if branch block.");
    AST_Node* body_node = read(reader);

    // this is obvious
    AST_Node* else_node = nullptr;
    if (reader->lexer->token.kind == tkElse) {
        next_token(reader->lexer); // eat 'else'
        if (reader->lexer->token.kind == tkIf) {
            else_node = parse_if_expression(reader); // for `else if`
        } else {
            assert(reader->lexer->token.kind == tkLBrace && "Expected '{' to start else branch block.");
            else_node = read(reader);
        }
    }

    AST_Node* result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    *result = (AST_Node) {
        .kind = nkIfExpr,
        .as_if_expression = {
            .condition  = cond_node,
            .body       = body_node,
            .maybe_else = else_node
        }
    };
    return result;
}

static AST_Node* parse_for_loop(Reader* reader) {
    next_token(reader->lexer); // eat 'for'

    // get the name of the itervar, later i wanna have multiple values like `for idx, x in xs {...}`
    assert(reader->lexer->token.kind == tkSymbol && "Expected loop iterator variable name after `for`.");
    const String_View var_view = reader->lexer->token.view_val;
    Symbol* iterator_sym = get_symbol_if_interned_or_alloc_and_intern_it(reader, &var_view);
    next_token(reader->lexer); // eat itervar sym

    assert(reader->lexer->token.kind == tkOperator && reader->lexer->token.opr_val.kind == opIn &&
        "Expected `in` after loop variable.");

    next_token(reader->lexer); // eat 'in'
    AST_Node* iterable_expr = parse_expr(reader, 0);

    assert(iterable_expr && "Expected valid iterable expression or range after `in`.");
    assert(reader->lexer->token.kind == tkLBrace && "Expected '{' to start for loop body block.");

    AST_Node* body_node = read(reader);
    AST_Node* result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    *result = (AST_Node) {
        .kind = nkForLoop,
        .as_for_loop = {
            .iterator = iterator_sym,
            .iterable = iterable_expr,
            .body     = body_node
        }
    };
    return result;
}

static AST_Node* parse_using_expression(Reader* reader) {
    next_token(reader->lexer); // eat `using`
    AST_Node* the_thing_we_are_using = read(reader);
    AST_Node* body = read(reader);
    // assert ... blah. lazy rn
    AST_Node* result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    result->kind = nkUsingExpr;
    result->as_using_expr.env  = the_thing_we_are_using;
    result->as_using_expr.body = body;
    return result;
}


static AST_Node* parse_basic(Reader* reader) {
    AST_Node* result = nullptr;

    switch (reader->lexer->token.kind) {
        case tkVar: {
            return parse_var_declaration(reader);
        }
       /* case tkDefine: {
            return parse_immu_declaration(reader);
        }*/
        case tkRout: {
            return parse_routine_definition(reader);
        }
        case tkIf: {
            return parse_if_expression(reader);
        }
        case tkFor: {
            return parse_for_loop(reader);
        }
        case tkUsing: {
            return parse_using_expression(reader);
        }
        case tkSymbol: {
            const String_View sym_view = reader->lexer->token.view_val;
            next_token(reader->lexer); // eat symbol

            result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
            *result = (AST_Node) {
                .kind      = nkSymLit,
                .as_symbol = get_symbol_if_interned_or_alloc_and_intern_it(reader, &sym_view)
            };
            break;
        }
        case tkInt: {
            result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
            result->kind = nkIntLit;
            result->as_int_lit = reader->lexer->token.int_val;
            next_token(reader->lexer);
            break;
        }
        case tkFloat: {
            result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
            result->kind = nkFloatLit;
            result->as_float_lit = reader->lexer->token.flt_val;
            next_token(reader->lexer);
            break;
        }
        case tkBoolLit: {
            result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
            result->kind = nkBoolLit;
            result->as_bool_lit = reader->lexer->token.bool_lit_val;
            next_token(reader->lexer);
            break;
        }
        case tkString: {
            result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
            result->kind = nkStrLit;

            const char* src_buf = reader->lexer->token.view_val.buf;
            const size_t src_len = reader->lexer->token.view_val.len;
            String* result_strlit = str_of_cap(src_len);

            for (size_t i = 0; i < src_len; i++) {
                if (src_buf[i] == '\\' && (i + 1) < src_len) {
                    i++;
                    char escaped_char;
                    switch (src_buf[i]) {
                        case 'n':  escaped_char = '\n'; break;
                        case 'r':  escaped_char = '\r'; break;
                        case 't':  escaped_char = '\t'; break;
                        case '\\': escaped_char = '\\'; break;
                        case '"':  escaped_char = '"';  break;
                        default:
                            str_append_bytes_unsafe(result_strlit, "\\", 1);
                            escaped_char = src_buf[i];
                            break;
                    }
                    str_append_bytes_unsafe(result_strlit, &escaped_char, 1);
                } else {
                    str_append_bytes_unsafe(result_strlit, &src_buf[i], 1);
                }
            }
            result->as_str_lit = result_strlit;
            next_token(reader->lexer);
            break;
        }
        case tkChar: {
            result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
            result->kind = nkCharLit;
            result->as_char_lit = reader->lexer->token.chr_val;
            next_token(reader->lexer);
            break;
        }
        case tkLBrace: {
            next_token(reader->lexer); // eat '{'
            Vector* stmts = vec_new_of_cap0(32);
            while (reader->lexer->token.kind != tkRBrace && reader->lexer->token.kind != tkEof) {
                AST_Node* node = read(reader);
                vec_append_unsafe(stmts, node);
            }
            assert(reader->lexer->token.kind == tkRBrace); // eat '}'
            next_token(reader->lexer);

            result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
            result->kind = nkBlockLit;
            result->as_block_lit = stmts;
            break;
        }
        default: {
            printf("Error: parse_basic() encountered unhandled token kind: ");
            str_print(string_of_token_kind(reader->lexer->token.kind));
            die("Not done in parse_basic");
        }
    }

    if (result != nullptr) {
        return parse_postfix_chains(reader, result);
    }
    return nullptr;
}



AST_Node* read(Reader* reader) {
    if (reader->lexer->token.kind == tkRout) {
        return parse_routine_definition(reader);
    }
    return parse_expr(reader, 0);
}