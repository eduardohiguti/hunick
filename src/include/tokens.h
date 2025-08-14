#ifndef TOKENS_H
#define TOKENS_H

typedef enum {
    
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_BOOL_TRUE,
    TOKEN_BOOL_FALSE,
    
    TOKEN_LET,
    TOKEN_CONST,
    TOKEN_FUNC,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_MATCH,
    TOKEN_TYPE,
    TOKEN_RETURN,
    
    TOKEN_INT_TYPE,
    TOKEN_FLOAT_TYPE,
    TOKEN_STRING_TYPE,
    TOKEN_BOOL_TYPE,
    
    TOKEN_ASSIGN,      		
    TOKEN_PLUS,        		
    TOKEN_MINUS,       		
    TOKEN_MULTIPLY,    		
    TOKEN_DIVIDE,      		
    TOKEN_MODULO,      		
    TOKEN_EQUAL,       		
    TOKEN_NOT_EQUAL,   		
    TOKEN_LESS_THAN,   		
    TOKEN_GREATER_THAN,		
    TOKEN_LESS_EQUAL,  		
    TOKEN_GREATER_EQUAL,	
    TOKEN_AND,         		
    TOKEN_OR,          		
    TOKEN_NOT,         		
    TOKEN_ARROW,       		
    TOKEN_PIPE,  

    TOKEN_REF,
    TOKEN_MUT_REF,      		
    
    TOKEN_SEMICOLON,   		
    TOKEN_COMMA,       		
    TOKEN_COLON,       		
    TOKEN_DOT,         		
    TOKEN_LPAREN,      		
    TOKEN_RPAREN,      		
    TOKEN_LBRACE,      		
    TOKEN_RBRACE,      		
    TOKEN_LBRACKET,   		
    TOKEN_RBRACKET,    		
    
    TOKEN_NEWLINE,     		
    TOKEN_EOF,         		
    TOKEN_ILLEGAL      		
} TokenType;

typedef struct {
    TokenType type;
    char* literal;
    int line;
    int column;
    int length;
} Token;

const char* token_type_string(TokenType type);
Token* token_new(TokenType type, const char* literal, int line, int column);
void token_free(Token* token);

#endif