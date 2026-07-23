#ifndef ALD_AL_ENVIRONMENT_H
#define ALD_AL_ENVIRONMENT_H
#include "al_objtab.h"
typedef struct Environment Environment;
struct Environment {
    Environment*  parent;
    Object_Table* locals;
    u32           refs;
};
Environment* env_new(Environment* parent);
Al_Object*   env_lookup_symbol(const Environment*, const Symbol* symbol);

Environment* env_retain(Environment* env);
void         env_release(Environment* env);
#endif //ALD_AL_ENVIRONMENT_H
