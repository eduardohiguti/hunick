#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char* string_duplicate(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char* dup = malloc(len + 1);
    if (!dup) return NULL;
    
    strcpy(dup, str);
    return dup;
}

static void lexer_read_char(Lexer* lexer);
static char* lexer_read_identifier(Lexer* lexer);
static char* lexer_read_number(Lexer* lexer);
static char* lexer_read_string(Lexer* lexer);
static TokenType lookup_identifier(const char* ident);
static int is_letter(char ch);
static int is_digit(char ch);

Lexer* lexer_new(const char* input) {
    Lexer* lexer = malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    
    lexer->input = string_duplicate(input);
    lexer->position = 0;
    lexer->read_position = 0;
    lexer->line = 1;
    lexer->column = 0;
    lexer->ch = 0;
    
    lexer_read_char(lexer);
    return lexer;
}

void lexer_free(Lexer* lexer) {
    if (lexer) {
        free(lexer->input);
        free(lexer);
    }
}

static void lexer_read_char(Lexer* lexer) {
    if (lexer->read_position >= (int)strlen(lexer->input)) {
        lexer->ch = 0; // EOF
    } else {
        lexer->ch = lexer->input[lexer->read_position];
    }
    
    lexer->position = lexer->read_position;
    lexer->read_position++;
    
    if (lexer->ch == '\n') {
        lexer->line++;
        lexer->column = 0;
    } else {
        lexer->column++;
    }
}

char lexer_peek_char(Lexer* lexer) {
    if (lexer->read_position >= (int)strlen(lexer->input)) {
        return 0;
    }
    return lexer->input[lexer->read_position];
}

void lexer_skip_whitespace(Lexer* lexer) {
    while (lexer->ch == ' ' || lexer->ch == '\t' || lexer->ch == '\r') {
        lexer_read_char(lexer);
    }
}

Token* lexer_next_token(Lexer* lexer) {
    Token* tok = NULL;
    
    lexer_skip_whitespace(lexer);
    
    switch (lexer->ch) {
        case '=':
            if (lexer_peek_char(lexer) == '=') {
                char ch = lexer->ch;
                lexer_read_char(lexer);
                char* literal = malloc(3);
                literal[0] = ch;
                literal[1] = lexer->ch;
                literal[2] = '\0';
                tok = token_new(TOKEN_EQUAL, literal, lexer->line, lexer->column - 1);
                free(literal);
            } else {
                tok = token_new(TOKEN_ASSIGN, "=", lexer->line, lexer->column);
            }
            break;
            
        case '+':
            tok = token_new(TOKEN_PLUS, "+", lexer->line, lexer->column);
            break;
            
        case '-':
            if (lexer_peek_char(lexer) == '>') {
                char ch = lexer->ch;
                lexer_read_char(lexer);
                char* literal = malloc(3);
                literal[0] = ch;
                literal[1] = lexer->ch;
                literal[2] = '\0';
                tok = token_new(TOKEN_ARROW, literal, lexer->line, lexer->column - 1);
                free(literal);
            } else {
                tok = token_new(TOKEN_MINUS, "-", lexer->line, lexer->column);
            }
            break;
            
        case '*':
            tok = token_new(TOKEN_MULTIPLY, "*", lexer->line, lexer->column);
            break;
            
        case '/':
            tok = token_new(TOKEN_DIVIDE, "/", lexer->line, lexer->column);
            break;
            
        case '%':
            tok = token_new(TOKEN_MODULO, "%", lexer->line, lexer->column);
            break;
            
        case '!':
            if (lexer_peek_char(lexer) == '=') {
                char ch = lexer->ch;
                lexer_read_char(lexer);
                char* literal = malloc(3);
                literal[0] = ch;
                literal[1] = lexer->ch;
                literal[2] = '\0';
                tok = token_new(TOKEN_NOT_EQUAL, literal, lexer->line, lexer->column - 1);
                free(literal);
            } else {
                tok = token_new(TOKEN_NOT, "!", lexer->line, lexer->column);
            }
            break;
            
        case '<':
            if (lexer_peek_char(lexer) == '=') {
                char ch = lexer->ch;
                lexer_read_char(lexer);
                char* literal = malloc(3);
                literal[0] = ch;
                literal[1] = lexer->ch;
                literal[2] = '\0';
                tok = token_new(TOKEN_LESS_EQUAL, literal, lexer->line, lexer->column - 1);
                free(literal);
            } else {
                tok = token_new(TOKEN_LESS_THAN, "<", lexer->line, lexer->column);
            }
            break;
            
        case '>':
            if (lexer_peek_char(lexer) == '=') {
                char ch = lexer->ch;
                lexer_read_char(lexer);
                char* literal = malloc(3);
                literal[0] = ch;
                literal[1] = lexer->ch;
                literal[2] = '\0';
                tok = token_new(TOKEN_GREATER_EQUAL, literal, lexer->line, lexer->column - 1);
                free(literal);
            } else {
                tok = token_new(TOKEN_GREATER_THAN, ">", lexer->line, lexer->column);
            }
            break;
            
        case '&':
            if (lexer_peek_char(lexer) == '&') {
                char ch = lexer->ch;
                lexer_read_char(lexer);
                char* literal = malloc(3);
                literal[0] = ch;
                literal[1] = lexer->ch;
                literal[2] = '\0';
                tok = token_new(TOKEN_AND, literal, lexer->line, lexer->column - 1);
                free(literal);
            } else {
                tok = token_new(TOKEN_ILLEGAL, "&", lexer->line, lexer->column);
            }
            break;
            
        case '|':
            if (lexer_peek_char(lexer) == '|') {
                char ch = lexer->ch;
                lexer_read_char(lexer);
                char* literal = malloc(3);
                literal[0] = ch;
                literal[1] = lexer->ch;
                literal[2] = '\0';
                tok = token_new(TOKEN_OR, literal, lexer->line, lexer->column - 1);
                free(literal);
            } else if (lexer_peek_char(lexer) == '>') {
                char ch = lexer->ch;
                lexer_read_char(lexer);
                char* literal = malloc(3);
                literal[0] = ch;
                literal[1] = lexer->ch;
                literal[2] = '\0';
                tok = token_new(TOKEN_PIPE, literal, lexer->line, lexer->column - 1);
                free(literal);
            } else {
                tok = token_new(TOKEN_ILLEGAL, "|", lexer->line, lexer->column);
            }
            break;
            
        case ';':
            tok = token_new(TOKEN_SEMICOLON, ";", lexer->line, lexer->column);
            break;
            
        case ',':
            tok = token_new(TOKEN_COMMA, ",", lexer->line, lexer->column);
            break;
            
        case ':':
            tok = token_new(TOKEN_COLON, ":", lexer->line, lexer->column);
            break;
            
        case '.':
            tok = token_new(TOKEN_DOT, ".", lexer->line, lexer->column);
            break;
            
        case '(':
            tok = token_new(TOKEN_LPAREN, "(", lexer->line, lexer->column);
            break;
            
        case ')':
            tok = token_new(TOKEN_RPAREN, ")", lexer->line, lexer->column);
            break;
            
        case '{':
            tok = token_new(TOKEN_LBRACE, "{", lexer->line, lexer->column);
            break;
            
        case '}':
            tok = token_new(TOKEN_RBRACE, "}", lexer->line, lexer->column);
            break;
            
        case '[':
            tok = token_new(TOKEN_LBRACKET, "[", lexer->line, lexer->column);
            break;
            
        case ']':
            tok = token_new(TOKEN_RBRACKET, "]", lexer->line, lexer->column);
            break;
            
        case '"':
            {
                char* literal = lexer_read_string(lexer);
                tok = token_new(TOKEN_STRING, literal, lexer->line, lexer->column - strlen(literal));
                free(literal);
            }
            break;
            
        case '\n':
            tok = token_new(TOKEN_NEWLINE, "\\n", lexer->line, lexer->column);
            break;
            
        case 0:
            tok = token_new(TOKEN_EOF, "", lexer->line, lexer->column);
            break;
            
        default:
            if (is_letter(lexer->ch)) {
                char* literal = lexer_read_identifier(lexer);
                TokenType type = lookup_identifier(literal);
                tok = token_new(type, literal, lexer->line, lexer->column - strlen(literal));
                free(literal);
                return tok; 
            } else if (is_digit(lexer->ch)) {
                char* literal = lexer_read_number(lexer);
                TokenType type = strchr(literal, '.') ? TOKEN_FLOAT : TOKEN_INTEGER;
                tok = token_new(type, literal, lexer->line, lexer->column - strlen(literal));
                free(literal);
                return tok;
            } else {
                char illegal_char[2] = {lexer->ch, '\0'};
                tok = token_new(TOKEN_ILLEGAL, illegal_char, lexer->line, lexer->column);
            }
            break;
    }
    
    lexer_read_char(lexer);
    return tok;
}

static char* lexer_read_identifier(Lexer* lexer) {
    int position = lexer->position;
    while (is_letter(lexer->ch) || is_digit(lexer->ch)) {
        lexer_read_char(lexer);
    }
    
    int length = lexer->position - position;
    char* identifier = malloc(length + 1);
    strncpy(identifier, lexer->input + position, length);
    identifier[length] = '\0';
    
    return identifier;
}

static char* lexer_read_number(Lexer* lexer) {
    int position = lexer->position;
    int has_dot = 0;
    
    while (is_digit(lexer->ch) || (lexer->ch == '.' && !has_dot)) {
        if (lexer->ch == '.') {
            has_dot = 1;
        }
        lexer_read_char(lexer);
    }
    
    int length = lexer->position - position;
    char* number = malloc(length + 1);
    strncpy(number, lexer->input + position, length);
    number[length] = '\0';
    
    return number;
}

static char* lexer_read_string(Lexer* lexer) {
    int position = lexer->position + 1;
    lexer_read_char(lexer);
    
    while (lexer->ch != '"' && lexer->ch != 0) {
        lexer_read_char(lexer);
    }
    
    int length = lexer->position - position;
    char* string = malloc(length + 1);
    strncpy(string, lexer->input + position, length);
    string[length] = '\0';
    
    return string;
}

static TokenType lookup_identifier(const char* ident) {
    struct {
        const char* keyword;
        TokenType type;
    } keywords[] = {
        {"let", TOKEN_LET},
        {"const", TOKEN_CONST},
        {"func", TOKEN_FUNC},
        {"if", TOKEN_IF},
        {"else", TOKEN_ELSE},
        {"match", TOKEN_MATCH},
        {"type", TOKEN_TYPE},
        {"return", TOKEN_RETURN},
        {"true", TOKEN_BOOL_TRUE},
        {"false", TOKEN_BOOL_FALSE},
        {"int", TOKEN_INT_TYPE},
        {"float", TOKEN_FLOAT_TYPE},
        {"string", TOKEN_STRING_TYPE},
        {"bool", TOKEN_BOOL_TYPE},
        {NULL, TOKEN_IDENTIFIER}
    };
    
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(ident, keywords[i].keyword) == 0) {
            return keywords[i].type;
        }
    }
    
    return TOKEN_IDENTIFIER;
}

static int is_letter(char ch) {
    return isalpha(ch) || ch == '_';
}

static int is_digit(char ch) {
    return isdigit(ch);
}