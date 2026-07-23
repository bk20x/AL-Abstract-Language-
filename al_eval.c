#include "al_eval.h"

#include <assert.h>
#include "al_alloc.h"
#include "al_assert.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "al_object.h"
#include "al_environment.h"
#include "al_ast.h"
#include "al_symbol.h"

Eval_Runtime eval_init() {
    Environment* toplevel = env_new(nullptr);
    assert(toplevel);
    return (Eval_Runtime){
        .toplevel = toplevel,
        .reader   = init_reader()
    };
}

Al_Object* eval_ast_in(Eval_Runtime* eval, Environment* scope, AST_Node* ast) {
    switch (ast->kind) {
        case nkIntLit: {
            Al_Object* result = new(Al_Object);
            result->kind      = okInt;
            result->ref_count = 1;
            result->as_int    = ast->as_int_lit;
            return result;
        }
        case nkFloatLit: {
            Al_Object* result = new(Al_Object);
            result->kind      = okFloat;
            result->ref_count = 1;
            result->as_float  = ast->as_float_lit;
            return result;
        }
        case nkCharLit: {
            Al_Object* result = new(Al_Object);
            result->kind      = okChar;
            result->ref_count = 1;
            result->as_char   = ast->as_char_lit;
            return result;
        }
        case nkBoolLit: {
            Al_Object* result = new(Al_Object);
            result->kind      = okBool;
            result->ref_count = 1;
            result->as_bool   = ast->as_bool_lit;
            return result;
        }
        case nkStrLit: {
            Al_Object* result = new(Al_Object);
            result->kind      = okString;
            result->ref_count = 1;
            result->as_string = str_byteslice(ast->as_str_lit->chars, 0, ast->as_str_lit->len);
            return result;
        }
        case nkSymLit: {
            Al_Object* value_of_the_symbol = env_lookup_symbol(scope, ast->as_symbol);
            if (!value_of_the_symbol) {
                fprintf(stderr, "Runtime Error: Undefined identifier '%.*s'\n", (int)ast->as_symbol->len, ast->as_symbol->name);
                exit(EXIT_FAILURE);
            }
            return obj_retain(value_of_the_symbol); // +1 ref
        }
        case nkVarDecl: {
            #define vardecl ast->as_vardecl
            if (ot_get(scope->locals, vardecl.lhs)) {
                fprintf(stderr, "Variable %.*s is already defined\n", (int)vardecl.lhs->len, vardecl.lhs->name);
                exit(EXIT_FAILURE);
            }

            Al_Object* var_val = eval_ast_in(eval, scope, vardecl.rhs);
            ot_put(scope->locals, vardecl.lhs, var_val);

            return (Al_Object*)&TRUE;
        }
        case nkFuncDef: {
            #define routdef ast->as_func_def
            Al_Object* rout_obj = new(Al_Object);
            rout_obj->kind      = okFunction;
            rout_obj->ref_count = 1;

            rout_obj->as_function = new(Al_Routine);
            rout_obj->as_function->scope = env_new(scope);
            rout_obj->as_function->routine_ast = ast;

            ot_put(scope->locals, routdef.rout_name, rout_obj);

            return (Al_Object*)&TRUE;
        }
        case nkBlockLit: {
            Environment* block_scope = env_new(scope);
            Al_Object* block_result = (Al_Object*)&TRUE;

            size_t expr_index = 0;
            vforeach(AST_Node, ast->as_block_lit, stmt) {
                if (!stmt) { expr_index++; continue; }

                Al_Object* last_result = eval_ast_in(eval, block_scope, stmt);

                if (expr_index + 1 == ast->as_block_lit->len) {
                    block_result = last_result;
                } else {
                    obj_release(last_result);
                }
                expr_index++;
            }

            // for nested functions, if they arent returned, we release them.
            for (size_t idx = 0; idx < block_scope->locals->cap; idx++) {
                Al_Object* val = block_scope->locals->entries[idx].value;
                if (val && val->kind == okFunction && val != block_result) {
                    block_scope->locals->entries[idx].value = nullptr;
                    obj_release(val);
                }
            }

            // if the block_result is a closure about to return, delete it from locals
            // transfer ownership out to the caller,
            // this protects upvalues from being destroyed too early
            if (block_result->kind == okFunction) {
                for (size_t idx = 0; idx < block_scope->locals->cap; idx++) {
                    if (block_scope->locals->entries[idx].value == block_result) {
                        block_scope->locals->entries[idx].value = nullptr;
                    }
                }
                const Environment* closure_scope = block_result->as_function->scope;
                if (closure_scope->parent == block_scope) {
                    if (block_result->ref_count > 1) {
                        block_result->ref_count--;
                    }
                }
            }
            env_release(block_scope);
            return block_result;
        }
        case nkFuncall: {
            #define funcall ast->as_funcall
            Al_Object* func_obj = eval_ast_in(eval, scope, funcall.func_name);

            // builtins (C)
            if (func_obj->kind == okBuiltin) {
                Al_Object* builtin_res = func_obj->as_builtin.builtin(eval, scope, funcall.func_params);
                obj_release(func_obj);
                return builtin_res;
            }

            // user defined routines, and lambdas when i add them
            if (func_obj->kind != okFunction) {
                die("Runtime Error: Attempted to invoke a non-callable object.");
            }

            const AST_Node* definition_ast = func_obj->as_function->routine_ast;
            const Vector* param_names = definition_ast->as_func_def.params;
            const Vector* args_exprs = funcall.func_params->as_param_list;
            const size_t provided_args = args_exprs ? args_exprs->len : 0;

            if (provided_args != param_names->len) {
                die("Runtime Error: Arity mismatch during routine invocation.");
            }

            // new frame
            Environment* call_scope = env_new(func_obj->as_function->scope);
            for (size_t i = 0; i < provided_args; i++) {
                AST_Node* arg_expr = args_exprs->data[i];
                Symbol* param_name = param_names->data[i];

                Al_Object* evaluated_val = eval_ast_in(eval, scope, arg_expr);
                ot_put(call_scope->locals, param_name, evaluated_val);
            }


            // We could just evaluate the body directly, but that creates ANOTHER scope. blocks are their own thing
            // some duplicate code but this is fine
            Al_Object* call_result = (Al_Object*)&TRUE;
            const AST_Node* body_node = definition_ast->as_func_def.body;

            size_t expr_index = 0;
            vforeach(AST_Node, body_node->as_block_lit, stmt) {
                if (!stmt) {
                    expr_index++;
                    continue;
                }
                Al_Object* current_res = eval_ast_in(eval, call_scope, stmt);

                if (expr_index + 1 == body_node->as_block_lit->len) {
                    call_result = current_res;
                } else  {
                    obj_release(current_res);
                }
                expr_index++;
            }

            for (size_t idx = 0; idx < call_scope->locals->cap; idx++) {
                Al_Object* val = call_scope->locals->entries[idx].value;
                if (val && val->kind == okFunction && val != call_result) {
                    call_scope->locals->entries[idx].value = nullptr;
                    obj_release(val);
                }
            }

            // if the call_result is a closure are returning,
            // have to disconnect its tracking entry cell slot from call_scope->locals right here.
            // This leaves unreturned child functions intact
            bool is_returning_closure = false;
            if (call_result->kind == okFunction) {
                is_returning_closure = true;
                for (size_t idx = 0; idx < call_scope->locals->cap; idx++) {
                    if (call_scope->locals->entries[idx].value == call_result) {
                        call_scope->locals->entries[idx].value = nullptr;
                    }
                }
            }

            env_release(call_scope);
            obj_release(func_obj);

            // decrement ref on new local closures
            if (is_returning_closure) {
                const Environment* closure_env = call_result->as_function->scope;
                if (closure_env && closure_env->parent == call_scope) {
                    if (call_result->ref_count > 1) {
                        call_result->ref_count--;
                    }
                }
            }
            return call_result;
        }
        case nkBinaryExpr: {
            const Binary_Expr_Node binop = ast->as_binary_expr;
            if (binop.op == opAssign) {
                assert(binop.left->kind == nkSymLit && "Runtime Error: L-value of an assignment statement must be an identifier.");

                Environment* target_scope = scope;
                while (target_scope != nullptr) {
                    if (ot_get(target_scope->locals, binop.left->as_symbol)) {
                        break;
                    }
                    target_scope = target_scope->parent;
                }

                if (!target_scope) {
                    fprintf(stderr, "Assignment to undeclared variable %.*s\n",
                        (int)binop.left->as_symbol->len, binop.left->as_symbol->name);
                    exit(EXIT_FAILURE); // temp
                }

                Al_Object* right_val = eval_ast_in(eval, scope, binop.right);
                ot_put(target_scope->locals, binop.left->as_symbol, right_val);

                return (Al_Object*)&TRUE;
            }

            Al_Object* left  = eval_ast_in(eval, scope, binop.left);
            Al_Object* right = eval_ast_in(eval, scope, binop.right);

            if (binop.op == opIn) {
                Al_Object* result = nullptr;
                assert(right->kind == okRange || right->kind == okVector || right->kind == okString &&
                    "Runtime Error: Right-hand operand of 'in' must be (Range, Vector, String)");

                if (right->kind == okRange) {
                    if (right->as_range.kind == rkInt) {
                        assert(left->kind == okInt && "Runtime Error: Type mismatch. Left-hand operand must be an Int for an Integer range");
                    } else {
                        assert(left->kind == okChar && "Runtime Error: Type mismatch. Left-hand operand must be a Char for a Character range");
                    }

                    s64 element = 0;
                    s64 start   = 0;
                    s64 end     = 0;

                    if (right->as_range.kind == rkInt) {
                        element = left->as_int;
                        start   = right->as_range.as_int_range.low;
                        end     = right->as_range.as_int_range.high;
                    } else {
                        element = (s64)left->as_char;
                        start   = (s64)right->as_range.as_char_range.low;
                        end     = (s64)right->as_range.as_char_range.high;
                    }
                    bool is_member = false;
                    if (start <= end) {
                        is_member = (element >= start && element <= end);
                    } else {
                        is_member = (element >= end && element <= start);
                    }
                    result = is_member ? (Al_Object*)&TRUE : (Al_Object*)&FALSE;
                } else if (right->kind == okString) {
                    assert(left->kind == okChar && "Its not a char silly, there's no way it could be in a String");
                    for (size_t i = 0; i < right->as_string->len; i++) {
                        if (left->as_char == right->as_string->chars[i]) {
                            result = (Al_Object*)&TRUE;
                            break;
                        }
                    }
                } else {
                    vforeach(Al_Object, right->as_vector, obj) {
                        if (left == obj) { // just for now this just works if they are the same object in memory, need to impl compare_object
                            result = (Al_Object*)&TRUE;
                            break;
                        }
                    }
                }
                if (result == nullptr) result = (Al_Object*)&FALSE;
                obj_release(right);
                obj_release(left);
                return result;
            }

            Al_Object* op_result = (Al_Object*)&TRUE;
            if ((left->kind  == okInt || left->kind  == okFloat) &&
                (right->kind == okInt || right->kind == okFloat)) {
                const float lf = (left->kind == okFloat)  ? left->as_float  : (float)left->as_int;
                const float rf = (right->kind == okFloat) ? right->as_float : (float)right->as_int;
                const s64 li   = (left->kind == okInt)    ? left->as_int    : (s64)left->as_float;
                const s64 ri   = (right->kind == okInt)   ? right->as_int   : (s64)right->as_float;

                const bool is_float_mode = (left->kind == okFloat || right->kind == okFloat);

                switch (binop.op) {
                    case opEq:      op_result = (is_float_mode ? (lf == rf) : (li == ri)) ? (Al_Object*)&TRUE  : (Al_Object*)&FALSE; break;
                    case opGThan:   op_result = (is_float_mode ? (lf > rf)  : (li > ri)) ? (Al_Object*)&TRUE   : (Al_Object*)&FALSE; break;
                    case opGThanEq: op_result = (is_float_mode ? (lf >= rf) : (li >= ri)) ? (Al_Object*)&TRUE  : (Al_Object*)&FALSE; break;
                    case opLThan:   op_result = (is_float_mode ? (lf < rf)  : (li < ri)) ? (Al_Object*)&TRUE   : (Al_Object*)&FALSE; break;
                    case opLThanEq: op_result = (is_float_mode ? (lf <= rf) : (li <= ri)) ? (Al_Object*)&TRUE  : (Al_Object*)&FALSE; break;
                    default: {
                        Al_Object* result = new(Al_Object);
                        result->ref_count = 1;

                        if (is_float_mode) {
                            result->kind = okFloat;
                            switch (binop.op) {
                                case opAdd: result->as_float = lf + rf; break;
                                case opSub: result->as_float = lf - rf; break;
                                case opMul: result->as_float = lf * rf; break;
                                case opDiv: assert(rf != 0.0f); result->as_float = lf / rf; break;
                                default: die("Unimplemented operator in float binop evaluation");
                            }
                        } else {
                            result->kind = okInt;
                            switch (binop.op) {
                                case opAdd: result->as_int = li + ri; break;
                                case opSub: result->as_int = li - ri; break;
                                case opMul: result->as_int = li * ri; break;
                                case opDiv: assert(ri != 0); result->as_int = li / ri; break;
                                default: die("Unimplemented operator in integer binop evaluation");
                            }
                        }
                        op_result = result;
                        break;
                    }
                }
            } else {
                switch (binop.op) {
                    case opEq: {
                        if (left->kind != right->kind) {
                            op_result = (Al_Object*)&FALSE;
                            break;
                        }
                        switch (left->kind) {
                            case okVector:
                                op_result = left->as_vector == right->as_vector ? (Al_Object*)&TRUE : (Al_Object*)&FALSE;
                                break;
                            case okFunction:
                                op_result = left->as_function->routine_ast == right->as_function->routine_ast ? (Al_Object*)&TRUE : (Al_Object*)&FALSE;
                                break;
                            case okBuiltin:
                                op_result = left->as_builtin.builtin == right->as_builtin.builtin ? (Al_Object*)&TRUE : (Al_Object*)&FALSE;
                                break;
                            case okString:
                                if (left == right) {
                                    op_result = (Al_Object*)&TRUE;
                                }
                                else if (left->as_string->len != right->as_string->len) {
                                    op_result = (Al_Object*)&FALSE;
                                }
                                else if (memcmp(left->as_string->chars, right->as_string->chars, left->as_string->len) == 0) {
                                    op_result = (Al_Object*)&TRUE;
                                } else {
                                    op_result = (Al_Object*)&FALSE;
                                }
                                break;
                            case okBool:
                                op_result = left->as_bool == right->as_bool ? (Al_Object*)&TRUE : (Al_Object*)&FALSE;
                                break;
                            case okChar:
                                op_result = left->as_char == right->as_char ? (Al_Object*)&TRUE : (Al_Object*)&FALSE;
                                break;
                            default: die("Unimplemented comparison");
                        }
                        break;
                    }
                    default: die("havent implemented any other operators besides `==` for non numeric types");
                }
            }
            obj_release(right);
            obj_release(left);
            return op_result;
        }

        case nkIfExpr: {
            #define ifexpr ast->as_if_expression
            Al_Object* cond = eval_ast_in(eval, scope, ifexpr.condition);
            const bool is_true = cond != (Al_Object*)&FALSE;
            obj_release(cond);
            if (is_true) {
                return eval_ast_in(eval, scope, ifexpr.body);
            }
            if (ifexpr.maybe_else != nullptr) {
                return eval_ast_in(eval, scope, ifexpr.maybe_else);
            }
            #undef ifexpr
            return (Al_Object*)&TRUE;
        }
        case nkForLoop: {
            #define for_loop ast->as_for_loop
            Al_Object*   iterable   = eval_ast_in(eval, scope, for_loop.iterable);
            Environment* loop_scope = env_new(scope);
            assert(iterable->kind == okRange || iterable->kind == okVector || iterable->kind == okString &&
                   "Runtime Error: Target operand of a for loop must be a range or vector.");

            if (iterable->kind == okRange) {
                s64 low  = 0;
                s64 high = 0;
                const Al_Range_Kind range_kind = iterable->as_range.kind;

                if (range_kind == rkInt) {
                    low  = iterable->as_range.as_int_range.low;
                    high = iterable->as_range.as_int_range.high;
                } else {
                    low  = (s64)iterable->as_range.as_char_range.low;
                    high = (s64)iterable->as_range.as_char_range.high;
                }

                const bool is_incremental = (low <= high);
                s64 current_idx = low;

                while (true) {
                    if (is_incremental) {
                        if (current_idx > high) break;
                    } else {
                        if (current_idx < high) break;
                    }

                    Al_Object* idx_object = new(Al_Object);
                    idx_object->ref_count = 1;

                    if (range_kind == rkInt) {
                        idx_object->kind   = okInt;
                        idx_object->as_int = current_idx;
                    } else {
                        idx_object->kind    = okChar;
                        idx_object->as_char = (char)current_idx;
                    }

                    ot_put(loop_scope->locals, for_loop.iterator, idx_object);

                    Al_Object* iteration_res = eval_ast_in(eval, loop_scope, for_loop.body);
                    obj_release(iteration_res);

                    if (is_incremental) current_idx++; else current_idx--;
                }
            }
            else if (iterable->kind == okVector) {
                Vector* vec = iterable->as_vector;

                for (size_t i = 0; i < vec->len; i++) {
                    Al_Object* elt = vec->data[i];
                    if (!elt) continue;

                    // this balances the release that happens when ot_put overwrites it on the next iteration
                    ot_put(loop_scope->locals, for_loop.iterator, obj_retain(elt));

                    Al_Object* iteration_res = eval_ast_in(eval, loop_scope, for_loop.body);
                    obj_release(iteration_res);
                }
            } else if (iterable->kind == okString) {
                String* str = iterable->as_string;
                for (size_t i = 0; i < str->len; i++) {
                    Al_Object* char_obj = new(Al_Object);
                    char_obj->kind      = okChar;
                    char_obj->ref_count = 1;
                    char_obj->as_char   = str->chars[i];

                    ot_put(loop_scope->locals, for_loop.iterator, char_obj);
                    Al_Object* iteration_res = eval_ast_in(eval, loop_scope, for_loop.body);
                    obj_release(iteration_res);
                }
            }

           /* for (size_t idx = 0; idx < loop_scope->locals->cap; idx++) {
                if (loop_scope->locals->entries[idx].key == for_loop.iterator) {
                    Al_Object* final_val = loop_scope->locals->entries[idx].value;
                    loop_scope->locals->entries[idx].value = nullptr;
                    if (final_val) obj_release(final_val);
                    break;
                }
            }*/


            obj_release(iterable);
            env_release(loop_scope);

            #undef for_loop
            return (Al_Object*)&TRUE;
        }

        case nkRangeExpr: {
            #define range_expr ast->as_range_expr
            Al_Object* start = eval_ast_in(eval, scope, range_expr.start);
            Al_Object* end   = eval_ast_in(eval, scope, range_expr.end);

            assert((start->kind == okInt || start->kind == okChar) && (end->kind == okInt || end->kind == okChar) &&
                   "Runtime Error: Range boundaries must evaluate to ordinal types (Char, Int).");
            assert(start->kind == end->kind && "Runtime Error: Range boundaries must be of the same type");

            Al_Object* result = new(Al_Object);
            result->kind      = okRange;
            result->ref_count = 1;

            if (start->kind == okInt) {
                result->as_range = (Al_Range_Object) {
                    .kind = rkInt,
                    .as_int_range = {
                        .low  = start->as_int,
                        .high = end->as_int
                    }
                };
            } else {
                result->as_range = (Al_Range_Object) {
                    .kind = rkChar,
                    .as_char_range = {
                        .low   = start->as_char,
                        .high  = end->as_char
                    }
                };
            }

            obj_release(start);
            obj_release(end);
            #undef range_expr
            return result;
        }



        default: {
            die("Unimplemented case in eval_ast");
        }
    }
}



void eval_dofile(Eval_Runtime* eval, const char* restrict filename) {
    prime_reader_from_file(eval->reader, filename);
    while (eval->reader->lexer->token.kind != tkEof) {
        AST_Node* node = read(eval->reader);
        Al_Object* obj = eval_ast_in(eval, eval->toplevel, node);
        obj_release(obj);
    }
}

void eval_dofile_s(Eval_Runtime* eval, const String* restrict filename) {
    String* temp = str_of_cap(filename->len + 1);
    str_append_bytes_unsafe(temp, filename->chars, filename->len);

    constexpr char terminator = '\0';
    str_append_bytes_unsafe(temp, &terminator, 1);

    prime_reader_from_file(eval->reader, temp->chars);
    while (eval->reader->lexer->token.kind != tkEof) {
        AST_Node* node = read(eval->reader);
        Al_Object* obj = eval_ast_in(eval, eval->toplevel, node);
        obj_release(obj);
    }

    str_free(temp);
}

Al_Object* eval_dostring(Eval_Runtime* eval, const cstring string) {
    Al_Object* result = (Al_Object*)&TRUE;
    String* temp = str_of_cstr(string);

    prime_lexer_from_string(eval->reader->lexer, temp);
    next_token(eval->reader->lexer);

    Vector* program = vec_new_of_cap(8);
    while (eval->reader->lexer->token.kind != tkEof) {
        AST_Node* node = read(eval->reader);
        vec_append_unsafe(program, node);
    }
    u32 expr_index = 0;
    vforeach(AST_Node, program, node) {
        expr_index++;
        result = eval_ast_in(eval, eval->toplevel, node) ;
        if (expr_index != program->len) obj_release(result);
    }
    vec_destroy_if_elements_are_pooled(program);
    return result;
}


void register_builtin(const Eval_Runtime* eval, const cstring name, const Al_Eval_Builtin builtin) {
    const String_View sv = {.buf = (char*)name, .len = strlen(name)};
    Symbol* builtin_name = get_symbol_if_interned_or_alloc_and_intern_it(eval->reader, &sv);
    Al_Object* builtin_obj = new(Al_Object);
    *builtin_obj = (Al_Object){
        .kind = okBuiltin,
        .ref_count = 1,
        .as_builtin = {
            .rout_name = builtin_name,
            .builtin   = builtin
        }
    };
    ot_put(eval->toplevel->locals, builtin_name, builtin_obj);
}

