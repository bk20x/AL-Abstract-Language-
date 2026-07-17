#include "al_object.h"

#include <assert.h>

#include "al_alloc.h"
#include "al_assert.h"

const Al_Object FALSE = {
    .kind = okBool, .as_bool = false
};

const Al_Object TRUE = {
    .kind = okBool, .as_bool = true
};


void destroy_object(Al_Object* object) {
    switch (object->kind) {
        case okString: {
            str_free(object->as_string);
            break;
        }
        case okInt:
            break;
        case okFloat:
            break;
        case okBool:
            return; // bools are static
        default:
            die("Not done in destroy_object");
    }
    dealloc(object);
}

void print_object(const Al_Object* object) {
    switch (object->kind) {
        case okInt: {
            printf("%ld\n", object->as_int);
            break;
        }
        case okFloat: {
            printf("%f\n", object->as_float);
            break;
        }
        case okString: {
            str_println(object->as_string);
            break;
        }
        case okBool: {
            puts(object->as_bool ? "true" : "false");
            break;
        }
    }
}