#ifndef ALD_AL_STRINGVIEW_H
#define ALD_AL_STRINGVIEW_H
#include "al_string.h"

typedef struct {
    char*   buf;
    size_t  len;
} String_View;
#define SV_SLICE(s, lo, hi) ((String_View){.buf=s+lo, .len = hi - lo})
#define SV_UNSAFE_COPY_INTO_STR(sv, str) do{memcpy(str->chars, sv.buf, sv.len); str->len = sv.len;} while(0)

bool sv_copy_into_str(String_View, String*);
void sv_print(String_View);





#endif //ALD_AL_STRINGVIEW_H
