
#include "al_ast.h"
#include <assert.h>
#include "al_assert.h"


String* string_of_node_kind(const Node_Kind kind) {
    switch (kind) {
        case nkVarDecl:    return str_of_cstr("nkVarDecl");
        case nkFuncall:    return str_of_cstr("nkFuncall");
        case nkIntLit:     return str_of_cstr("nkIntLit");
        case nkStrLit:     return str_of_cstr("nkStrLit");
        case nkSymLit:     return str_of_cstr("nkSymLit");
        case nkBinaryExpr: return str_of_cstr("nkBinaryExpr");
        case nkFuncDef:    return str_of_cstr("nkFuncDef");
        case nkRangeExpr:  return str_of_cstr("nkRangeExpr");
        default:
            printf("%ld\n", (s64)kind);
            assert(false && "unimplemented some node kind in string_of_node_kind");
    }
}


void destroy_pooled_ast(const AST_Node* node) {
    switch (node->kind) {
        case nkIntLit:
        case nkFloatLit:
        case nkCharLit:
        case nkBoolLit:
        case nkSymLit:
            // owned by interned_symbols
            break;
        case nkStrLit: {
            if (node->as_str_lit) str_free(node->as_str_lit);
            break;
        }
        case nkFuncallParamList: {
            if (node->as_param_list != nullptr) {
                vec_destroy_if_elements_are_pooled(node->as_param_list);
            }
            break;
        }
        case nkFuncDef: {
            if (node->as_func_def.params != nullptr) {
                vec_destroy_if_elements_are_pooled(node->as_func_def.params);
            }
            break;
        }
        case nkDefine: break;
        case nkFuncall: {
            break;
        }
        case nkVarDecl: {
            break;
        }
        case nkBinaryExpr: {
            break;
        }
        case nkBlockLit: {
            vec_destroy_if_elements_are_pooled(node->as_block_lit);
            break;
        }
        case nkIfExpr: {
            break;
        }
        case nkForLoop: {
            break;
        }
        case nkRangeExpr: {
            break;
        }
        case nkDotAccess: {
            break;
        }
        default:
            str_print(string_of_node_kind(node->kind));
            die("Unhandled node kind in destroy_pooled_ast");
    }
}
