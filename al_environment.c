#include "al_environment.h"

#include <assert.h>

#include "al_alloc.h"

Environment* env_new(Environment* parent) {
    Environment* result = new(Environment);
    *result = (Environment) {
        .parent    = parent ? env_retain(parent) : nullptr,
        .locals    = create_object_table(16),
        .refs      = 1
    };
    assert(result && result->locals);
    return result;
}



Al_Object* env_lookup_symbol(const Environment* scope, const Symbol* symbol) {
    const Environment* current = scope;

    while (current != nullptr) {
        Al_Object* value = ot_get(current->locals, symbol);
        if (value != nullptr) {
            return value;
        }
        current = current->parent;
    }

    return nullptr;
}


Environment* env_retain(Environment* env) {
    if (env) {
        env->refs++;
    }
    return env;
}

void env_release(Environment* env) {
    if (env == nullptr) return; // toplevel parent is nul
    assert(env->refs > 0 && "Environment reference count underflow!");

    env->refs--;
    if (env->refs > 0) return;

    ot_destroy(env->locals);
    env->locals = nullptr;

    Environment* current = env->parent;
    env->parent = nullptr;
    dealloc(env);

    while (current != nullptr) {
        assert(current->refs > 0 && "Parent environment reference count underflow!");
        current->refs--;
        if (current->refs > 0) {
            break;
        }
        Environment* next_parent = current->parent;
        // no refs left
        if (current->locals) {
            ot_destroy(current->locals);
            current->locals = nullptr;
        }

        current->parent = nullptr;
        dealloc(current);
        current = next_parent;
    }
}
