#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "al_lexer.h"
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
        .token = {.kind = tkEof}
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
        .token = {.kind = tkEof}
    };
}

Lexer* init_lexer_from_file(const char* filename) {
    String* file_contents = read_entire_file(filename);
    if (!file_contents) return nullptr;

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

void prime_lexer_from_file(Lexer* lexer, const char* filename) {
    if (lexer->buf != nullptr) {
        str_free(lexer->buf);
        lexer->buf = nullptr;
    }

    String* file_contents = read_entire_file(filename);
    if (!file_contents) {
        printf("Couldnt read file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    lexer->buf   = file_contents;
    lexer->line  = 1;
    lexer->col   = 1;
    lexer->pos   = 0;
    lexer->token = (Token){.kind = tkEof};
}


void deinit_lexer(Lexer* lexer) {
    if (lexer->buf) str_free(lexer->buf);
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
    const size_t buflen = lexer->buf->len;

    while (lexer->pos < buflen &&
          (IS_IDENT_START(lexer->buf->chars[lexer->pos])
          || IS_ASCII_DIGIT(lexer->buf->chars[lexer->pos]))) {
        advance(lexer, 1);
    }

    const auto sym_view = SV_SLICE(lexer->buf->chars, start, lexer->pos);
    const size_t len = lexer->pos - start;

    if (len == 3 && memcmp(sym_view.buf, "var", len) == 0) {
        lexer->token = (Token){.kind = tkVar};
    } else if (len == 2 && memcmp(sym_view.buf, "if", len) == 0) {
        lexer->token = (Token){.kind = tkIf};
    } else if (len == 4 && memcmp(sym_view.buf, "else", len) == 0) {
        lexer->token = (Token){.kind = tkElse};
    } else if (len == 2 && memcmp(sym_view.buf, "in", len) == 0) {
        lexer->token = (Token){
            .kind    = tkOperator,
            .opr_val = OPERATOR(opIn)
        };
    } else if (len == 3 && memcmp(sym_view.buf, "for", len) == 0) {
        lexer->token = (Token){.kind = tkFor};
    } else if (len == 4 && memcmp(sym_view.buf, "true", len) == 0) {
        lexer->token = (Token){.kind = tkBoolLit, .bool_lit_val = true};
    } else if (len == 5 && memcmp(sym_view.buf, "false", len) == 0) {
        lexer->token = (Token){.kind = tkBoolLit, .bool_lit_val = false};
    } else {
        lexer->token = (Token){
            .kind     = tkSymbol,
            .view_val = sym_view
        };
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
    const size_t buflen = lexer->buf->len;
    size_t start_pos = lexer->pos;
    bool is_negative = false;
    // sign?
    if (lexer->pos < buflen && lexer->buf->chars[lexer->pos] == '-') {
        is_negative = true;
        advance(lexer, 1);
    }
    while (lexer->pos < buflen && IS_ASCII_DIGIT(lexer->buf->chars[lexer->pos])) {
        advance(lexer, 1);
    }
    bool floating = false;
    if (lexer->pos < buflen && lexer->buf->chars[lexer->pos] == '.') {
        if (lexer->pos + 1 < buflen && IS_ASCII_DIGIT(lexer->buf->chars[lexer->pos + 1])) {
            floating = true;
            advance(lexer, 1); // eat '.'
            while (lexer->pos < buflen && IS_ASCII_DIGIT(lexer->buf->chars[lexer->pos])) {
                advance(lexer, 1);
            }
        }
    }

    size_t token_len = lexer->pos - start_pos;
    if (floating) {
        char tmp[64]; // this is stupid and im changing it later
        if (token_len >= sizeof(tmp)) {
            token_len = sizeof(tmp) - 1;
        }
        memcpy(tmp, &lexer->buf->chars[start_pos], token_len);
        tmp[token_len] = '\0';

        lexer->token = (Token) {
            .kind    = tkFloat,
            .flt_val = strtof(tmp, nullptr)
        };
    } else {
        s64 value = 0;
        size_t i = start_pos;

        if (is_negative) {
            i++;
        }

        while (i < lexer->pos) {
            value = (value * 10) + (lexer->buf->chars[i] - '0');
            i++;
        }

        if (is_negative) {
            value = -value;
        }

        lexer->token = (Token) {
            .kind    = tkInt,
            .int_val = value
        };
    }
}


void next_token(Lexer* lexer) {
    skip_whitespace(lexer);
    lexer->last_token = lexer->token;
    if (lexer->pos >= lexer->buf->len) {
        lexer->token = (Token){.kind = tkEof};
        return;
    }
    switch (lexer->buf->chars[lexer->pos]) {
        case '(': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkLPar};
            break;
        }
        case ')': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkRPar};
            break;
        }
        case '{': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkLBrace};
            break;
        }
        case '}': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkRBrace};
            break;
        }
        case '%': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkRout};
            break;
        }
        case '>': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkOperator, .opr_val = OPERATOR(opGThan)};
            break;
        }
        case '<': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkOperator, .opr_val = OPERATOR(opLThan)};
            break;
        }
        case '=': {
            advance(lexer, 1);
            if (lexer->pos < lexer->buf->len && lexer->buf->chars[lexer->pos] == '=') {
                advance(lexer, 1);
                lexer->token = (Token){.kind = tkOperator, .opr_val = OPERATOR(opEq)};
                break;
            }
            lexer->token = (Token){.kind = tkOperator, .opr_val = OPERATOR(opAssign)};
            break;
        }
        case '+': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkOperator, .opr_val = OPERATOR(opAdd)};
            break;
        }
        case '-': {
            const bool is_probably_infix_operator = (
                lexer->token.kind == tkInt    ||
                lexer->token.kind == tkFloat  ||
                lexer->token.kind == tkSymbol ||
                lexer->token.kind == tkRPar   ||
                lexer->token.kind == tkRBrace ||
                lexer->token.kind == tkString ||
                lexer->token.kind == tkChar
            );

            // for them mf negatives
            if (!is_probably_infix_operator && lexer->pos + 1 < lexer->buf->len && IS_ASCII_DIGIT(lexer->buf->chars[lexer->pos + 1])) {
                lex_number(lexer);
                break;
            }

            advance(lexer, 1);
            lexer->token = (Token){.kind = tkOperator, .opr_val = OPERATOR(opSub)};
            break;
        }


        case '*': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkOperator, .opr_val = OPERATOR(opMul)};
            break;
        }
        case '/': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkOperator, .opr_val = OPERATOR(opDiv)};
            break;
        }
        case ',': {
            advance(lexer, 1);
            lexer->token = (Token){.kind = tkComma};
            break;
        }
        case '.': {
            advance(lexer, 1); // eat `.`
            if (lexer->pos < lexer->buf->len && lexer->buf->chars[lexer->pos] == '.') {
                advance(lexer, 1); // eat `.` OH MT GOD ITS A RANGE!!!
                lexer->token = (Token){
                    .kind = tkOperator,
                    .opr_val = OPERATOR(opRange)
                };
                break;
            }
            fprintf(stderr, "Lexer Error: Unexpected single dot character.\n");
            exit(EXIT_FAILURE);
        }
        case '"': {
            lex_string(lexer);
            break;
        }
        case '\'': {
            advance(lexer, 1);

            // early EOF or empty chr lit ?
            if (lexer->pos >= lexer->buf->len || lexer->buf->chars[lexer->pos] == '\'') {
                fprintf(stderr, "Lexer Error: Empty or invalid character literal.\n");
                exit(EXIT_FAILURE);
            }

            char literal_char = '\0';

            // is esc seq?
            if (lexer->buf->chars[lexer->pos] == '\\') {
                advance(lexer, 1); // eat `\`

                if (lexer->pos >= lexer->buf->len) {
                    fprintf(stderr, "Lexer Error: Unexpected EOF inside escape sequence.\n");
                    exit(EXIT_FAILURE);
                }
                switch (lexer->buf->chars[lexer->pos]) {
                    case 'n':  literal_char = '\n'; break;
                    case 't':  literal_char = '\t'; break;
                    case 'r':  literal_char = '\r'; break;
                    case '\\': literal_char = '\\'; break;
                    case '\'': literal_char = '\''; break;
                    case '"':  literal_char = '"';  break;
                    case '0':  literal_char = '\0'; break;
                    default: {
                        fprintf(stderr, "Lexer Error: Unknown escape sequence '\\%c'.\n",
                                lexer->buf->chars[lexer->pos]);
                        exit(EXIT_FAILURE);
                    }
                }
                advance(lexer, 1);
            } else {
                literal_char = lexer->buf->chars[lexer->pos];
                advance(lexer, 1);
            }

            if (lexer->pos >= lexer->buf->len || lexer->buf->chars[lexer->pos] != '\'') {
                fprintf(stderr, "Lexer Error: Unclosed character literal.\n");
                exit(EXIT_FAILURE);
            }
            advance(lexer, 1); // eat `'`
            lexer->token = (Token){
                .kind = tkChar,
                .chr_val = literal_char
            };
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