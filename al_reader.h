#ifndef ALD_AL_PARSER_H
#define ALD_AL_PARSER_H
#include "al_lexer.h"
#include "al_ast.h"
#include "al_symtab.h"
#include "al_elastic_fixed_size_pool.h"

typedef struct {
    Lexer*                   lexer;
    Symbol_Table*            interned_symbols; // symbols already recognized
    Elastic_Fixed_Size_Pool  ast_pool;
} Reader;
/* Constructor Division */
Reader*     init_reader();
Reader*     init_reader_from_file(cstring);
/* Destructor Division */
void        deinit_reader(Reader*);

void prime_reader_from_file(const Reader*, cstring);

/* [Parsing] Operations Division */
AST_Node*   read(Reader*);

/* Symbol Operations Division */
Symbol* get_symbol_if_interned_or_alloc_and_intern_it(const Reader*, const String_View*);

#endif //ALD_AL_PARSER_H
