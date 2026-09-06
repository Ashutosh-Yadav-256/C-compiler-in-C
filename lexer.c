#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void lex_init(lexer_t *l, const char *source) {
    l->source = source;
    l->pos = 0;
    l->line = 1;
    l->col = 1;
}

static void advance(lexer_t *l) {
    if (l->source[l->pos] == '\n') {
        l->line++;
        l->col = 1;
    } else {
        l->col++;
    }
    l->pos++;
}

static char peek(const lexer_t *l) {
    return l->source[l->pos];
}

static char peek_next(const lexer_t *l) {
    if (l->source[l->pos] == '\0') return '\0';
    return l->source[l->pos + 1];
}

static void skip_whitespace_and_comments(lexer_t *l) {
    while (1) {
        char c = peek(l);
        if (isspace((unsigned char)c)) {
            advance(l);
        } else if (c == '/' && peek_next(l) == '/') {
            advance(l);
            advance(l);
            while (peek(l) != '\0' && peek(l) != '\n') {
                advance(l);
            }
        } else if (c == '/' && peek_next(l) == '*') {
            advance(l);
            advance(l);
            while (peek(l) != '\0') {
                if (peek(l) == '*' && peek_next(l) == '/') {
                    advance(l);
                    advance(l);
                    break;
                }
                advance(l);
            }
        } else {
            break;
        }
    }
}

static int is_ident_start(unsigned char c) {
    return isalpha(c) || c == '_';
}

static int is_ident_part(unsigned char c) {
    return isalnum(c) || c == '_';
}

size_t lex_decode_escape_string(char *dest, const char *src, size_t src_len) {
    if (!dest || !src) return 0;
    size_t out_len = 0;
    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '\\' && i + 1 < src_len) {
            i++;
            char esc = src[i];
            switch (esc) {
                case 'n': dest[out_len++] = '\n'; break;
                case 't': dest[out_len++] = '\t'; break;
                case 'r': dest[out_len++] = '\r'; break;
                case '0': dest[out_len++] = '\0'; break;
                case '\\': dest[out_len++] = '\\'; break;
                case '\'': dest[out_len++] = '\''; break;
                case '"': dest[out_len++] = '"'; break;
                case 'x':
                case 'X': {
                    if (i + 1 < src_len && isxdigit((unsigned char)src[i + 1])) {
                        i++;
                        int val = isdigit((unsigned char)src[i]) ? src[i] - '0' : (tolower((unsigned char)src[i]) - 'a' + 10);
                        if (i + 1 < src_len && isxdigit((unsigned char)src[i + 1])) {
                            i++;
                            int digit2 = isdigit((unsigned char)src[i]) ? src[i] - '0' : (tolower((unsigned char)src[i]) - 'a' + 10);
                            val = (val << 4) | digit2;
                        }
                        dest[out_len++] = (char)val;
                    } else {
                        dest[out_len++] = esc;
                    }
                    break;
                }
                default: dest[out_len++] = esc; break;
            }
        } else {
            dest[out_len++] = src[i];
        }
    }
    return out_len;
}

token_t lex_next(lexer_t *l) {
    skip_whitespace_and_comments(l);

    token_t tok = {
        .kind = TOK_EOF,
        .start = &l->source[l->pos],
        .length = 0,
        .int_val = 0,
        .char_val = 0,
        .line = l->line,
        .col = l->col
    };
    char c = peek(l);

    if (c == '\0') {
        return tok;
    }

    if (isdigit((unsigned char)c)) {
        tok.kind = TOK_INT_LITERAL;
        int base = 10;
        if (c == '0') {
            if (peek_next(l) == 'x' || peek_next(l) == 'X') {
                base = 16;
                advance(l);
                advance(l);
            } else if (peek_next(l) == 'b' || peek_next(l) == 'B') {
                base = 2;
                advance(l);
                advance(l);
            } else if (isdigit((unsigned char)peek_next(l))) {
                base = 8;
                advance(l);
            }
        }

        long long val = 0;
        while (1) {
            char curr = peek(l);
            if (base == 16 && isxdigit((unsigned char)curr)) {
                int digit = isdigit((unsigned char)curr) ? curr - '0' : (tolower((unsigned char)curr) - 'a' + 10);
                val = val * 16 + digit;
                advance(l);
            } else if (base == 10 && isdigit((unsigned char)curr)) {
                val = val * 10 + (curr - '0');
                advance(l);
            } else if (base == 8 && curr >= '0' && curr <= '7') {
                val = val * 8 + (curr - '0');
                advance(l);
            } else if (base == 2 && (curr == '0' || curr == '1')) {
                val = val * 2 + (curr - '0');
                advance(l);
            } else {
                break;
            }
        }

        while (peek(l) == 'u' || peek(l) == 'U' || peek(l) == 'l' || peek(l) == 'L') {
            advance(l);
        }

        tok.length = (size_t)(&l->source[l->pos] - tok.start);
        tok.int_val = val;
        return tok;
    }

    if (c == '\'') {
        tok.kind = TOK_CHAR_LITERAL;
        advance(l);
        char ch = peek(l);
        if (ch == '\\') {
            advance(l);
            char esc = peek(l);
            switch (esc) {
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case 'r': ch = '\r'; break;
                case '0': ch = '\0'; break;
                case '\\': ch = '\\'; break;
                case '\'': ch = '\''; break;
                case '\"': ch = '\"'; break;
                default: ch = esc; break;
            }
            advance(l);
        } else if (ch != '\0' && ch != '\'') {
            advance(l);
        }
        if (peek(l) == '\'') {
            advance(l);
        } else {
            tok.kind = TOK_ERROR;
        }
        tok.length = (size_t)(&l->source[l->pos] - tok.start);
        tok.char_val = ch;
        tok.int_val = (unsigned char)ch;
        return tok;
    }

    if (is_ident_start(c)) {
        while (is_ident_part(peek(l))) {
            tok.length++;
            advance(l);
        }

        if (tok.length == 3 && strncmp(tok.start, "int", 3) == 0) tok.kind = TOK_INT_KW;
        else if (tok.length == 4 && strncmp(tok.start, "char", 4) == 0) tok.kind = TOK_CHAR_KW;
        else if (tok.length == 4 && strncmp(tok.start, "void", 4) == 0) tok.kind = TOK_VOID_KW;
        else if (tok.length == 4 && strncmp(tok.start, "long", 4) == 0) tok.kind = TOK_LONG_KW;
        else if (tok.length == 6 && strncmp(tok.start, "return", 6) == 0) tok.kind = TOK_RETURN_KW;
        else if (tok.length == 2 && strncmp(tok.start, "if", 2) == 0) tok.kind = TOK_IF_KW;
        else if (tok.length == 4 && strncmp(tok.start, "else", 4) == 0) tok.kind = TOK_ELSE_KW;
        else if (tok.length == 5 && strncmp(tok.start, "while", 5) == 0) tok.kind = TOK_WHILE_KW;
        else if (tok.length == 3 && strncmp(tok.start, "for", 3) == 0) tok.kind = TOK_FOR_KW;
        else if (tok.length == 5 && strncmp(tok.start, "break", 5) == 0) tok.kind = TOK_BREAK_KW;
        else if (tok.length == 8 && strncmp(tok.start, "continue", 8) == 0) tok.kind = TOK_CONTINUE_KW;
        else if (tok.length == 6 && strncmp(tok.start, "sizeof", 6) == 0) tok.kind = TOK_SIZEOF_KW;
        else tok.kind = TOK_IDENTIFIER;

        return tok;
    }

    if (c == '"') {
        tok.kind = TOK_STRING;
        advance(l);
        tok.start = &l->source[l->pos];
        const char *content_start = tok.start;
        while (peek(l) != '\0' && peek(l) != '"') {
            if (peek(l) == '\\' && peek_next(l) != '\0') {
                advance(l);
            }
            advance(l);
        }
        tok.length = (size_t)(&l->source[l->pos] - content_start);
        if (peek(l) == '"') {
            advance(l);
        } else {
            tok.kind = TOK_ERROR;
        }
        return tok;
    }

    char next_c = peek_next(l);
    if (c == '+' && next_c == '+') { tok.kind = TOK_INC; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '-' && next_c == '-') { tok.kind = TOK_DEC; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '-' && next_c == '>') { tok.kind = TOK_ARROW; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '+' && next_c == '=') { tok.kind = TOK_PLUS_ASSIGN; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '-' && next_c == '=') { tok.kind = TOK_MINUS_ASSIGN; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '*' && next_c == '=') { tok.kind = TOK_STAR_ASSIGN; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '/' && next_c == '=') { tok.kind = TOK_SLASH_ASSIGN; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '=' && next_c == '=') { tok.kind = TOK_EQ; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '!' && next_c == '=') { tok.kind = TOK_NEQ; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '<' && next_c == '=') { tok.kind = TOK_LTE; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '>' && next_c == '=') { tok.kind = TOK_GTE; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '<' && next_c == '<') { tok.kind = TOK_SHL; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '>' && next_c == '>') { tok.kind = TOK_SHR; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '&' && next_c == '&') { tok.kind = TOK_LOGICAL_AND; tok.length = 2; advance(l); advance(l); return tok; }
    if (c == '|' && next_c == '|') { tok.kind = TOK_LOGICAL_OR; tok.length = 2; advance(l); advance(l); return tok; }

    tok.length = 1;
    switch (c) {
        case '{': tok.kind = TOK_LBRACE; advance(l); return tok;
        case '}': tok.kind = TOK_RBRACE; advance(l); return tok;
        case '(': tok.kind = TOK_LPAREN; advance(l); return tok;
        case ')': tok.kind = TOK_RPAREN; advance(l); return tok;
        case '[': tok.kind = TOK_LBRACKET; advance(l); return tok;
        case ']': tok.kind = TOK_RBRACKET; advance(l); return tok;
        case ';': tok.kind = TOK_SEMI; advance(l); return tok;
        case ',': tok.kind = TOK_COMMA; advance(l); return tok;
        case '+': tok.kind = TOK_PLUS; advance(l); return tok;
        case '-': tok.kind = TOK_MINUS; advance(l); return tok;
        case '*': tok.kind = TOK_STAR; advance(l); return tok;
        case '/': tok.kind = TOK_SLASH; advance(l); return tok;
        case '%': tok.kind = TOK_PERCENT; advance(l); return tok;
        case '=': tok.kind = TOK_ASSIGN; advance(l); return tok;
        case '!': tok.kind = TOK_BANG; advance(l); return tok;
        case '~': tok.kind = TOK_TILDE; advance(l); return tok;
        case '&': tok.kind = TOK_AMP; advance(l); return tok;
        case '|': tok.kind = TOK_PIPE; advance(l); return tok;
        case '^': tok.kind = TOK_CARET; advance(l); return tok;
        case '<': tok.kind = TOK_LT; advance(l); return tok;
        case '>': tok.kind = TOK_GT; advance(l); return tok;
        case '.': tok.kind = TOK_DOT; advance(l); return tok;
        default: break;
    }

    tok.kind = TOK_ERROR;
    advance(l);
    return tok;
}

