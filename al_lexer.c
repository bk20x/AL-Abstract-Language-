#include <string.h>
#include "al_lexer.h"

#include <assert.h>

#include "al_alloc.h"
#include "al_charutils.h"
#include "al_fileutils.h"

Lexer* init_lexer() {
    Lexer* result = alloc(sizeof(Lexer));
    *result = (Lexer){
        .buf   = nullptr,
        .col   = 1,
        .line  = 1,
        .pos   = 0,
        .token = {tkEof}
    };
    assert(result);
    return result;
}

void prime_lexer_from_string(Lexer* lexer, String* str) {
    if (lexer->buf) str_free(lexer->buf);
    *lexer = (Lexer) {
        .buf   = str,
        .col   = 1,
        .line  = 1,
        .pos   = 0,
        .token = {tkEof}
    };
}

Lexer* init_lexer_from_file(const char* filename) {
    String* file_contents = read_entire_file(filename);
    if (!file_contents) return nullptr;

    str_ensure_cap(file_contents, file_contents->len+1);
    file_contents->chars[file_contents->len] = '\0'; // sentinel for lexer, `String` isn't null terminated

    Lexer* result = alloc(sizeof(Lexer));
    assert(result);

    *result = (Lexer) {
        .buf       = file_contents,
        .line      = 1,
        .col       = 1,
        .pos       = 0,
        .token     = {tkEof}
    };
    return result;
}

bool prime_lexer_from_file(Lexer* lexer, const char* filename) {
    if (!lexer || !lexer->buf) return false;
    str_free(lexer->buf);

    String* file_contents = read_entire_file(filename);
    if (!file_contents) return false;

    str_ensure_cap(file_contents, file_contents->len+1);
    file_contents->chars[file_contents->len] = '\0';

    lexer->buf   = file_contents;
    lexer->line  = 1;
    lexer->col   = 1;
    lexer->pos   = 0;
    lexer->token = (Token){tkEof};
    return true;
}

void deinit_lexer(Lexer* lexer) {
    str_free(lexer->buf);
    dealloc(lexer);
}

void skip_whitespace(Lexer* lexer) {
    while (lexer->pos < lexer->buf->len && lexer->buf->chars[lexer->pos] <= 32) {
        switch (lexer->buf->chars[lexer->pos]) {
            case '\r':
                lexer->line++;
                lexer->col = 1;
                if (lexer->pos + 1 < lexer->buf->len && lexer->buf->chars[lexer->pos + 1] == '\n') {
                    lexer->pos += 2;
                } else {
                    lexer->pos++;
                }
                break;
            case '\n':
                lexer->line++;
                lexer->col = 1;
                lexer->pos++;
                break;
            default:
                lexer->col++;
                lexer->pos++;
                break;
        }
    }
}

void lex_symbol(Lexer* lexer) {
    const size_t start = lexer->pos;
    char c = lexer->buf->chars[lexer->pos];
    while (lexer->pos < lexer->buf->len && (IS_IDENT_START(c) || IS_ASCII_DIGIT(c))) {
        advance(lexer, 1);
        c = lexer->buf->chars[lexer->pos];
    }
    const auto sym_view = SV_SLICE(lexer->buf->chars, start, lexer->pos);
    if (memcmp(sym_view.buf, "var", 3) == 0) {
        lexer->token = (Token){tkVar};
    } else {
        lexer->token = (Token){
            .kind    = tkSymbol,
            .view_val = sym_view
        };
        return;
    }
}

void lex_string(Lexer* lexer) {
    advance(lexer, 1); // `"`
    const Line_Infos start_loc = {.line = lexer->line, .col = lexer->col};
    const size_t start   = lexer->pos;
    const size_t buflen = lexer->buf->len;
    const char*  buf     = lexer->buf->chars;
    while (lexer->pos < buflen && buf[lexer->pos] != '"') {
        advance(lexer, 1);
    }
    // hit eof
    if (lexer->pos >= buflen) {
        lexer->token = (Token) {
            .kind = tkError,
            .err_val  = {
                .msg    = "Unterminated String Literal",
                .line_info = start_loc
            }
        };
        return;
    }
    lexer->token  = (Token) {
        .kind     = tkString,
        .view_val  = SV_SLICE(lexer->buf->chars, start, lexer->pos)
    };
    advance(lexer, 1); // `"`
}

void lex_number(Lexer* lexer) {
    bool floating = false;
    const size_t buflen = lexer->buf->len;
    const char*  start_addr  = &lexer->buf->chars[lexer->pos];
    while (lexer->pos < buflen) {
        const char c = lexer->buf->chars[lexer->pos];
        if (IS_ASCII_DIGIT(c)) {
            advance(lexer, 1);
        } else if (!floating && c == '.') {
            if (lexer->pos+1 < buflen && IS_ASCII_DIGIT(lexer->buf->chars[lexer->pos+1])) {
                floating = true;
                advance(lexer, 1);
            } else break;
        } else break;
    }
    if (floating) {
        lexer->token = (Token) {
            .kind    = tkFloat,
            .flt_val = strtof(start_addr, nullptr)
        };
    } else {
        lexer->token = (Token) {
            .kind    = tkInt,
            .int_val = strtoll(start_addr, nullptr, 10)
        };
    }
}

void next_token(Lexer* lexer) {
    skip_whitespace(lexer);
    lexer->last_token = lexer->token;
    switch (lexer->buf->chars[lexer->pos]) {
        case '(': {
            advance(lexer, 1);
            lexer->token = (Token){tkLPar};
            break;
        }
        case ')': {
            advance(lexer, 1);
            lexer->token = (Token){tkRPar};
            break;
        }
        case '{': {
            advance(lexer, 1);
            lexer->token = (Token){tkLBrace};
            break;
        }
        case '}': {
            advance(lexer, 1);
            lexer->token = (Token){tkRBrace};
            break;
        }
        case '%': {
            advance(lexer, 1);
            lexer->token = (Token){tkRout};
            break;
        }
        case '=': {
            advance(lexer, 1);
            if (lexer->pos < lexer->buf->len && lexer->buf->chars[lexer->pos] == '=') {
                advance(lexer, 1);
                lexer->token = (Token){tkOperator, .opr_val = OPERATOR(opEq)};
                break;
            }
            lexer->token = (Token){tkOperator, .opr_val = OPERATOR(opAssign)};
            break;
        }
        case '+': {
            advance(lexer, 1);
            lexer->token = (Token){tkOperator, .opr_val = OPERATOR(opAdd)};
            break;
        }
        case '-': {
            advance(lexer, 1);
            lexer->token = (Token){tkOperator, .opr_val = OPERATOR(opSub)};
            break;
        }
        case '*': {
            advance(lexer, 1);
            lexer->token = (Token){tkOperator, .opr_val = OPERATOR(opMul)};
            break;
        }
        case '/': {
            advance(lexer, 1);
            lexer->token = (Token){tkOperator, .opr_val = OPERATOR(opDiv)};
            break;
        }
        case ',': {
            advance(lexer, 1);
            lexer->token = (Token){tkComma};
            break;
        }
        case '"': {
            lex_string(lexer);
            break;
        }
        case '\0': {
            lexer->token = (Token){tkEof};
            break;
        }
        default: {
            const char c = lexer->buf->chars[lexer->pos];
            if (IS_IDENT_START(c)) {
                lex_symbol(lexer);
            } else if (IS_ASCII_DIGIT(c)) {
                lex_number(lexer);
            }
            break;
        }
    }
}

String* string_of_token_kind(const Token_Kind kind) {
    switch (kind) {
        case tkVar:     return str_of_cstr("tkVar `var`");
        case tkLPar:    return str_of_cstr("tkLPar `(`");
        case tkRPar:    return str_of_cstr("tkRPar `)`");
        case tkLBrace:  return str_of_cstr("tkLBrace `{`");
        case tkRBrace:  return str_of_cstr("tkRBrace `}`");
        case tkOperator:return str_of_cstr("tkOperator");
        case tkComma:   return str_of_cstr("tkComma `,`");
        case tkRout:    return str_of_cstr("tkRout `%`");
        case tkInt:     return str_of_cstr("tkInt");
        case tkFloat:   return str_of_cstr("tkFloat");
        case tkSymbol:  return str_of_cstr("tkSymbol");
        case tkString:  return str_of_cstr("tkString");
        case tkEof:     return str_of_cstr("tkEof");
        case tkError:   return str_of_cstr("tkError");
        default:        return str_of_cstr("???");
    }
}

void assert_token_is_kind(const Lexer* lexer, const Token_Kind expected) {
    assert(lexer->token.kind == expected && "Invalid token");
}