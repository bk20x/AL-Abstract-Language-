#ifndef ALD_AL_ENVIRONMENT_H
#define ALD_AL_ENVIRONMENT_H
#include "al_objtab.h"
typedef struct Environment Environment;
struct Environment {
    Environment*  parent;
    Object_Table* locals;
};
Environment* env_new(Environment* parent);
#endif //ALD_AL_ENVIRONMENT_H
