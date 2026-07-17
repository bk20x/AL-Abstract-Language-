#include "al_reader.h"

#include <assert.h>
#include "al_alloc.h"
#include "al_assert.h"
#define NODE_SIZE         sizeof(AST_Node)
#define NODES_PER_CHUNK   64
#define INITIAL_CHUNKS    32


Reader* init_reader() {
    Reader* result = new(Reader);
    Lexer*  lexer  = init_lexer();
    assert(result && lexer);
    result->interned_symbols = create_symbol_table(16, symhash);
    result->lexer = lexer;
    result->ast_pool = create_elastic_fixed_size_pool(NODE_SIZE, NODES_PER_CHUNK, INITIAL_CHUNKS);
    return result;
}

Reader* init_reader_from_file(const char* restrict filename) {
    Lexer* lexer = init_lexer_from_file(filename);
    if (!lexer) return nullptr;

    Reader* result = new(Reader);
    if (!result) return nullptr;

    *result = (Reader) {
        .lexer            = lexer,
        .interned_symbols = create_symbol_table(16, symhash),
        .ast_pool         = create_elastic_fixed_size_pool(NODE_SIZE, NODES_PER_CHUNK, INITIAL_CHUNKS)
    };
    return result;
}

void deinit_reader(Reader* reader) {
    deinit_lexer(reader->lexer);
    st_destroy(reader->interned_symbols);
    destroy_elastic_fixed_size_pool(&reader->ast_pool);
    dealloc(reader);
}



Symbol* sym_get_if_interned_or_alloc_and_intern_it(const Reader* reader, const String_View* sym_view) {
    const u64 hash = symhash(sym_view->buf, sym_view->len);
    if (!st_contains_hash(reader->interned_symbols, hash)) {
        Symbol* new_symbol = sym_new(sym_view->buf, sym_view->len);
        st_put(reader->interned_symbols, new_symbol, new_symbol);
        return new_symbol;
    }
    return st_gethash(reader->interned_symbols, hash);
}

AST_Node* parse_var_declaration(Reader* reader) {
    next_token(reader->lexer); // eat tkVar
    assert(reader->lexer->token.kind == tkSymbol && "Expected symbol after `var`");

    const String_View name_view = reader->lexer->token.view_val;
    next_token(reader->lexer); // eat symbol

    assert(reader->lexer->token.kind == tkOperator && reader->lexer->token.opr_val.kind == opAssign);
    next_token(reader->lexer);

    Symbol*   var_name = sym_get_if_interned_or_alloc_and_intern_it(reader, &name_view);
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

AST_Node* parse_funcall_param_list(Reader* reader) {
    AST_Node* result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    *result = (AST_Node) {
        .kind          = nkParamList,
        .as_param_list = nullptr
    };

    if (reader->lexer->token.kind == tkRPar) {
        next_token(reader->lexer);
        return result;
    }

    Vector* const params = vec_new_of_cap(8);
    assert(params);

    while (true) {
        AST_Node* param = read(reader);
        vec_append(params, param);

        if (reader->lexer->token.kind == tkRPar) {
            next_token(reader->lexer);
            if (result->as_param_list == nullptr) {
                result->as_param_list = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
            }
            result->as_param_list->params = params;
            return result;
        }

        if (reader->lexer->token.kind == tkComma) {
            next_token(reader->lexer);
            continue;
        }

        if (reader->lexer->token.kind == tkEof) {
            fprintf(stderr, "Fatal Unexpected EOF in argument list\n");
            exit(EXIT_FAILURE);
        }

        const String* kind_str = string_of_token_kind(reader->lexer->token.kind);
        fprintf(stderr, "Unexpected token kind in parse_funcall_param_list:\n");
        str_write(kind_str, stderr);
        exit(EXIT_FAILURE);
    }
}


AST_Node* parse_funcall(Reader* reader) {
    Symbol*   func_sym  = sym_get_if_interned_or_alloc_and_intern_it(reader, &reader->lexer->last_token.view_val);
    AST_Node* func_name = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    *func_name = (AST_Node) {
        .kind      = nkSymLit,
        .as_symbol = func_sym
    };

    assert_token_is_kind(reader->lexer, tkLPar);
    next_token(reader->lexer);

    AST_Node* func_params = parse_funcall_param_list(reader);
    AST_Node* result = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
    *result = (AST_Node) {
        .kind = nkFuncall,
        .as_funcall = {
            .func_name   = func_name,
            .func_params = func_params
        }
    };
    return result;
}


AST_Node* read(Reader* reader) {
    while (reader->lexer->token.kind != tkEof) {
        switch (reader->lexer->token.kind) {
            case tkVar: {
                return parse_var_declaration(reader);
            }
            case tkSymbol: {
                const String_View sym_view = reader->lexer->token.view_val;

                next_token(reader->lexer);
                if (reader->lexer->token.kind == tkLPar)
                    return parse_funcall(reader);

                AST_Node* sym_node = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
                *sym_node = (AST_Node) {
                    .kind      = nkSymLit,
                    .as_symbol = sym_new(sym_view.buf, sym_view.len)
                };
                next_token(reader->lexer);
                return sym_node;
            }
            case tkInt: {
                AST_Node* int_lit = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
                int_lit->kind = nkIntLit;
                int_lit->as_int_lit = reader->lexer->token.int_val;
                next_token(reader->lexer);
                return int_lit;
            }
            case tkFloat: {
                AST_Node* float_lit = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
                float_lit->kind = nkFloatLit;
                float_lit->as_float_lit = reader->lexer->token.flt_val;
                next_token(reader->lexer);
                return float_lit;
            }
            case tkString: {
                AST_Node* strlit_node = alloc_from_elastic_fixed_size_pool(&reader->ast_pool);
                strlit_node->kind = nkStrLit;

                String* str = str_byteslice(reader->lexer->token.view_val.buf, 0, reader->lexer->token.view_val.len);
                strlit_node->as_strlit = str;

                next_token(reader->lexer);
                return strlit_node;
            }
            default: {
                printf("Error: read() encountered unhandled token kind: ");
                str_print(string_of_token_kind(reader->lexer->token.kind));
                die("Not done in read");
            }
        }
    }
    return nullptr;
}
