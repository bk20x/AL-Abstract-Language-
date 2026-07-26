#include "al_object.h"

#include <assert.h>
#include <string.h>

#include "al_alloc.h"
#include "al_assert.h"
#include "al_environment.h"



Al_Object* obj_retain(Al_Object* object) {
    if (object && object->kind != okBool) {
        object->ref_count++;
        if (object->kind == okEnvironment) {
            env_retain(object->as_env);
        }
    }
    return object;
}

void destroy_object_contents(Al_Object* object) {
    if (!object) return;

    switch (object->kind) {
        case okString: {
            str_free(object->as_string);
            object->as_string = nullptr;

            break;
        }
        case okFunction: {
                Environment* scope_to_release = object->as_function->scope;
                object->as_function->scope = nullptr;

                if (scope_to_release->locals) {
                    for (size_t i = 0; i < scope_to_release->locals->cap; i++) {
                        Al_Object* val = scope_to_release->locals->entries[i].value;
                        if (val && val->kind == okFunction && val != object) {
                            scope_to_release->locals->entries[i].value = nullptr;
                            obj_release(val);
                        }
                    }
                }

                env_release(scope_to_release);
                dealloc(object->as_function);
                object->as_function = nullptr;

            break;
        }
        case okVector: {
            Vector* vec = object->as_vector;
            object->as_vector = nullptr;

            if (vec != nullptr) {
                for (size_t i = 0; i < vec->len; i++) {
                    Al_Object* obj = vec->data[i];
                    if (obj != nullptr) {
                        vec->data[i] = nullptr;
                        obj_release(obj);
                    }
                }
                vec_destroy(vec);
            }
            break;
        }
        case okEnvironment: {
            Environment* targ = object->as_env;
            object->as_env = nullptr;
            if (targ) {
                if (targ->locals) {
                    ot_destroy(targ->locals);
                    targ->locals = nullptr;
                }
                if (targ->parent) {
                    env_release(targ->parent);
                    targ->parent = nullptr;
                }
                dealloc(targ);
            }
            break;
        }

        case okInt:
            break;
        case okFloat:
            break;
        case okBuiltin:
            break;
        case okRange:
            break;
        case okChar:
            break;
        case okBool:
            break;
        default: die("Not done in destroy_object_contents");
    }
}

void obj_release(Al_Object* object) {
    if (!object || object->kind == okBool) return;
    assert(object->ref_count > 0 && "Object reference count underflow!");

    object->ref_count--;
    if (object->ref_count > 0) return;

    destroy_object_contents(object);
    dealloc(object);
}

void print_object(const Al_Object* object) {
    switch (object->kind) {
        case okInt: {
            printf("%ld", object->as_int);
            break;
        }
        case okFloat: {
            printf("%f", object->as_float);
            break;
        }
        case okString: {
            str_print(object->as_string);
            break;
        }
        case okFunction: {
            printf("#<Function %.*s>",
                (int)object->as_function->routine_ast->as_func_def.rout_name->len,
                object->as_function->routine_ast->as_func_def.rout_name->name);
            break;
        }
        case okBuiltin: {
            printf("#<Builtin %.*s>",
                (int)object->as_builtin.rout_name->len,
                object->as_builtin.rout_name->name);
            break;
        }
        case okVector: {
            printf("#<Vector %p | $len = %lu $cap = %lu>", object, object->as_vector->len, object->as_vector->cap);
            break;
        }
        case okBool: {
            printf(object->as_bool ? "true" : "false");
            break;
        }
        case okChar: {
            printf("%c", object->as_char);
            break;
        }
        case okRange: {
            switch (object->as_range.kind) {
                case rkInt: {
                    printf("%ld..%ld", object->as_range.as_int_range.low, object->as_range.as_int_range.high);
                    break;
                }
                case rkChar: {
                    printf("%c..%c", object->as_range.as_char_range.low, object->as_range.as_char_range.high);
                    break;
                }
            }

            break;
        }
        case okEnvironment: {
            printf("#<Environment %p | $parent = %p>",
                object->as_env,
                object->as_env->parent != nullptr ? object->as_env->parent : (void*)nullptr);
            break;
        }
        default:
            str_print(string_of_object_kind(object->kind));
            die("Not done in print object");
    }
}

String* string_of_object_kind(const Al_Object_Kind kind) {
    switch (kind) {
        case okInt:         return str_of_cstr("Int");
        case okFloat:       return str_of_cstr("Float");
        case okBool:        return str_of_cstr("Bool");
        case okString:      return str_of_cstr("String");
        case okBuiltin:     return str_of_cstr("Builtin");
        case okFunction:    return str_of_cstr("Function");
        case okVector:      return str_of_cstr("Vector");
        case okEnvironment: return str_of_cstr("Environment");
        default: die("Not done in string_of_object_kind");
    }
}

String* string_of_object(const Al_Object* object) {
    switch (object->kind) {
        case okChar: {
            String* result = str_of_cap(1);
            result->len = 1;
            result->chars[0] = object->as_char;
            return result;
        }
        case okBool: {
            return str_of_cstr(object->as_bool ? "true" : "false");
        }
        case okInt: {
            String* result = str_of_cap(8);
            str_appendf(result, "%ld", object->as_int);
            return result;
        }
        case okFloat: {
            String* result = str_of_cap(8);
            str_appendf(result, "%f", object->as_float);
            return result;
        }
        case okString: {
            return str_byteslice(object->as_string->chars, 0, object->as_string->len);
        }
        case okBuiltin: {
            String* result = str_of_cap(32);
            str_appendf(result, "#<Builtin %.*s>\n", (int)object->as_builtin.rout_name->len, object->as_builtin.rout_name->name);
            return result;
        }
        case okFunction: {
            String* result = str_of_cap(32);
            str_appendf(result, "#<Function %.*s>",
                        (int)object->as_function->routine_ast->as_func_def.rout_name->len,
                        object->as_function->routine_ast->as_func_def.rout_name->name);
            return result;
        }
        case okVector: {
            String* result = str_of_cap(64);
            str_appendf(result, "#<Vector %p | $len = %lu $cap = %lu>", object, object->as_vector->len, object->as_vector->cap);
            return result;
        }
        case okRange: {
            String* result = str_of_cap(16);
            switch (object->as_range.kind) {
                case rkInt: {
                    str_appendf(result, "%ld..%ld", object->as_range.as_int_range.low, object->as_range.as_int_range.high);
                    break;
                }
                case rkChar: {
                    str_appendf(result, "%c..%c", object->as_range.as_char_range.low, object->as_range.as_char_range.high);
                    break;
                }
            }
            return result;
        }
        case okEnvironment: {
            String* result = str_of_cap(64);
            str_appendf(result, "#<Environment %p | $parent = %p>",
                object->as_env,
                object->as_env->parent != nullptr ? object->as_env->parent : (void*)nullptr);
            return result;
        }
        default: die("Not done in string_of_object_kind");
    }
}
bool obj_eq(const Al_Object* const restrict a, const Al_Object* const restrict b) {
    if (a == b) return true;
    if (a->kind!=b->kind) return false;
    switch (a->kind) {
        case okInt: {
            return a->as_int == b->as_int;
        }
        case okFloat: {
            return a->as_float == b->as_float;
        }
        case okBool: {
            return a->as_bool == b->as_bool;
        }
        case okChar: {
            return a->as_char == b->as_char;
        }
        case okBuiltin: {
            return a->as_builtin.builtin == b->as_builtin.builtin;
        }
        case okFunction: {
            return a->as_function->routine_ast == b->as_function->routine_ast;
        }
        case okString: {
            if (a->as_string->len!=b->as_string->len) return false;
            return memcmp(a->as_string->chars, b->as_string->chars, a->as_string->len) == 0;
        }
        case okVector:
            return a == b;
        case okEnvironment:
            return a == b;
        default: die("im not done here yet!");
    }
}

