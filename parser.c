#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void parser_next_token(Parser* parser);
static Statement* parser_parse_statement(Parser* parser);
static Statement* parser_parse_let_statement(Parser* parser);
static Statement* parser_parse_const_statement(Parser* parser);
static Statement* parser_parse_return_statement(Parser* parser);
static Statement* parser_parse_expression_statement(Parser* parser);
static Expression* parser_parse_expression(Parser* parser, Precedence precedence);
static Expression* parser_parse_prefix_expression(Parser* parser);
static Expression* parser_parse_infix_expression(Parser* parser, Expression* left);
static Expression* parser_parse_identifier(Parser* parser);
static Expression* parser_parse_integer_literal(Parser* parser);
static Expression* parser_parse_float_literal(Parser* parser);
static Expression* parser_parse_string_literal(Parser* parser);
static Expression* parser_parse_boolean_literal(Parser* parser);
static Expression* parser_parse_grouped_expression(Parser* parser);
static Expression* parser_parse_if_expression(Parser* parser);
static Expression* parser_parse_function_literal(Parser* parser);
static Expression* parser_parse_call_expression(Parser* parser, Expression* function);
static Expression* parser_parse_match_expression(Parser* parser);
static Type* parser_parse_type(Parser* parser);
static Parameter** parser_parse_function_parameters(Parser* parser, int* param_count);
static Expression** parser_parse_call_arguments(Parser* parser, int* arg_count);
static Statement** parser_parse_block_statement(Parser* parser, int* stmt_count);
static Precedence parser_get_precedence(TokenType token_type);
static int parser_current_token_is(Parser* parser, TokenType token_type);
static int parser_peek_token_is(Parser* parser, TokenType token_type);
static int parser_expect_peek(Parser* parser, TokenType token_type);


static char* string_duplicate(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char* dup = malloc(len + 1);
    if (!dup) return NULL;
    
    strcpy(dup, str);
    return dup;
}

Parser* parser_new(Lexer* lexer) {
    Parser* parser = malloc(sizeof(Parser));
    if (!parser) return NULL;
    
    parser->lexer = lexer;
    parser->current_token = NULL;
    parser->peek_token = NULL;
    parser->errors = malloc(sizeof(char*) * 10);
    parser->error_count = 0;
    parser->error_capacity = 10;
    
    
    parser_next_token(parser);
    parser_next_token(parser);
    
    return parser;
}

void parser_free(Parser* parser) {
    if (parser) {
        if (parser->current_token) token_free(parser->current_token);
        if (parser->peek_token) token_free(parser->peek_token);
        
        for (int i = 0; i < parser->error_count; i++) {
            free(parser->errors[i]);
        }
        free(parser->errors);
        free(parser);
    }
}

static void parser_next_token(Parser* parser) {
    if (parser->current_token) {
        token_free(parser->current_token);
    }
    parser->current_token = parser->peek_token;
    parser->peek_token = lexer_next_token(parser->lexer);
}

void parser_add_error(Parser* parser, const char* message) {
    if (parser->error_count >= parser->error_capacity) {
        parser->error_capacity *= 2;
        parser->errors = realloc(parser->errors, sizeof(char*) * parser->error_capacity);
    }
    
    parser->errors[parser->error_count] = string_duplicate(message);
    parser->error_count++;
}

void parser_print_errors(Parser* parser) {
    printf("Parser errors:\n");
    for (int i = 0; i < parser->error_count; i++) {
        printf("  %s\n", parser->errors[i]);
    }
}

Program* parser_parse_program(Parser* parser) {
    Program* program = program_new();
    if (!program) return NULL;
    
    while (parser->current_token->type != TOKEN_EOF) {
        
        if (parser->current_token->type == TOKEN_NEWLINE) {
            parser_next_token(parser);
            continue;
        }
        
        Statement* stmt = parser_parse_statement(parser);
        if (stmt) {
            program_add_statement(program, stmt);
        }
        parser_next_token(parser);
    }
    
    return program;
}

static Statement* parser_parse_statement(Parser* parser) {
    switch (parser->current_token->type) {
        case TOKEN_LET:
            return parser_parse_let_statement(parser);
        case TOKEN_CONST:
            return parser_parse_const_statement(parser);
        case TOKEN_RETURN:
            return parser_parse_return_statement(parser);
        default:
            return parser_parse_expression_statement(parser);
    }
}

static Statement* parser_parse_let_statement(Parser* parser) {
    if (!parser_expect_peek(parser, TOKEN_IDENTIFIER)) {
        return NULL;
    }
    
    char* name = string_duplicate(parser->current_token->literal);
    Type* type = NULL;
    
    
    if (parser_peek_token_is(parser, TOKEN_COLON)) {
        parser_next_token(parser); 
        parser_next_token(parser); 
        type = parser_parse_type(parser);
    }
    
    if (!parser_expect_peek(parser, TOKEN_ASSIGN)) {
        free(name);
        if (type) type_free(type);
        return NULL;
    }
    
    parser_next_token(parser);
    Expression* value = parser_parse_expression(parser, PRECEDENCE_LOWEST);
    
    if (parser_peek_token_is(parser, TOKEN_SEMICOLON)) {
        parser_next_token(parser);
    }
    
    return statement_new_let(name, type, value, 0); 
}

static Statement* parser_parse_const_statement(Parser* parser) {
    if (!parser_expect_peek(parser, TOKEN_IDENTIFIER)) {
        return NULL;
    }
    
    char* name = string_duplicate(parser->current_token->literal);
    Type* type = NULL;
    
    
    if (parser_peek_token_is(parser, TOKEN_COLON)) {
        parser_next_token(parser); 
        parser_next_token(parser); 
        type = parser_parse_type(parser);
    }
    
    if (!parser_expect_peek(parser, TOKEN_ASSIGN)) {
        free(name);
        if (type) type_free(type);
        return NULL;
    }
    
    parser_next_token(parser);
    Expression* value = parser_parse_expression(parser, PRECEDENCE_LOWEST);
    
    if (parser_peek_token_is(parser, TOKEN_SEMICOLON)) {
        parser_next_token(parser);
    }
    
    return statement_new_let(name, type, value, 1); 
}

static Statement* parser_parse_return_statement(Parser* parser) {
    parser_next_token(parser);
    
    Expression* return_value = parser_parse_expression(parser, PRECEDENCE_LOWEST);
    
    if (parser_peek_token_is(parser, TOKEN_SEMICOLON)) {
        parser_next_token(parser);
    }
    
    return statement_new_return(return_value);
}

static Statement* parser_parse_expression_statement(Parser* parser) {
    Expression* expr = parser_parse_expression(parser, PRECEDENCE_LOWEST);
    
    if (parser_peek_token_is(parser, TOKEN_SEMICOLON)) {
        parser_next_token(parser);
    }
    
    return statement_new_expression(expr);
}

static Expression* parser_parse_expression(Parser* parser, Precedence precedence) {
    
    Expression* left = NULL;
    
    switch (parser->current_token->type) {
        case TOKEN_IDENTIFIER:
            left = parser_parse_identifier(parser);
            break;
        case TOKEN_INTEGER:
            left = parser_parse_integer_literal(parser);
            break;
        case TOKEN_FLOAT:
            left = parser_parse_float_literal(parser);
            break;
        case TOKEN_STRING:
            left = parser_parse_string_literal(parser);
            break;
        case TOKEN_BOOL_TRUE:
        case TOKEN_BOOL_FALSE:
            left = parser_parse_boolean_literal(parser);
            break;
        case TOKEN_NOT:
        case TOKEN_MINUS:
            left = parser_parse_prefix_expression(parser);
            break;
        case TOKEN_LPAREN:
            left = parser_parse_grouped_expression(parser);
            break;
        case TOKEN_IF:
            left = parser_parse_if_expression(parser);
            break;
        case TOKEN_FUNC:
            left = parser_parse_function_literal(parser);
            break;
        case TOKEN_MATCH:
            left = parser_parse_match_expression(parser);
            break;
        default:
            {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "no prefix parse function for %s found", 
                        token_type_string(parser->current_token->type));
                parser_add_error(parser, error_msg);
                return NULL;
            }
    }
    
    
    while (!parser_peek_token_is(parser, TOKEN_SEMICOLON) && 
           precedence < parser_get_precedence(parser->peek_token->type)) {
        
        switch (parser->peek_token->type) {
            case TOKEN_PLUS:
            case TOKEN_MINUS:
            case TOKEN_DIVIDE:
            case TOKEN_MULTIPLY:
            case TOKEN_MODULO:
            case TOKEN_EQUAL:
            case TOKEN_NOT_EQUAL:
            case TOKEN_LESS_THAN:
            case TOKEN_GREATER_THAN:
            case TOKEN_LESS_EQUAL:
            case TOKEN_GREATER_EQUAL:
            case TOKEN_AND:
            case TOKEN_OR:
                parser_next_token(parser);
                left = parser_parse_infix_expression(parser, left);
                break;
            case TOKEN_PIPE:
                parser_next_token(parser);
                parser_next_token(parser);
                {
                    Expression* right = parser_parse_expression(parser, PRECEDENCE_PIPE);
                    left = expression_new_pipe(left, right);
                }
                break;
            case TOKEN_LPAREN:
                parser_next_token(parser);
                left = parser_parse_call_expression(parser, left);
                break;
            default:
                return left;
        }
    }
    
    return left;
}


static Expression* parser_parse_boolean_literal(Parser* parser) {
    int value = (parser->current_token->type == TOKEN_BOOL_TRUE) ? 1 : 0;
    return expression_new_boolean_literal(value);
}

static Expression* parser_parse_identifier(Parser* parser) {
    return expression_new_identifier(string_duplicate(parser->current_token->literal));
}

static Expression* parser_parse_integer_literal(Parser* parser) {
    int value = atoi(parser->current_token->literal);
    return expression_new_integer_literal(value);
}

static Expression* parser_parse_float_literal(Parser* parser) {
    double value = atof(parser->current_token->literal);
    return expression_new_float_literal(value);
}

static Expression* parser_parse_string_literal(Parser* parser) {
    return expression_new_string_literal(string_duplicate(parser->current_token->literal));
}

static Expression* parser_parse_prefix_expression(Parser* parser) {
    char* operator = string_duplicate(parser->current_token->literal);
    parser_next_token(parser);
    Expression* right = parser_parse_expression(parser, PRECEDENCE_PREFIX);
    return expression_new_prefix(operator, right);
}

static Expression* parser_parse_infix_expression(Parser* parser, Expression* left) {
    char* operator = string_duplicate(parser->current_token->literal);
    Precedence precedence = parser_get_precedence(parser->current_token->type);
    parser_next_token(parser);
    Expression* right = parser_parse_expression(parser, precedence);
    return expression_new_infix(left, operator, right);
}

static Expression* parser_parse_grouped_expression(Parser* parser) {
    parser_next_token(parser);
    Expression* expr = parser_parse_expression(parser, PRECEDENCE_LOWEST);
    
    if (!parser_expect_peek(parser, TOKEN_RPAREN)) {
        return NULL;
    }
    
    return expr;
}

static Expression* parser_parse_if_expression(Parser* parser) {
    if (!parser_expect_peek(parser, TOKEN_LPAREN)) {
        return NULL;
    }
    
    parser_next_token(parser);
    Expression* condition = parser_parse_expression(parser, PRECEDENCE_LOWEST);
    
    if (!parser_expect_peek(parser, TOKEN_RPAREN)) {
        return NULL;
    }
    
    if (!parser_expect_peek(parser, TOKEN_LBRACE)) {
        return NULL;
    }
    
    int then_count;
    Statement** then_branch = parser_parse_block_statement(parser, &then_count);
    
    Statement** else_branch = NULL;
    int else_count = 0;
    
    if (parser_peek_token_is(parser, TOKEN_ELSE)) {
        parser_next_token(parser);
        if (!parser_expect_peek(parser, TOKEN_LBRACE)) {
            return NULL;
        }
        else_branch = parser_parse_block_statement(parser, &else_count);
    }
    
    return expression_new_if(condition, then_branch, then_count, else_branch, else_count);
}

static Expression* parser_parse_function_literal(Parser* parser) {
    if (!parser_expect_peek(parser, TOKEN_LPAREN)) {
        return NULL;
    }
    
    int param_count;
    Parameter** parameters = parser_parse_function_parameters(parser, &param_count);
    
    Type* return_type = NULL;
    if (parser_peek_token_is(parser, TOKEN_ARROW)) {
        parser_next_token(parser); 
        parser_next_token(parser); 
        return_type = parser_parse_type(parser);
    }
    
    if (!parser_expect_peek(parser, TOKEN_LBRACE)) {
        return NULL;
    }
    
    int body_count;
    Statement** body = parser_parse_block_statement(parser, &body_count);
    
    return expression_new_function_literal(parameters, param_count, return_type, body, body_count);
}


static Precedence parser_get_precedence(TokenType token_type) {
    switch (token_type) {
        case TOKEN_PIPE: return PRECEDENCE_PIPE;
        case TOKEN_OR: return PRECEDENCE_OR;
        case TOKEN_AND: return PRECEDENCE_AND;
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL: return PRECEDENCE_EQUALS;
        case TOKEN_LESS_THAN:
        case TOKEN_GREATER_THAN:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER_EQUAL: return PRECEDENCE_LESSGREATER;
        case TOKEN_PLUS:
        case TOKEN_MINUS: return PRECEDENCE_SUM;
        case TOKEN_DIVIDE:
        case TOKEN_MULTIPLY:
        case TOKEN_MODULO: return PRECEDENCE_PRODUCT;
        case TOKEN_LPAREN: return PRECEDENCE_CALL;
        default: return PRECEDENCE_LOWEST;
    }
}

static int parser_current_token_is(Parser* parser, TokenType token_type) {
    return parser->current_token->type == token_type;
}

static int parser_peek_token_is(Parser* parser, TokenType token_type) {
    return parser->peek_token->type == token_type;
}

static int parser_expect_peek(Parser* parser, TokenType token_type) {
    if (parser_peek_token_is(parser, token_type)) {
        parser_next_token(parser);
        return 1;
    } else {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "expected next token to be %s, got %s instead",
                token_type_string(token_type), token_type_string(parser->peek_token->type));
        parser_add_error(parser, error_msg);
        return 0;
    }
}


static Expression* parser_parse_call_expression(Parser* parser, Expression* function) {
    int arg_count;
    Expression** arguments = parser_parse_call_arguments(parser, &arg_count);
    return expression_new_call(function, arguments, arg_count);
}

static Expression* parser_parse_match_expression(Parser* parser) {
    
    parser_add_error(parser, "match expressions not yet implemented");
    return NULL;
}

static Type* parser_parse_type(Parser* parser) {
    return type_new_identifier(string_duplicate(parser->current_token->literal));
}

static Parameter** parser_parse_function_parameters(Parser* parser, int* param_count) {
    Parameter** parameters = malloc(sizeof(Parameter*) * 10);
    *param_count = 0;
    
    if (parser_peek_token_is(parser, TOKEN_RPAREN)) {
        parser_next_token(parser);
        return parameters;
    }
    
    parser_next_token(parser);
    
    do {
        
        Type* type = parser_parse_type(parser);
        if (!parser_expect_peek(parser, TOKEN_IDENTIFIER)) {
            return parameters;
        }
        char* name = string_duplicate(parser->current_token->literal);
        
        parameters[*param_count] = parameter_new(type, name);
        (*param_count)++;
        
        if (parser_peek_token_is(parser, TOKEN_COMMA)) {
            parser_next_token(parser);
            parser_next_token(parser);
        }
    } while (!parser_peek_token_is(parser, TOKEN_RPAREN));
    
    if (!parser_expect_peek(parser, TOKEN_RPAREN)) {
        return parameters;
    }
    
    return parameters;
}

static Expression** parser_parse_call_arguments(Parser* parser, int* arg_count) {
    Expression** arguments = malloc(sizeof(Expression*) * 10);
    *arg_count = 0;
    
    if (parser_peek_token_is(parser, TOKEN_RPAREN)) {
        parser_next_token(parser);
        return arguments;
    }
    
    parser_next_token(parser);
    arguments[*arg_count] = parser_parse_expression(parser, PRECEDENCE_LOWEST);
    (*arg_count)++;
    
    while (parser_peek_token_is(parser, TOKEN_COMMA)) {
        parser_next_token(parser);
        parser_next_token(parser);
        arguments[*arg_count] = parser_parse_expression(parser, PRECEDENCE_LOWEST);
        (*arg_count)++;
    }
    
    if (!parser_expect_peek(parser, TOKEN_RPAREN)) {
        return arguments;
    }
    
    return arguments;
}

static Statement** parser_parse_block_statement(Parser* parser, int* stmt_count) {
    Statement** statements = malloc(sizeof(Statement*) * 20);
    *stmt_count = 0;
    
    parser_next_token(parser);
    
    while (!parser_current_token_is(parser, TOKEN_RBRACE) && 
           !parser_current_token_is(parser, TOKEN_EOF)) {
        
        
        if (parser->current_token->type == TOKEN_NEWLINE) {
            parser_next_token(parser);
            continue;
        }
        
        Statement* stmt = parser_parse_statement(parser);
        if (stmt) {
            statements[*stmt_count] = stmt;
            (*stmt_count)++;
        }
        parser_next_token(parser);
    }
    
    return statements;
}