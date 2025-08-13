#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokens.h"
#include "lexer.h"

void test_lexer(const char* input) {
    printf("Testing input: %s\n", input);
    printf("===========================================\n");
    
    Lexer* lexer = lexer_new(input);
    if (!lexer) {
        fprintf(stderr, "Error: Could not create lexer\n");
        return;
    }
    
    Token* token;
    do {
        token = lexer_next_token(lexer);
        if (!token) {
            fprintf(stderr, "Error: Could not get token\n");
            break;
        }
        
        printf("Type: %-15s | Literal: %-10s | Line: %d | Column: %d\n",
               token_type_string(token->type),
               token->literal,
               token->line,
               token->column);
        
        TokenType type = token->type;
        token_free(token);
        
        if (type == TOKEN_EOF) break;
        
    } while (1);
    
    lexer_free(lexer);
    printf("\n");
}

int main() {
    // Test 1: Basic function definition
    test_lexer("let add = func(int x, int y) -> int { x + y }");
    
    // Test 2: Function with pipe operator
    test_lexer("let result = 5 |> add(3) |> multiply(2)");
    
    // Test 3: Pattern matching
    test_lexer("match value { 0 -> \"zero\", 1 -> \"one\", _ -> \"other\" }");
    
    // Test 4: Conditional expression
    test_lexer("if x > 0 { \"positive\" } else { \"negative or zero\" }");
    
    // Test 5: Type definition
    test_lexer("type Point = { x: float, y: float }");
    
    // Test 6: Comparison operators
    test_lexer("x == 5 && y != 10 || z <= 20");
    
    // Test 7: String and numbers with constants
    test_lexer("const name = \"Hello, World!\"; let pi = 3.14159; let count = 42");
    
    // Test 8: Complex expression
    test_lexer("let factorial = func(int n) -> int {\n    if n <= 1 { 1 } else { n * factorial(n - 1) }\n}");
    
    return 0;
}