#ifndef ALD_AL_AST_H
#define ALD_AL_AST_H
#include "al_cdefs.h"
#include "al_lexer.h"
#include "al_string.h"
#include "al_symbol.h"
#include "al_vector.h"
#define MAX_IDENT_LEN 128

enum Node_Kind : u64 {
    nkVarDecl,
    nkSymLit,
    nkStrLit,
    nkIntLit,
    nkFloatLit,
    nkBoolLit,
    nkCharLit,
    nkFuncall,
    nkBinaryExpr,
    nkFuncallParamList,
    nkFuncDef,
    nkBlockLit,
    nkIfExpr,
    nkForLoop,
    nkRangeExpr
};
typedef enum Node_Kind Node_Kind;
String* string_of_node_kind(Node_Kind);

typedef struct AST_Node AST_Node;

typedef s64     Int_Lit;
typedef String* Str_Lit;
typedef float   Float_Lit;
typedef bool    Bool_Lit;

typedef struct Var_Decl_Node Var_Decl_Node;
struct Var_Decl_Node {
    Symbol*   lhs;
    AST_Node* rhs;
};

typedef struct Funcall_Node Funcall_Node;
struct Funcall_Node {
    AST_Node*    func_name;
    AST_Node*    func_params;
};

typedef struct Func_Def_Node Func_Def_Node;
struct Func_Def_Node {
    Symbol*   rout_name;
    Vector*   params;
    AST_Node* body;
};

typedef struct Binary_Expr_Node Binary_Expr_Node;
struct Binary_Expr_Node {
    Operator_Kind op;
    AST_Node*     left;
    AST_Node*     right;
};

typedef struct If_Expr_Node If_Expr_Node;
struct If_Expr_Node {
    AST_Node* condition;
    AST_Node* body;
    AST_Node* maybe_else;
};

typedef struct {
    Symbol*   iterator;
    AST_Node* iterable;
    AST_Node* body;
} For_Loop_Node;


typedef struct {
    AST_Node* start;
    AST_Node* end;
} Range_Expr_Node;


typedef struct Expression Expression;
struct Expression {};


struct AST_Node {
    Node_Kind kind;
    union {
        Binary_Expr_Node  as_binary_expr;
        For_Loop_Node     as_for_loop;
        If_Expr_Node      as_if_expression;
        Func_Def_Node     as_func_def;
        Range_Expr_Node   as_range_expr;
        Funcall_Node      as_funcall;
        Var_Decl_Node     as_vardecl;
        Symbol*           as_symbol;
        String*           as_str_lit;
        Vector*           as_block_lit;
        Vector*           as_param_list;
        s64               as_int_lit;
        float             as_float_lit;
        bool              as_bool_lit;
        char              as_char_lit;
    };
};

void destroy_pooled_ast(AST_Node*);

#endif //ALD_AL_AST_H
