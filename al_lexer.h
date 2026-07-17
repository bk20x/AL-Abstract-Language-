#ifndef ALD_LEXER_H
#define ALD_LEXER_H
#include "al_string.h"
#include "al_stringview.h"
#include "al_symbol.h"


typedef enum : u64 {
    tkLPar,     // `(`
    tkRPar,     // `)`
    tkLBrace,   // `{`
    tkRBrace,   // `}`
    tkRout,     // `%`
    tkComma,    // `,`
    tkVar,      // `var`
    tkOperator, // generic Token kind for all operators
    tkInt,
    tkFloat,
    tkSymbol,
    tkString,
    tkEof,
    tkError
} Token_Kind;

typedef enum : u32 {
/*---Special Operators---*/
    opAssign,  // `=`
/*---Comparators-----*/
    opEq,      // `==`
    opGThan,   // `>`
    opGThanEq, // `>=`
    opLThan,   // `<`
    opLThanEq, // `<=`
/*---Arithmetic------*/
    opSub,     // `-`
    opAdd,     // `+`
    opMul,     // `*`
    opDiv      // `/`
} Operator_Kind;

static constexpr u32 Operator_Precedences[] = {
    [opAssign]  = 0,
    [opEq]      = 1,
    [opGThan]   = 1,
    [opGThanEq] = 1,
    [opLThan]   = 1,
    [opLThanEq] = 1,
    [opSub]     = 2,
    [opAdd]     = 2,
    [opMul]     = 3,
    [opDiv]     = 3
};

typedef struct {
    Operator_Kind kind;
    u32           precedence;
} Operator_Record;

#define OPERATOR(kind) ((Operator_Record){kind, Operator_Precedences[kind]})


typedef struct {
    u32 line, col;
} Line_Infos;

typedef struct {
    const char* msg;
    Line_Infos  line_info;
} Lexing_Error;

typedef struct {
    Token_Kind kind;
    union {
        String_View     view_val; // for strings and symbols to avoid allocations until the parser really uses something
        Lexing_Error    err_val;
        Operator_Record opr_val;
        s64             int_val;
        float           flt_val;
    };
} Token;

typedef struct {
    Token            token;
    Token            last_token;
    String*          buf;
    size_t           pos;
    int              line;
    int              col;
} Lexer;

/* Constructor Division */
Lexer* init_lexer();
Lexer* init_lexer_from_file(const char*);
void   prime_lexer_from_string(Lexer*, String*);
bool   prime_lexer_from_file(Lexer*, const char*);

/* Destructor Division */
void   deinit_lexer(Lexer*);

/* [Parsing] Operations Division */
void   skip_whitespace(Lexer*);
void   lex_symbol(Lexer*);
void   lex_string(Lexer*);
void   lex_number(Lexer*);
void   next_token(Lexer*);
void   assert_token_is_kind(const Lexer*, Token_Kind expected);
#define advance(lexer, by) do { \
    auto _by = (by);            \
    lexer->pos += _by;          \
    lexer->col += _by;          \
} while(0)

// Extra stuff
String* string_of_token_kind(Token_Kind);
#endif //ALD_LEXER_H
