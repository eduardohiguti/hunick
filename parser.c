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

static Type** parser_parse_type_list(Parser* parser, int* type_count) {
    *type_count = 0;
    int capacity = 10;
    Type** types = malloc(sizeof(Type*) * capacity);

    if (parser_peek_token_is(parser, TOKEN_RPAREN)) {
        parser_next_token(parser);
        return types;
    }

    parser_next_token(parser);

    types[*type_count] = parser_parse_type(parser);
    (*type_count)++;

    while (parser_peek_token_is(parser, TOKEN_COMMA)) {
        parser_next_token(parser);
        parser_next_token(parser);
        types[*type_count] = parser_parse_type(parser);
        (*type_count)++;
    }

    if (!parser_expect_peek(parser, TOKEN_RPAREN)) {
        for (int i = 0; i < *type_count; i++) type_free(types[i]);
        free(types);
        return NULL;
    }

    return types;
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
        case TOKEN_LBRACE:
            {
                Token* brace_token = parser->current_token;
                int stmt_count;
                Statement** stmts = parser_parse_block_statement(parser, &stmt_count);
                return statement_new_block(stmts, stmt_count, brace_token->line, brace_token->column);
            }  
        default:
            return parser_parse_expression_statement(parser);
    }
}

static Statement* parser_parse_let_statement(Parser* parser) {
    Token* let_token = parser->current_token;
    int is_mutable = 0;
    
    if (parser_peek_token_is(parser, TOKEN_IDENTIFIER) && strcmp(parser->peek_token->literal, "mut") == 0) {
        is_mutable = 1;
        parser_next_token(parser);
    }

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
    
    Expression* value = NULL;

    if (parser_peek_token_is(parser, TOKEN_ASSIGN)) {
        parser_next_token(parser);
        parser_next_token(parser);
        value = parser_parse_expression(parser, PRECEDENCE_LOWEST);
    }

    if (parser_peek_token_is(parser, TOKEN_SEMICOLON)) {
        parser_next_token(parser);
    }
    
    return statement_new_let(name, type, value, !is_mutable, let_token->line, let_token->column);
}

static Statement* parser_parse_const_statement(Parser* parser) {
    Token* const_token = parser->current_token;
    
    if (parser_peek_token_is(parser, TOKEN_IDENTIFIER) && strcmp(parser->peek_token->literal, "mut") == 0) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Cannot use 'mut' with 'const'. Constants are always immutable.");
        parser_add_error(parser, error_msg);
        return NULL;
    }

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
    
    return statement_new_let(name, type, value, 1, const_token->line, const_token->column);
}

static Statement* parser_parse_return_statement(Parser* parser) {
    Token* return_token = parser->current_token;
    parser_next_token(parser);
    
    Expression* return_value = parser_parse_expression(parser, PRECEDENCE_LOWEST);
    
    if (parser_peek_token_is(parser, TOKEN_SEMICOLON)) {
        parser_next_token(parser);
    }
    
    return statement_new_return(return_value, return_token->line, return_token->column);
}

static Statement* parser_parse_expression_statement(Parser* parser) {
    Token* start_token = parser->current_token;
    Expression* expr = parser_parse_expression(parser, PRECEDENCE_LOWEST);
    
    if (parser_peek_token_is(parser, TOKEN_SEMICOLON)) {
        parser_next_token(parser);
    }
    
    return statement_new_expression(expr, start_token->line, start_token->column);
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
        case TOKEN_REF:
        case TOKEN_MUT_REF:
        case TOKEN_MULTIPLY:
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
    
    while (!parser_peek_token_is(parser, TOKEN_SEMICOLON) && precedence < parser_get_precedence(parser->peek_token->type)) {
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
            case TOKEN_ASSIGN:
                parser_next_token(parser);
                left = parser_parse_infix_expression(parser, left);
                break;
            case TOKEN_PIPE:
                {
                    Token* pipe_token = parser->peek_token;
                    parser_next_token(parser);
                    parser_next_token(parser);
                    Expression* right = parser_parse_expression(parser, PRECEDENCE_PIPE);
                    left = expression_new_pipe(left, right, pipe_token->line, pipe_token->column);
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
    Token* token = parser->current_token;
    int value = (parser->current_token->type == TOKEN_BOOL_TRUE) ? 1 : 0;
    return expression_new_boolean_literal(value, token->line, token->column);
}

static Expression* parser_parse_identifier(Parser* parser) {
    Token* token = parser->current_token;
    return expression_new_identifier(string_duplicate(parser->current_token->literal), token->line, token->column);
}

static Expression* parser_parse_integer_literal(Parser* parser) {
    Token* token = parser->current_token;
    int value = atoi(parser->current_token->literal);
    return expression_new_integer_literal(value, token->line, token->column);
}

static Expression* parser_parse_float_literal(Parser* parser) {
    Token* token = parser->current_token;
    double value = atof(parser->current_token->literal);
    return expression_new_float_literal(value, token->line, token->column);
}

static Expression* parser_parse_string_literal(Parser* parser) {
    Token* token = parser->current_token;
    return expression_new_string_literal(string_duplicate(parser->current_token->literal), token->line, token->column);
}

static Expression* parser_parse_prefix_expression(Parser* parser) {
    Token* operator_token = parser->current_token;
    char* operator = string_duplicate(parser->current_token->literal);
    parser_next_token(parser);
    Expression* right = parser_parse_expression(parser, PRECEDENCE_PREFIX);
    return expression_new_prefix(operator, right, operator_token->line, operator_token->column);
}

static Expression* parser_parse_infix_expression(Parser* parser, Expression* left) {
    Token* operator_token = parser->current_token;
    char* operator = string_duplicate(parser->current_token->literal);
    Precedence precedence = parser_get_precedence(parser->current_token->type);
    parser_next_token(parser);
    Expression* right = parser_parse_expression(parser, precedence);
    return expression_new_infix(left, operator, right, operator_token->line, operator_token->column);
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
    Token* if_token = parser->current_token;
    
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
    
    return expression_new_if(condition, then_branch, then_count, else_branch, else_count, if_token->line, if_token->column);
}

static Expression* parser_parse_function_literal(Parser* parser) {
    Token* func_token = parser->current_token;
    
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
    
    return expression_new_function_literal(parameters, param_count, return_type, body, body_count, func_token->line, func_token->column);
}

static Precedence parser_get_precedence(TokenType token_type) {
    switch (token_type) {
        case TOKEN_ASSIGN: return PRECEDENCE_ASSIGN;
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
        snprintf(error_msg, sizeof(error_msg), "expected next token to be %s, got %s instead", token_type_string(token_type), token_type_string(parser->peek_token->type));
        parser_add_error(parser, error_msg);
        return 0;
    }
}

static Expression* parser_parse_call_expression(Parser* parser, Expression* function) {
    Token* lparen_token = parser->current_token;
    int arg_count;
    Expression** arguments = parser_parse_call_arguments(parser, &arg_count);
    return expression_new_call(function, arguments, arg_count, lparen_token->line, lparen_token->column);
}

static Expression* parser_parse_match_expression(Parser* parser) {
    parser_add_error(parser, "match expressions not yet implemented");
    return NULL;
}

static Type* parser_parse_type(Parser* parser) {
    if (parser_current_token_is(parser, TOKEN_FUNC)) {
        if (!parser_expect_peek(parser, TOKEN_LPAREN)) {
            return NULL;
        }

        int param_count;
        Type** param_types = parser_parse_type_list(parser, &param_count);
        if (param_types == NULL) return NULL;
        
        if (!parser_expect_peek(parser, TOKEN_ARROW)) {
            return NULL;
        }

        parser_next_token(parser);
        Type* return_type = parser_parse_type(parser);

        return type_new_function(param_types, param_count, return_type);
    }

    return type_new_identifier(string_duplicate(parser->current_token->literal));
}

static Parameter** parser_parse_function_parameters(Parser* parser, int* param_count) {
    *param_count = 0;
    int capacity = 8;
    Parameter** parameters = malloc(sizeof(Parameter*) * capacity);
    if (!parameters) return NULL;

    if (parser_peek_token_is(parser, TOKEN_RPAREN)) {
        parser_next_token(parser);
        return parameters;
    }

    parser_next_token(parser);

    if (!parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
        parser_add_error(parser, "expected parameter name");
        free(parameters);
        return NULL;
    }
    char* name = string_duplicate(parser->current_token->literal);

    if (!parser_expect_peek(parser, TOKEN_COLON)) {
        free(name);
        free(parameters);
        return NULL;
    }

    parser_next_token(parser);
    Type* type = parser_parse_type(parser);
    
    parameters[*param_count] = parameter_new(type, name);
    (*param_count)++;
    while (parser_peek_token_is(parser, TOKEN_COMMA)) {
        parser_next_token(parser);
        parser_next_token(parser);

        if (*param_count >= capacity) { 
            capacity *= 2;
            parameters = realloc(parameters, sizeof(Parameter*) * capacity);
        }

        if (!parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
            parser_add_error(parser, "expected parameter name after comma");
            for(int i = 0; i < *param_count; i++) parameter_free(parameters[i]);
            free(parameters);
            return NULL;
        }
        name = string_duplicate(parser->current_token->literal);

        if (!parser_expect_peek(parser, TOKEN_COLON)) {
            free(name);
            for(int i = 0; i < *param_count; i++) parameter_free(parameters[i]);
            free(parameters);
            return NULL;
        }

        parser_next_token(parser);
        type = parser_parse_type(parser);
        parameters[*param_count] = parameter_new(type, name);
        (*param_count)++;
    }

    if (!parser_expect_peek(parser, TOKEN_RPAREN)) {
        for(int i = 0; i < *param_count; i++) parameter_free(parameters[i]);
        free(parameters);
        return NULL;
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