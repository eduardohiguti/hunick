#ifndef TOKENS_H
#define TOKENS_H

typedef enum {
	// Literals
	TOKEN_IDENTIFIER,
	TOKEN_INTEGER,
	TOKEN_FLOAT,
	TOKEN_STRING,
	TOKEN_BOOL_TRUE,
	TOKEN_BOOL_FALSE,
	
	// Keywords
	TOKEN_LET,
	TOKEN_FN,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_MATCH,
	TOKEN_TYPE,
	TOKEN_RETURN,

	// Types
	TOKEN_INT_TYPE,
	TOKEN_FLOAT_TYPE,
	TOKEN_STRING_TYPE,
	TOKEN_BOOL_TYPE,

	// Operators
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

	// Delimiters
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

	// Special
	TOKEN_NEWLINE,
	TOKEN_EOF,
	TOKEN_ILLEGAL,
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