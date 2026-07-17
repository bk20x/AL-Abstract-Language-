#include "al_string.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include "al_alloc.h"
#define STRING_INITIAL_CAP 16

String* str_of_cap(const size_t cap) {
    String* result = new(String);
    if (!result) return nullptr;
    *result = (String) {
        .len       = 0,
        .cap       = cap,
        .chars     = alloc(sizeof(char) * cap)
    };
    if (!result->chars) {
        dealloc(result);
        return nullptr;
    }
    return result;
}

String* str_of_cstr(const char* cstr) {
    if (!cstr) return nullptr;

    String* result = new(String);
    if (!result) return nullptr;

    const size_t len = strlen(cstr);
    *result = (String) {
        .len       = len,
        .cap       = len,
        .chars     = alloc(sizeof(char) * len)
    };
    if (!result->chars) {
        dealloc(result);
        return nullptr;
    }
    memcpy(result->chars, cstr, len);
    return result;
}

void str_init_of_cap(String* restrict str, const size_t cap) {
    str->cap   = cap;
    str->len   = 0;
    str->chars = alloc(cap);
    assert(str->chars || cap == 0);
}

void str_init_of_cstr(String* restrict str, const char* restrict cstr) {
    const size_t len = strlen(cstr);

    str->len   = len;
    str->cap   = len;
    str->chars = alloc(len);

    assert(str->chars || len == 0);
    if (len > 0) memcpy(str->chars, cstr, len);
}


String* str_byteslice(const char* restrict buf, const size_t low, const size_t high) {
    assert(high >= low && buf);

    const size_t slice_len = high - low;
    String* result = str_of_cap(slice_len);
    if (!result) return nullptr;

    memcpy(result->chars, buf + low, slice_len);
    result->len = slice_len;
    return result;
}

s64 str_append_cstr(String* restrict str, const char* cstr) {
    if (!str || !cstr) return -1;

    const size_t cstr_len = strlen(cstr);
    const size_t min_cap  = str->len + cstr_len;
    if (min_cap > str->cap) {
        size_t new_cap = str->cap > 0 ? str->cap * 2 : STRING_INITIAL_CAP;
        if (min_cap > new_cap) new_cap = min_cap;

        char* new_chars = realloc(str->chars, new_cap);
        if (!new_chars) return -1;

        str->chars = new_chars;
        str->cap   = new_cap;
    }
    memcpy(str->chars + str->len, cstr, cstr_len);
    str->len += cstr_len;
    return (s64)str->len;
}

s64 str_append_bytes(String* restrict str, const char* bytes, const size_t n_bytes) {
    if (!str || !bytes) return -1;

    const size_t min_cap = str->len + n_bytes;
    if (min_cap > str->cap) {
        size_t new_cap = str->cap > 0 ? str->cap * 2 : STRING_INITIAL_CAP;
        if (min_cap > new_cap) new_cap = min_cap;

        char* new_chars = realloc(str->chars, new_cap);
        if (!new_chars) return -1;

        str->chars = new_chars;
        str->cap   = new_cap;
    }
    memcpy(str->chars + str->len, bytes, n_bytes);
    str->len += n_bytes;
    return (s64)str->len;
}

s64 str_appendf(String* str, const char* format, ...) {
    if (!str) return -1;
    va_list args, args_copy;
    va_start(args, format);
    va_copy(args_copy, args);

    const int formatted_len = vsnprintf(nullptr, 0, format, args);
    va_end(args);

    if (formatted_len < 0) {
        va_end(args_copy);
        return -1;
    }

    const size_t min_cap = str->len + formatted_len + 1;
    if (min_cap > str->cap) {
        size_t new_cap = str->cap > 0 ? str->cap * 2 : STRING_INITIAL_CAP;
        if (min_cap > new_cap) new_cap = min_cap;

        char* new_chars = realloc(str->chars, new_cap);
        if (!new_chars) {
            va_end(args_copy);
            return -1;
        }
        str->chars = new_chars;
        str->cap   = new_cap;
    }
    vsnprintf(str->chars + str->len, formatted_len + 1, format, args_copy);
    va_end(args_copy);
    str->len += formatted_len;
    return (s64)str->len;
}

void str_free(String* str) {
    dealloc(str->chars);
    dealloc(str);
}

void str_ensure_cap(String* str, const size_t needed_capacity) {
    if (str->cap >= needed_capacity) return;
    char* resized = realloc(str->chars, needed_capacity);
    assert(resized);
    str->chars = resized;
    str->cap   = needed_capacity;
}

void str_writen(const String* str, FILE* file, const size_t n) {
    fwrite(str->chars, sizeof(char), n <= str->len ? n : str->len, file);
}

void str_write(const String* str, FILE* file) {
    fwrite(str->chars, sizeof(char), str->len, file);
}

void str_print(const String* str) {
    fwrite(str->chars, sizeof(char), str->len, stdout);
}

void str_println(const String* str) {
    fwrite(str->chars, sizeof(char), str->len, stdout);
    fwrite("\n", sizeof(char), 1, stdout);
}
