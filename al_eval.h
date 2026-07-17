#ifndef ALD_AL_EVAL_H
#define ALD_AL_EVAL_H
#include "al_environment.h"
#include "al_object.h"
#include "al_reader.h"

typedef struct Eval_Runtime Eval_Runtime;
struct Eval_Runtime {
    Reader*      reader;
    Environment* toplevel;
};
Eval_Runtime eval_init();
Eval_Runtime eval_init_from_file(cstring);
Al_Object*   eval_ast(Eval_Runtime*, AST_Node*);
void         eval_dofile(Eval_Runtime*, const char* filename);
#endif //ALD_AL_EVAL_H
