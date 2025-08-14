#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef enum {
    PRECEDENCE_LOWEST = 1,
    PRECEDENCE_ASSIGN,
    PRECEDENCE_PIPE,       
    PRECEDENCE_OR,         
    PRECEDENCE_AND,        
    PRECEDENCE_EQUALS,     
    PRECEDENCE_LESSGREATER,
    PRECEDENCE_SUM,        
    PRECEDENCE_PRODUCT,    
    PRECEDENCE_PREFIX,     
    PRECEDENCE_CALL        
} Precedence;

typedef struct Parser {
    Lexer* lexer;
    Token* current_token;
    Token* peek_token;
    
    
    char** errors;
    int error_count;
    int error_capacity;
} Parser;

Parser* parser_new(Lexer* lexer);
void parser_free(Parser* parser);
Program* parser_parse_program(Parser* parser);

void parser_add_error(Parser* parser, const char* message);
void parser_print_errors(Parser* parser);

#endif