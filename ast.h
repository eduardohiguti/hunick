#ifndef AST_H
#define AST_H

#include "tokens.h"

typedef struct Expression Expression;
typedef struct Statement Statement;
typedef struct Program Program;

typedef enum {
    EXPR_IDENTIFIER,
    EXPR_INTEGER_LITERAL,
    EXPR_FLOAT_LITERAL,
    EXPR_STRING_LITERAL,
    EXPR_BOOLEAN_LITERAL,
    EXPR_FUNCTION_LITERAL,
    EXPR_CALL,
    EXPR_INFIX,
    EXPR_PREFIX,
    EXPR_IF,
    EXPR_MATCH,
    EXPR_PIPE,

    STMT_LET,
    STMT_CONST,
    STMT_RETURN,
    STMT_EXPRESSION,

    TYPE_IDENTIFIER,
    TYPE_FUNCTION,
    TYPE_STRUCT
} NodeType;

typedef struct Type {
    NodeType node_type;
    union {
        struct {
            char* name;
        } identifier;

        struct {
            struct Type** params;
            int param_count;
            struct Type* return_type;
        } function;

        struct {
            char** field_names;
            struct Type** field_types;
            int field_count;
        } struct_type;
    } data;
} Type;

typedef struct Parameter {
    Type* type;
    char* name;
} Parameter;

typedef struct Expression {
    NodeType node_type;
    union {
        struct {
            char* value;
        } identifier;

        struct {
            int value;
        } integer_literal;

        struct {
            double value;
        } float_literal;

        struct {
            char* value;
        } string_literal;

        struct {
            int value;
        } boolean_literal;

        struct {
            Parameter** parameters;
            int parameter_count;
            Type* return_type;
            struct Statement** body;
            int body_count;
        } function_literal;

        struct {
            Expression* function;
            Expression** arguments;
            int argument_count;
        } call;

        struct {
            Expression* left;
            char* operator;
            Expression* right;
        } infix;

        struct {
            char* operator;
            Expression* right;
        } prefix;

        struct { 
            Expression* condition;
            struct Statement** then_branch;
            int then_count;
            struct Statement** else_branch;
            int else_count;
        } if_expr;

        struct {
            Expression* expression;
            struct MatchCase** cases;
            int case_count;
        } match;

        struct {
            Expression* left;
            Expression* right;
        } pipe;
    } data;
} Expression;

typedef struct MatchCase {
    Expression* pattern;
    Expression* result;
} MatchCase;

typedef struct Statement {
    NodeType node_type;
    union {
        struct {
            char* name;
            Type* type;
            Expression* value;
            int is_const;
        } let_stmt;

        struct {
            Expression* return_value;
        } return_stmt;

        struct {
            Expression* expression;
        } expression_stmt;
    } data;
} Statement;

typedef struct Program {
    Statement** statements;
    int statement_count;
    int capacity;
} Program;

Program* program_new(void);
void program_free(Program* progam);
void program_add_statement(Program* program, Statement* stmt);

Statement* statement_new_let(char* name, Type* type, Expression* value, int is_count);
Statement* statement_new_return(Expression* return_value);
Statement* statement_new_expression(Expression* expression);
void statement_free(Statement* stmt);

Expression* expression_new_identifier(char* value);
Expression* expression_new_integer_literal(int value);
Expression* expression_new_float_literal(double value);
Expression* expression_new_string_literal(char* value);
Expression* expression_new_boolean_literal(int value);
Expression* expression_new_function_literal(Parameter** params, int param_count, Type* return_type, Statement** body, int body_count);
Expression* expression_new_call(Expression* function, Expression** arguments, int argument_count);
Expression* expression_new_infix(Expression* left, char* operator, Expression* right);
Expression* expression_new_prefix(char* operator, Expression* right);
Expression* expression_new_if(Expression* condition, Statement** then_branch, int then_count, Statement** else_branch, int else_count);
Expression* expression_new_match(Expression* expression, MatchCase** cases, int case_count);
Expression* expression_new_pipe(Expression* left, Expression* right);
void expression_free(Expression* expr);

Parameter* parameter_new(Type* type, char* name);
void parameter_free(Parameter* param);

MatchCase* match_case_new(Expression* pattern, Expression* result);
void match_case_free(MatchCase* match_case);

Type* type_new_identifier(char* name);
Type* type_new_function(Type** params, int param_count, Type* return_type);
void type_free(Type* type);

void ast_print_program(Program* program, int indent);
void ast_print_statement(Statement* stmt, int indent);
void ast_print_expression(Expression* expr, int indent);

#endif