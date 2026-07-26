#ifndef ALD_AL_OBJECT_H
#define ALD_AL_OBJECT_H

#include "al_cdefs.h"
#include "al_ast.h"
#include "al_string.h"

// mf circular deps
typedef struct Environment Environment;
typedef struct Eval_Runtime Eval_Runtime;

typedef enum : u32 {
    okInt,
    okFloat,
    okBool,
    okChar,
    okString,
    okFunction,
    okBuiltin,
    okStruct,
    okRange,
    okVector,
    okEnvironment
} Al_Object_Kind;
String* string_of_object_kind(Al_Object_Kind);

typedef struct Al_Object Al_Object;

typedef struct {
    AST_Node*    routine_ast;
    Environment* scope;
} Al_Routine;

typedef Al_Object*(*Al_Eval_Builtin)(Eval_Runtime*, Environment*, AST_Node*);
typedef struct {
    Al_Eval_Builtin builtin;
    Symbol*         rout_name;
} Al_Builtin_Object;


typedef struct {
    s64 low;
    s64 high;
} Al_Int_Range_Object;

typedef struct {
    char low;
    char high;
} Al_Char_Range_Object;

typedef enum : u64 {
    rkInt,
    rkChar,
} Al_Range_Kind;

typedef struct {
    Al_Range_Kind kind;
    union {
        Al_Int_Range_Object  as_int_range;
        Al_Char_Range_Object as_char_range;
    };
} Al_Range_Object;


struct Al_Object {
    Al_Object_Kind kind;
    u32            ref_count;
    union {
        Al_Range_Object     as_range;
        Al_Builtin_Object   as_builtin;
        Environment*        as_env;
        Al_Routine*         as_function;
        Vector*             as_vector;
        String*             as_string;
        s64                 as_int;
        float               as_float;
        char                as_char;
        bool                as_bool;
    };
};

static constexpr Al_Object FALSE = {.kind = okBool, .as_bool = false};
static constexpr Al_Object TRUE =  {.kind = okBool, .as_bool = true};

Al_Object* obj_retain(Al_Object* object);
void       obj_release(Al_Object*);
void       destroy_object_contents(Al_Object*);

String* string_of_object(const Al_Object*);
void    print_object(const Al_Object*);
bool    obj_eq(const Al_Object* restrict, const Al_Object* restrict);
bool    complex_object_eq(const Al_Object*  restrict a, const Al_Object* restrict b);
#endif //ALD_AL_OBJECT_H
