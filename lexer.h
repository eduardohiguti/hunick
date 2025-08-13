#ifndef LEXER_H
#define LEXER_H

#include "tokens.h"

typedef struct {
    char* input;
    int position;
    int read_position;
    char ch;
    int line;
    int column;
} Lexer;

Lexer* lexer_new(const char* input);
void lexer_free(Lexer* lexer);
Token* lexer_next_token(Lexer* lexer);
void lexer_skip_whitespace(Lexer* lexer);
char lexer_peek_char(Lexer* lexer);

#endif