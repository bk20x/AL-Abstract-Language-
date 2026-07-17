#ifndef ALD_AL_AST_H
#define ALD_AL_AST_H
#include "al_cdefs.h"
#include "al_string.h"
#include "al_symbol.h"
#include "al_vector.h"
#define MAX_IDENT_LEN 128


enum Node_Kind : u64 /*for now until we need to shove more shit on there*/ {
    nkVarDecl,
    nkSymLit,
    nkStrLit,
    nkIntLit,
    nkFloatLit,
    nkFuncall,
    nkFuncallParamList,
    nkStmt,
    nkStmtList
};
typedef enum Node_Kind Node_Kind;
String* string_of_node_kind(Node_Kind);
/*{base}*/
typedef struct AST_Node AST_Node;

typedef s64     IntLit;
typedef String* StrLit;
typedef float   FloatLit;

typedef struct Var_Decl_Node Var_Decl_Node;
struct Var_Decl_Node {
    Symbol*   lhs;
    AST_Node* rhs;
};

typedef struct Funcall_Param_List Funcall_Param_List;
struct Funcall_Param_List {
    Vector* params;
};

typedef struct Funcall_Node Funcall_Node;
struct Funcall_Node {
    AST_Node*    func_name;
    AST_Node*    func_params;
};

typedef struct Stmt_Node Stmt_Node;
struct Expression {

};

struct AST_Node {
    Node_Kind kind;
    union {
        Funcall_Param_List* as_param_list;
        Funcall_Node  as_funcall;   // nkFuncall
        Var_Decl_Node as_vardecl;   // nkVarDecl
        FloatLit      as_float_lit; // nkFloatLit
        IntLit        as_int_lit;   // nkIntLit
        StrLit        as_strlit;    // nkStrLit
        Symbol*       as_symbol;    // nkSymLit
    };
};

void destroy_ast(AST_Node*);

#endif //ALD_AL_AST_H
