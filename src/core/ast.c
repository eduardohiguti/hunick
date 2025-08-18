#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Program* program_new(void) {
    Program* program = malloc(sizeof(Program));
    if (!program) return NULL;
    
    program->statements = malloc(sizeof(Statement*) * 10);
    program->statement_count = 0;
    program->capacity = 10;
    
    return program;
}

void program_free(Program* program) {
    if (program) {
        for (int i = 0; i < program->statement_count; i++) {
            statement_free(program->statements[i]);
        }
        free(program->statements);
        free(program);
    }
}

void program_add_statement(Program* program, Statement* stmt) {
    if (program->statement_count >= program->capacity) {
        program->capacity *= 2;
        program->statements = realloc(program->statements, sizeof(Statement*) * program->capacity);
    }
    program->statements[program->statement_count] = stmt;
    program->statement_count++;
}

Statement* statement_new_let(char* name, Type* type, Expression* value, int is_const, int line, int column) {
    Statement* stmt = malloc(sizeof(Statement));
    if (!stmt) return NULL;
    
    stmt->node_type = STMT_LET;
    stmt->line = line;
    stmt->column = column;
    stmt->data.let_stmt.name = name;
    stmt->data.let_stmt.type = type;
    stmt->data.let_stmt.value = value;
    stmt->data.let_stmt.is_const = is_const;
    
    return stmt;
}

Statement* statement_new_return(Expression* return_value, int line, int column) {
    Statement* stmt = malloc(sizeof(Statement));
    if (!stmt) return NULL;
    
    stmt->node_type = STMT_RETURN;
    stmt->line = line;
    stmt->column = column;
    stmt->data.return_stmt.return_value = return_value;
    
    return stmt;
}

Statement* statement_new_expression(Expression* expression, int line, int column) {
    Statement* stmt = malloc(sizeof(Statement));
    if (!stmt) return NULL;
    
    stmt->node_type = STMT_EXPRESSION;
    stmt->line = line;
    stmt->column = column;
    stmt->data.expression_stmt.expression = expression;
    
    return stmt;
}

Statement* statement_new_block(Statement** statements, int statement_count, int line, int column) {
    Statement* stmt = malloc(sizeof(Statement));
    if (!stmt) return NULL;
    
    stmt->node_type = STMT_BLOCK;
    stmt->line = line;
    stmt->column = column;
    stmt->data.block_stmt.statements = statements;
    stmt->data.block_stmt.statement_count = statement_count;
    
    return stmt;
}

Statement* statement_new_while(Expression* condition, Statement* body, int line, int column) {
    Statement* stmt = malloc(sizeof(Statement));
    if (!stmt) return NULL;
    
    stmt->node_type = STMT_WHILE;
    stmt->line = line;
    stmt->column = column;
    stmt->data.while_stmt.condition = condition;
    stmt->data.while_stmt.body = body;
    
    return stmt;
}

void statement_free(Statement* stmt) {
    if (!stmt) return;
    
    switch (stmt->node_type) {
        case STMT_LET:
        case STMT_CONST:
            free(stmt->data.let_stmt.name);
            if (stmt->data.let_stmt.type) type_free(stmt->data.let_stmt.type);
            if (stmt->data.let_stmt.value) expression_free(stmt->data.let_stmt.value);
            break;
        case STMT_RETURN:
            if (stmt->data.return_stmt.return_value) 
                expression_free(stmt->data.return_stmt.return_value);
            break;
        case STMT_EXPRESSION:
            if (stmt->data.expression_stmt.expression) 
                expression_free(stmt->data.expression_stmt.expression);
            break;
        case STMT_BLOCK:
            for (int i = 0; i < stmt->data.block_stmt.statement_count; i++) {
                statement_free(stmt->data.block_stmt.statements[i]);
            }
            free(stmt->data.block_stmt.statements);
            break;
        case STMT_WHILE:
            expression_free(stmt->data.while_stmt.condition);
            statement_free(stmt->data.while_stmt.body);
            break;
        default:
            break;
    }
    free(stmt);
}

Expression* expression_new_identifier(char* value, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_IDENTIFIER;
    expr->line = line;
    expr->column = column;
    expr->data.identifier.value = value;
    
    return expr;
}

Expression* expression_new_integer_literal(int value, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_INTEGER_LITERAL;
    expr->line = line;
    expr->column = column;
    expr->data.integer_literal.value = value;
    
    return expr;
}

Expression* expression_new_float_literal(double value, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_FLOAT_LITERAL;
    expr->line = line;
    expr->column = column;
    expr->data.float_literal.value = value;
    
    return expr;
}

Expression* expression_new_string_literal(char* value, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_STRING_LITERAL;
    expr->line = line;
    expr->column = column;
    expr->data.string_literal.value = value;
    
    return expr;
}

Expression* expression_new_boolean_literal(int value, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_BOOLEAN_LITERAL;
    expr->line = line;
    expr->column = column;
    expr->data.boolean_literal.value = value;
    
    return expr;
}

Expression* expression_new_function_literal(Parameter** params, int param_count, Type* return_type, Statement** body, int body_count, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_FUNCTION_LITERAL;
    expr->line = line;
    expr->column = column;
    expr->data.function_literal.parameters = params;
    expr->data.function_literal.parameter_count = param_count;
    expr->data.function_literal.return_type = return_type;
    expr->data.function_literal.body = body;
    expr->data.function_literal.body_count = body_count;
    
    return expr;
}

Expression* expression_new_call(Expression* function, Expression** arguments, int argument_count, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_CALL;
    expr->line = line;
    expr->column = column;
    expr->data.call.function = function;
    expr->data.call.arguments = arguments;
    expr->data.call.argument_count = argument_count;
    
    return expr;
}

Expression* expression_new_infix(Expression* left, char* operator, Expression* right, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_INFIX;
    expr->line = line;
    expr->column = column;
    expr->data.infix.left = left;
    expr->data.infix.operator = operator;
    expr->data.infix.right = right;
    
    return expr;
}

Expression* expression_new_prefix(char* operator, Expression* right, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_PREFIX;
    expr->line = line;
    expr->column = column;
    expr->data.prefix.operator = operator;
    expr->data.prefix.right = right;
    
    return expr;
}

Expression* expression_new_if(Expression* condition, Statement** then_branch, int then_count, Statement** else_branch, int else_count, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_IF;
    expr->line = line;
    expr->column = column;
    expr->data.if_expr.condition = condition;
    expr->data.if_expr.then_branch = then_branch;
    expr->data.if_expr.then_count = then_count;
    expr->data.if_expr.else_branch = else_branch;
    expr->data.if_expr.else_count = else_count;
    
    return expr;
}

Expression* expression_new_match(Expression* expression, MatchCase** cases, int case_count, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_MATCH;
    expr->line = line;
    expr->column = column;
    expr->data.match.expression = expression;
    expr->data.match.cases = cases;
    expr->data.match.case_count = case_count;
    
    return expr;
}

Expression* expression_new_pipe(Expression* left, Expression* right, int line, int column) {
    Expression* expr = malloc(sizeof(Expression));
    if (!expr) return NULL;
    
    expr->node_type = EXPR_PIPE;
    expr->line = line;
    expr->column = column;
    expr->data.pipe.left = left;
    expr->data.pipe.right = right;
    
    return expr;
}

void expression_free(Expression* expr) {
    if (!expr) return;
    
    switch (expr->node_type) {
        case EXPR_IDENTIFIER:
            free(expr->data.identifier.value);
            break;
        case EXPR_STRING_LITERAL:
            free(expr->data.string_literal.value);
            break;
        case EXPR_FUNCTION_LITERAL:
            for (int i = 0; i < expr->data.function_literal.parameter_count; i++) {
                parameter_free(expr->data.function_literal.parameters[i]);
            }
            free(expr->data.function_literal.parameters);
            if (expr->data.function_literal.return_type) 
                type_free(expr->data.function_literal.return_type);
            for (int i = 0; i < expr->data.function_literal.body_count; i++) {
                statement_free(expr->data.function_literal.body[i]);
            }
            free(expr->data.function_literal.body);
            break;
        case EXPR_CALL:
            expression_free(expr->data.call.function);
            for (int i = 0; i < expr->data.call.argument_count; i++) {
                expression_free(expr->data.call.arguments[i]);
            }
            free(expr->data.call.arguments);
            break;
        case EXPR_INFIX:
            expression_free(expr->data.infix.left);
            free(expr->data.infix.operator);
            expression_free(expr->data.infix.right);
            break;
        case EXPR_PREFIX:
            free(expr->data.prefix.operator);
            expression_free(expr->data.prefix.right);
            break;
        case EXPR_IF:
            expression_free(expr->data.if_expr.condition);
            for (int i = 0; i < expr->data.if_expr.then_count; i++) {
                statement_free(expr->data.if_expr.then_branch[i]);
            }
            free(expr->data.if_expr.then_branch);
            if (expr->data.if_expr.else_branch) {
                for (int i = 0; i < expr->data.if_expr.else_count; i++) {
                    statement_free(expr->data.if_expr.else_branch[i]);
                }
                free(expr->data.if_expr.else_branch);
            }
            break;
        case EXPR_MATCH:
            expression_free(expr->data.match.expression);
            for (int i = 0; i < expr->data.match.case_count; i++) {
                match_case_free(expr->data.match.cases[i]);
            }
            free(expr->data.match.cases);
            break;
        case EXPR_PIPE:
            expression_free(expr->data.pipe.left);
            expression_free(expr->data.pipe.right);
            break;
        default:
            break;
    }
    free(expr);
}

Parameter* parameter_new(Type* type, char* name) {
    Parameter* param = malloc(sizeof(Parameter));
    if (!param) return NULL;
    
    param->type = type;
    param->name = name;
    
    return param;
}

void parameter_free(Parameter* param) {
    if (param) {
        if (param->type) type_free(param->type);
        free(param->name);
        free(param);
    }
}

MatchCase* match_case_new(Expression* pattern, Expression* result) {
    MatchCase* match_case = malloc(sizeof(MatchCase));
    if (!match_case) return NULL;
    
    match_case->pattern = pattern;
    match_case->result = result;
    
    return match_case;
}

void match_case_free(MatchCase* match_case) {
    if (match_case) {
        expression_free(match_case->pattern);
        expression_free(match_case->result);
        free(match_case);
    }
}

Type* type_new_identifier(char* name) {
    Type* type = malloc(sizeof(Type));
    if (!type) return NULL;
    
    type->node_type = TYPE_IDENTIFIER;
    type->data.identifier.name = name;
    
    return type;
}

Type* type_new_function(Type** params, int param_count, Type* return_type) {
    Type* type = malloc(sizeof(Type));
    if (!type) return NULL;
    
    type->node_type = TYPE_FUNCTION;
    type->data.function.params = params;
    type->data.function.param_count = param_count;
    type->data.function.return_type = return_type;
    
    return type;
}

void type_free(Type* type) {
    if (!type) return;
    
    switch (type->node_type) {
        case TYPE_IDENTIFIER:
            free(type->data.identifier.name);
            break;
        case TYPE_FUNCTION:
            for (int i = 0; i < type->data.function.param_count; i++) {
                type_free(type->data.function.params[i]);
            }
            free(type->data.function.params);
            type_free(type->data.function.return_type);
            break;
        case TYPE_STRUCT:
            for (int i = 0; i < type->data.struct_type.field_count; i++) {
                free(type->data.struct_type.field_names[i]);
                type_free(type->data.struct_type.field_types[i]);
            }
            free(type->data.struct_type.field_names);
            free(type->data.struct_type.field_types);
            break;
        default:
            break;
    }
    free(type);
}

void ast_print_program(Program* program, int indent) {
    printf("%*sProgram {\n", indent, "");
    for (int i = 0; i < program->statement_count; i++) {
        ast_print_statement(program->statements[i], indent + 2);
    }
    printf("%*s}\n", indent, "");
}

void ast_print_statement(Statement* stmt, int indent) {
    if (!stmt) return;
    
    switch (stmt->node_type) {
        case STMT_LET:
            printf("%*s%s %s", indent, "", 
                   stmt->data.let_stmt.is_const ? "const" : "let",
                   stmt->data.let_stmt.name);
            if (stmt->data.let_stmt.type) {
                printf(": %s", stmt->data.let_stmt.type->data.identifier.name);
            }
            printf(" = ");
            ast_print_expression(stmt->data.let_stmt.value, 0);
            printf(";\n");
            break;
        case STMT_RETURN:
            printf("%*sreturn ", indent, "");
            ast_print_expression(stmt->data.return_stmt.return_value, 0);
            printf(";\n");
            break;
        case STMT_EXPRESSION:
            printf("%*s", indent, "");
            ast_print_expression(stmt->data.expression_stmt.expression, 0);
            printf(";\n");
            break;
        default:
            printf("%*sUnknown statement\n", indent, "");
            break;
    }
}

void ast_print_expression(Expression* expr, int indent) {
    (void)indent; 
    if (!expr) return;
    
    switch (expr->node_type) {
        case EXPR_IDENTIFIER:
            printf("%s", expr->data.identifier.value);
            break;
        case EXPR_INTEGER_LITERAL:
            printf("%d", expr->data.integer_literal.value);
            break;
        case EXPR_FLOAT_LITERAL:
            printf("%f", expr->data.float_literal.value);
            break;
        case EXPR_STRING_LITERAL:
            printf("\"%s\"", expr->data.string_literal.value);
            break;
        case EXPR_BOOLEAN_LITERAL:
            printf("%s", expr->data.boolean_literal.value ? "true" : "false");
            break;
        case EXPR_FUNCTION_LITERAL:
            printf("func(");
            for (int i = 0; i < expr->data.function_literal.parameter_count; i++) {
                if (i > 0) printf(", ");
                printf("%s %s", 
                       expr->data.function_literal.parameters[i]->type->data.identifier.name,
                       expr->data.function_literal.parameters[i]->name);
            }
            printf(")");
            if (expr->data.function_literal.return_type) {
                printf(" -> %s", expr->data.function_literal.return_type->data.identifier.name);
            }
            printf(" { ... }");
            break;
        case EXPR_CALL:
            ast_print_expression(expr->data.call.function, 0);
            printf("(");
            for (int i = 0; i < expr->data.call.argument_count; i++) {
                if (i > 0) printf(", ");
                ast_print_expression(expr->data.call.arguments[i], 0);
            }
            printf(")");
            break;
        case EXPR_INFIX:
            printf("(");
            ast_print_expression(expr->data.infix.left, 0);
            printf(" %s ", expr->data.infix.operator);
            ast_print_expression(expr->data.infix.right, 0);
            printf(")");
            break;
        case EXPR_PREFIX:
            printf("(%s", expr->data.prefix.operator);
            ast_print_expression(expr->data.prefix.right, 0);
            printf(")");
            break;
        case EXPR_IF:
            printf("if (");
            ast_print_expression(expr->data.if_expr.condition, 0);
            printf(") { ... }");
            if (expr->data.if_expr.else_branch) {
                printf(" else { ... }");
            }
            break;
        case EXPR_PIPE:
            ast_print_expression(expr->data.pipe.left, 0);
            printf(" |> ");
            ast_print_expression(expr->data.pipe.right, 0);
            break;
        default:
            printf("Unknown expression");
            break;
    }
}