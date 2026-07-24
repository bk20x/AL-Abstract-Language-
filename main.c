#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "al_alloc.h"
#include "al_assert.h"
#include "al_eval.h"
#include "al_fileutils.h"
#include "al_reader.h"
#include "al_object.h"
#include "al_environment.h"
#include "al_ast.h"
#include "al_vector.h"
#include "al_cf.h"
static Al_Object* bprintln(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    if (!args || !args->as_param_list) {
        putc('\n', stdout);
        return (Al_Object*)&TRUE;
    }
    const Vector* exprs = args->as_param_list;
    for (size_t i = 0; i < exprs->len; i++) {
        AST_Node* expr = exprs->data[i];
        Al_Object* val = eval_ast_in(eval, scope, expr);

        print_object(val);
        obj_release(val);

    }
    putc('\n', stdout);
    return (Al_Object*)&TRUE;
}

static Al_Object* bstrcat(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 2 && "`strcat` expects 2 parameters");
    AST_Node* e1 = params->data[0];
    AST_Node* e2 = params->data[1];
    Al_Object* str1 = eval_ast_in(eval, scope, e1);
    Al_Object* str2 = eval_ast_in(eval, scope, e2);
    assert(str1->kind == okString && str2->kind == okString && "`strcat` expects 2 parameters of primitive kind String");

    String* new_s = str_of_cap(str1->as_string->len + str2->as_string->len);
    str_append_bytes_unsafe(new_s, str1->as_string->chars, str1->as_string->len);
    str_append_bytes_unsafe(new_s, str2->as_string->chars, str2->as_string->len);

    Al_Object* result = new(Al_Object);
    result->kind = okString;
    result->ref_count = 1;
    result->as_string = new_s;

    obj_release(str1);
    obj_release(str2);
    return result;
}

static Al_Object* bstrappend(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 2 && "`strappend` expects 2 arguments");

    AST_Node* e1 = params->data[0];
    AST_Node* e2 = params->data[1];
    Al_Object* str1 = eval_ast_in(eval, scope, e1);
    Al_Object* str2 = eval_ast_in(eval, scope, e2);
    assert(str1->kind == okString && str2->kind == okString && "`strappend` expects 2 parameters of kind String");

    str_append_bytes_unsafe(str1->as_string, str2->as_string->chars, str2->as_string->len);

    obj_release(str1);
    obj_release(str2);

    Al_Object* new_len = new(Al_Object);
    new_len->kind      = okInt;
    new_len->as_int    = str1->as_string->len;
    new_len->ref_count = 1;
    return new_len;
}

static Al_Object* btostring(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 1 && "`image` requires one argument of any kind");
    AST_Node*  expr = params->data[0];
    Al_Object* obj  = eval_ast_in(eval, scope, expr);


    String* result_string = string_of_object(obj);
    Al_Object* result = new(Al_Object);
    result->kind      = okString;
    result->as_string = result_string;
    result->ref_count = 1;

    obj_release(obj);

    return result;
}

static Al_Object* bload(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 1 && "`load` expects one argument");
    AST_Node* arg1 = params->data[0];
    if (arg1->kind == nkStrLit) {
        eval_dofile_s(eval, arg1->as_str_lit);
    } else if (arg1->kind == nkSymLit || arg1->kind == nkBlockLit || arg1->kind == nkFuncall) {
        Al_Object* str_obj = eval_ast_in(eval, scope, arg1);
        assert(str_obj->kind == okString && "`load` expects a string as its argument for filename");
        eval_dofile_s(eval, str_obj->as_string);
        obj_release(str_obj);
    } else {
        die("Invalid argument type for `load`");
    }
    return (Al_Object*)&TRUE;
}

static Al_Object* bmakevec(Eval_Runtime* eval, Environment* scope, AST_Node* restrict args) {
    const Vector* params = args->as_param_list;
    Vector* result_vec;
    if (params && params->len >= 1) {
        result_vec = vec_new_of_cap0(params->len);
        vforeach(AST_Node, params, expr) {
            Al_Object* elt = eval_ast_in(eval, scope, expr);
            vec_append_unsafe(result_vec, elt);
        }
    } else {
        result_vec = vec_new();
    }
    Al_Object* result = new(Al_Object);
    *result = (Al_Object){
        .kind = okVector,
        .ref_count = 1,
        .as_vector = result_vec
    };
    return result;
}

static Al_Object* baref(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 2 && "`aref` expects 2 arguments (Vector, Int)");
    Al_Object* vec_obj = eval_ast_in(eval, scope, params->data[0]);
    Al_Object* idx_obj = eval_ast_in(eval, scope, params->data[1]);
    assert(vec_obj->kind == okVector && idx_obj->kind == okInt && "Invalid argument types for `aref` expects (Vector, Int)");
    assert(vec_obj->as_vector->len > idx_obj->as_int && "in `aref`, attempt to index out of bounds");

    const s64 index = idx_obj->as_int;
    Al_Object* result_elt = obj_retain(vec_obj->as_vector->data[index]);

    obj_release(vec_obj);
    obj_release(idx_obj);
    return result_elt;
}



static Al_Object* baset(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 3 && "`aset` expects 3 arguments (Vector, Int, Any)");

    Al_Object* vec_obj = eval_ast_in(eval, scope, params->data[0]);
    Al_Object* idx_obj = eval_ast_in(eval, scope, params->data[1]);
    Al_Object* val_obj = eval_ast_in(eval, scope, params->data[2]);

    assert(vec_obj->kind == okVector && idx_obj->kind == okInt && "Invalid argument types for `aset` expects (Vector, Int, Any)");
    assert(vec_obj->as_vector->cap > idx_obj->as_int && "in `aset`, attempt to set index out of bounds");

    const s64 index = idx_obj->as_int;
    Al_Object* old_val = vec_obj->as_vector->data[index];
    vec_obj->as_vector->data[index] = val_obj;
    if (old_val != nullptr) {
        obj_release(old_val);
    }


    obj_release(vec_obj);
    obj_release(idx_obj);
    return (Al_Object*)&TRUE;
}

static Al_Object* bappend(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 2 && "`append` expects 2 arguments");
    assert(((AST_Node*)params->data[0])->kind != nkStrLit && "Cannot append to a literal string");
    Al_Object* coll_obj = eval_ast_in(eval, scope, params->data[0]);
    if (coll_obj->kind == okString) {
        obj_release(coll_obj);
        return bstrappend(eval, scope, args);
    }
    if (coll_obj->kind == okVector) {
        Al_Object* elt = eval_ast_in(eval, scope, params->data[1]);
        vec_append_unsafe(coll_obj->as_vector, elt);
        obj_release(coll_obj);
        return (Al_Object*)&TRUE;
    }
    die("Invalid type for first argument to append");
}

static Al_Object* blen(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 1 && "`len` expects 1 argument");
    Al_Object* target = eval_ast_in(eval, scope, args->as_param_list->data[0]);
    switch (target->kind) {
        case okString: {
            const auto result = new(Al_Object);
            result->kind      = okInt;
            result->ref_count = 1;
            result->as_int    = target->as_string->len;
            obj_release(target);
            return result;
        }
        case okVector: {
            const auto result = new(Al_Object);
            result->kind      = okInt;
            result->ref_count = 1;
            result->as_int    = target->as_vector->len;
            obj_release(target);
            return result;
        }
        default: die("Invalid argument type for `len`");
    }
}

static Al_Object* bhigh(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 1 && "`high` requires one argument of kind Range or Vector");
    Al_Object* range_or_vec_obj = eval_ast_in(eval, scope, args->as_param_list->data[0]);
    switch (range_or_vec_obj->kind) {
        case okRange: {
            const auto result = new(Al_Object);
            result->ref_count = 1;
            switch (range_or_vec_obj->as_range.kind) {
                case rkChar: {
                    result->kind    = okChar;
                    result->as_char = range_or_vec_obj->as_range.as_char_range.high;
                    obj_release(range_or_vec_obj);
                    return result;
                }
                case rkInt: {
                    result->kind   = okInt;
                    result->as_int = range_or_vec_obj->as_range.as_int_range.high;
                    obj_release(range_or_vec_obj);
                    return result;
                }
            }
            break;
        }
        case okVector: {
            const auto result = new(Al_Object);
            result->kind      = okInt;
            result->ref_count = 1;
            result->as_int    = range_or_vec_obj->as_vector->len - 1;
            obj_release(range_or_vec_obj);
            return result;
        }
        default: die("not done in builtin `high`");
    }
}
static Al_Object* blow(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 1 && "`low` requires one argument of kind Range or Vector");
    Al_Object* range_or_vec_obj = eval_ast_in(eval, scope, args->as_param_list->data[0]);
    switch (range_or_vec_obj->kind) {
        case okRange: {
            const auto result = new(Al_Object);
            result->ref_count = 1;
            switch (range_or_vec_obj->as_range.kind) {
                case rkChar: {
                    result->kind    = okChar;
                    result->as_char = range_or_vec_obj->as_range.as_char_range.low;
                    obj_release(range_or_vec_obj);
                    return result;
                }
                case rkInt: {
                    result->kind   = okInt;
                    result->as_int = range_or_vec_obj->as_range.as_int_range.low;
                    obj_release(range_or_vec_obj);
                    return result;
                }
            }
            break;
        }
        case okVector: {
            const auto result = new(Al_Object);
            result->kind      = okInt;
            result->ref_count = 1;
            result->as_int    = 0; // until i add custom index arrays (if i do)
            obj_release(range_or_vec_obj);
            return result;
        }
        default: die("not done in builtin `high`");
    }
}
static Al_Object* bcap(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 1 && "`cap` requires one argument of kind (Vector, String)");
    Al_Object* obj = eval_ast_in(eval, scope, args->as_param_list->data[0]);
    assert(obj->kind == okVector || obj->kind == okString && "`cap` requires its argument to be of kind (Vector, String)");
    Al_Object* result = new(Al_Object);
    result->kind      = okInt;
    result->ref_count = 1;
    if (obj->kind == okVector) {
        result->as_int = obj->as_vector->cap;
    } else {
        result->as_int = obj->as_string->cap;
    }
    obj_release(obj);
    return result;
}

static Al_Object* barrayOfCap(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 1 && "`arrayOfCap` expects one argument");

    Al_Object* cap_obj = eval_ast_in(eval, scope, args->as_param_list->data[0]);
    assert(cap_obj->kind == okInt && "`arrayOfCap` expects its argument to be in Int");

    Al_Object* result = new(Al_Object);
    result->kind      = okVector;
    result->as_vector = vec_new_of_cap0(cap_obj->as_int);
    result->ref_count = 1;

    obj_release(cap_obj);
    return result;
}

static Al_Object* bslurpfile(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 1 && "`slurp` expects on argument");
    AST_Node* fname_expr = params->data[0];
    if (fname_expr->kind == nkStrLit) {
        Al_Object* result  = new(Al_Object);
        result->kind = okString;
        result->ref_count = 1;
        result->as_string = read_entire_file_s(fname_expr->as_str_lit);
        return result;
    }

    Al_Object* fname_obj = eval_ast_in(eval, scope, fname_expr);
    assert(fname_obj->kind == okString && "`slurp` expects one argument of type string");
    Al_Object* result = new(Al_Object);
    result->kind = okString;
    result->ref_count = 1;
    result->as_string = read_entire_file_s(fname_obj->as_string);
    obj_release(fname_obj);
    return result;
}


static Al_Object* bstrcopy(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 1 && "`strcopy` expects one argument");
    AST_Node* p1 = params->data[0];
    Al_Object* obj = eval_ast_in(eval, scope, p1);
    assert(obj->kind == okString && "`strcopy` expects one argument of type String");
    Al_Object* result = new(Al_Object);
    result->kind      = okString;
    result->ref_count = 1;
    result->as_string = str_of_cap(obj->as_string->cap);
    str_append_bytes_unsafe(result->as_string, obj->as_string->chars, obj->as_string->len);
    obj_release(obj);
    return result;
}
static Al_Object* bsplitlines(Eval_Runtime* eval, Environment* scope, AST_Node* args) {
    const Vector* params = args->as_param_list;
    assert(params && params->len == 1 && "`splitLines` expects one argument");
    AST_Node* p1 = params->data[0];
    Al_Object* str_obj = eval_ast_in(eval, scope, p1);
    assert(str_obj->kind == okString && "`splitLines` requires its argument to be of type String");

    auto result  = new(Al_Object);
    result->kind = okVector;
    result->ref_count = 1;
    result->as_vector = vec_new_of_cap0(64);

    const size_t len  = str_obj->as_string->len;
    const char* chars = str_obj->as_string->chars;
    size_t start = 0;

    for (size_t i = 0; i < len; i++) {
        if (chars[i] == '\n' || chars[i] == '\r') {
            const size_t line_len = i - start;
            String* line = str_of_cap(line_len);
            if (line) {
                if (line_len > 0) {
                    str_append_bytes_unsafe(line, &chars[start], line_len);
                }

                Al_Object* string_obj = new(Al_Object);
                string_obj->kind = okString;
                string_obj->ref_count = 1;
                string_obj->as_string = line;

                vec_append(result->as_vector, string_obj);
            }
            if (chars[i] == '\r' && (i + 1) < len && chars[i + 1] == '\n') {
                i++;
            }
            start = i + 1;
        }
    }
    if (start <= len) {
        const size_t line_len = len - start;
        if (line_len > 0 || len == 0) {
            String* line = str_of_cap(line_len);
            if (line) {
                if (line_len > 0) {
                    str_append_bytes_unsafe(line, &chars[start], line_len);
                }

                Al_Object* string_obj = new(Al_Object);
                string_obj->kind = okString;
                string_obj->ref_count = 1;
                string_obj->as_string = line;

                vec_append(result->as_vector, string_obj);
            }
        }
    }

    obj_release(str_obj);
    return result;
}


static void run_repl(Eval_Runtime* eval) {
    char* line = nullptr;
    size_t s = 0;

    String* buffer = str_of_cap(64);
    printf("AL (Abstract Language) -- version %s\n", VERSION);
    puts("enter #q to quit");
    printf("> ");
    fflush(stdout);
    while (getline(&line, &s, stdin) != -1) {
        line[strcspn(line, "\n")] = '\0';
        size_t len = strlen(line);
        const int is_multiline = len > 0 && line[len - 1] == '\\';

        if (is_multiline) {
            line[len - 1] = '\0';
            len--;
        }

        str_ensure_cap(buffer, buffer->len + len + 1);
        str_append_bytes_unsafe(buffer, line, len);
        if (is_multiline) {
            printf(".. ");
            fflush(stdout);
            continue;
        }
        if (buffer->len == 2 && memcmp(buffer->chars, "#q", 2) == 0) {
            break;
        }
        if (buffer->len > 0) {
            buffer->chars[buffer->len] = '\0';

            Al_Object* eval_result = eval_dostring(eval, buffer->chars);
            printf("=> ");
            print_object(eval_result);
            putc('\n', stdout);
            obj_release(eval_result);
        }
        buffer->len = 0;
        printf("> ");
        fflush(stdout);
    }
    str_free(buffer);
    free(line); // would use dealloc here but if i change it, to not js be malloc, gotta use the libc shit
}

int main(const int argc, char** argv) {
    Eval_Runtime eval = eval_init();
    register_builtin(&eval, "print",     bprintln);
    register_builtin(&eval, "strcat",    bstrcat);
    register_builtin(&eval, "append",    bappend);
    register_builtin(&eval, "image",     btostring);
    register_builtin(&eval, "load",      bload);
    register_builtin(&eval, "array",     bmakevec);
    register_builtin(&eval, "arrayOfCap",barrayOfCap);
    register_builtin(&eval, "aref",      baref);
    register_builtin(&eval, "aset",      baset);
    register_builtin(&eval, "len",       blen);
    register_builtin(&eval, "low",       blow);
    register_builtin(&eval, "high",      bhigh);
    register_builtin(&eval, "cap",       bcap);
    register_builtin(&eval, "slurp",     bslurpfile);
    register_builtin(&eval, "strcopy",   bstrcopy);
    register_builtin(&eval, "splitLines",bsplitlines);
    if (argc == 1) {
        run_repl(&eval);
    } else if (argc == 2) {
        eval_dofile(&eval, argv[1]);
    }


    ot_destroy(eval.toplevel->locals);
    deinit_reader(eval.reader);
    dealloc(eval.toplevel);

    return 0;
}
