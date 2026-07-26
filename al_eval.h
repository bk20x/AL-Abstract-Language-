#ifndef ALD_AL_EVAL_H
#define ALD_AL_EVAL_H
#include "al_object.h"
#include "al_reader.h"

typedef Eval_Runtime Eval_Runtime;
struct Eval_Runtime {
    Reader*      reader;
    Environment* toplevel;
};
/* Constructor Division */
Eval_Runtime eval_init();

/* Operations Division [Evaluation] */
Al_Object*   eval_ast_in(Eval_Runtime*,   Environment*, AST_Node*);
void         eval_dofile(Eval_Runtime*,   cstring restrict filename);
void         eval_dofile_s(Eval_Runtime*, const String* restrict filename);
Al_Object    eval_dofile_export(Eval_Runtime*, const String* restrict filename);
Al_Object*   eval_dostring(Eval_Runtime*, cstring);



void         register_builtin(const Eval_Runtime*, cstring name, Al_Eval_Builtin builtin);
#endif //ALD_AL_EVAL_H
