
#include "al_ast.h"
#include "al_alloc.h"
#include <assert.h>
#include <string.h>


String* string_of_node_kind(const Node_Kind kind) {
    switch (kind) {
        case nkVarDecl: return str_of_cstr("nkVarDecl");
        case nkFuncall: return str_of_cstr("nkFuncall");
        case nkIntLit:  return str_of_cstr("nkIntLit");
        case nkStrLit:  return str_of_cstr("nkStrLit");
        case nkSymLit:  return str_of_cstr("nkSymLit");
        default: assert(false && "unimplemented some node kind in string_of_node_kind");
    }
}
