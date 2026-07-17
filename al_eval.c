#include "al_eval.h"

#include <assert.h>
#include <string.h>

#include "al_alloc.h"
#include "al_assert.h"


Eval_Runtime eval_init() {
    Environment* toplevel = env_new(nullptr);
    assert(toplevel);
    Reader* reader = init_reader();
    return (Eval_Runtime) {
        .toplevel = toplevel,
        .reader   = reader
    };
}

Eval_Runtime eval_init_from_file(const cstring filename) {
    Environment* toplevel = env_new(nullptr);
    assert(toplevel);
    Reader* reader = init_reader_from_file(filename);
    return (Eval_Runtime) {
        .toplevel = toplevel,
        .reader   = reader
    };
}

Al_Object* eval_ast(Eval_Runtime* current_scope, AST_Node* ast) {
    switch (ast->kind) {
        case nkIntLit: {
            Al_Object* result = new(Al_Object);
            result->kind   = okInt;
            result->as_int = ast->as_int_lit;
            return result;
        }
        case nkFloatLit: {
            Al_Object* result = new(Al_Object);
            result->kind     = okFloat;
            result->as_float = ast->as_float_lit;
            return result;
        }
        case nkStrLit: {
            Al_Object* result = new(Al_Object);
            result->kind      = okString;
            result->as_string = ast->as_strlit;
            return result;
        }
        case nkVarDecl: {
            #define vardecl ast->as_vardecl
            Al_Object* var_val = eval_ast(current_scope, vardecl.rhs);
            ot_put(current_scope->toplevel->locals, vardecl.lhs, var_val);
            return (Al_Object*)&TRUE;
            break;
        }
        default: {
            die("Unimplemented case in eval_ast");
        }
    }
}