#include "al_stringview.h"

#include <string.h>

bool sv_copy_into_str(const String_View sv, String* str) {
    if (!str || str->cap < sv.len) return false;
    SV_UNSAFE_COPY_INTO_STR(sv, str);
    return true;
}

void sv_print(const String_View sv) {
    fwrite(sv.buf, sizeof(char), sv.len, stdout);
}
