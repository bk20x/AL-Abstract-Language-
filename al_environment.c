#include "al_environment.h"

#include <assert.h>

#include "al_alloc.h"

Environment* env_new(Environment* parent) {
    Environment* result = new(Environment);
    *result = (Environment) {
        .parent = parent,
        .locals = create_object_table(16)
    };
    assert(result && result->locals);
    return result;
}
