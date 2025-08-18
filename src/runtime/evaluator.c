#include "evaluator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static Object* eval_statement(Statement* stmt, Environment* env);
static Object* eval_expression(Expression* expr, Environment* env);
static Object* eval_block_statement(Statement** statements, int count, Environment* env);

static int is_truthy(Object* obj) {
    if (obj == NULL) return 0;
    switch (obj->type) {
        case OBJ_NULL:
            return 0;
        case OBJ_BOOLEAN:
            return obj->value.boolean;
        default:
            return 1;
    }
}

static Environment* extend_function_env(Object* fn, Object** args, int arg_count) {
    Environment* env = environment_new_enclosed(fn->value.function.env);

    for (int i = 0; i < fn->value.function.parameter_count; i++) {
        char* param_name = fn->value.function.parameters[i]->name;
        Object* arg_value = args[i];
        environment_set(env, param_name, arg_value);
    }
    
    return env;
}

static Object* apply_function(Object* fn, Object** args, int arg_count) {
    if (fn->type != OBJ_FUNCTION) {
        return object_new_null();
    }
    
    if (fn->value.function.parameter_count != arg_count) {
        return object_new_null();
    }
    
    Environment* extended_env = extend_function_env(fn, args, arg_count);
    Object* evaluated = eval_block_statement(fn->value.function.body, fn->value.function.body_count, extended_env);
    
    if (evaluated && evaluated->type == OBJ_RETURN_VALUE) {
        Object* unwrapped_value = evaluated->value.return_value;
        evaluated->value.return_value = NULL; 
        object_free(evaluated);
        return unwrapped_value;
    }
    
    return evaluated;
}

Object* Eval(Statement* stmt, Environment* env) {
    return eval_statement(stmt, env);
}

Object* eval_program(Program* program, Environment* env) {
    Object* result = NULL;
    for (int i = 0; i < program->statement_count; i++) {
        result = eval_statement(program->statements[i], env);
        if (result && result->type == OBJ_RETURN_VALUE) {
            return result->value.return_value;
        }
    }
    return result;
}

static Object* eval_statement(Statement* stmt, Environment* env) {
    switch (stmt->node_type) {
        case STMT_EXPRESSION:
            return eval_expression(stmt->data.expression_stmt.expression, env);
        case STMT_LET: {
            Object* val = eval_expression(stmt->data.let_stmt.value, env);
            if (val) {
                environment_set(env, stmt->data.let_stmt.name, val);
            }
            return NULL;
        }
        case STMT_RETURN: {
            Object* val = eval_expression(stmt->data.return_stmt.return_value, env);
            return object_new_return_value(val);
        }
        case STMT_BLOCK:
            return eval_block_statement(stmt->data.block_stmt.statements, stmt->data.block_stmt.statement_count, env);
        case STMT_WHILE:
            {
                Object* result = NULL;
                while (1) {
                    Object* condition = eval_expression(stmt->data.while_stmt.condition, env);
                    if (!is_truthy(condition)) {
                        // TODO: Liberar memória da condição se necessário
                        break;
                    }
                    // TODO: Liberar memória da condição se necessário
                    
                    result = eval_statement(stmt->data.while_stmt.body, env);
                    
                    if (result != NULL && result->type == OBJ_RETURN_VALUE) {
                        return result;
                    }
                    // TODO: Liberar memória do result se necessário
                }
                return NULL;
            }
        default:
            return NULL;
    }
}

static Object* eval_bang_operator_expression(Object* right) {
    if (right->type == OBJ_BOOLEAN) {
        return object_new_boolean(!right->value.boolean);
    }
    if (right->type == OBJ_NULL) {
        return object_new_boolean(1);
    }
    return object_new_boolean(0);
}

static Object* eval_minus_prefix_operator_expression(Object* right) {
    if (right->type != OBJ_INTEGER && right->type != OBJ_FLOAT) {
        return object_new_null(); 
    }
    if (right->type == OBJ_INTEGER) {
        return object_new_integer(-right->value.integer);
    }
    return object_new_null();
}

static Object* eval_prefix_expression(const char* operator, Object* right) {
    if (strcmp(operator, "!") == 0) {
        return eval_bang_operator_expression(right);
    }
    if (strcmp(operator, "-") == 0) {
        return eval_minus_prefix_operator_expression(right);
    }
    return object_new_null();
}

static Object* eval_integer_infix_expression(const char* operator, Object* left, Object* right) {
    int64_t left_val = left->value.integer;
    int64_t right_val = right->value.integer;

    if (strcmp(operator, "+") == 0) return object_new_integer(left_val + right_val);
    if (strcmp(operator, "-") == 0) return object_new_integer(left_val - right_val);
    if (strcmp(operator, "*") == 0) return object_new_integer(left_val * right_val);
    if (strcmp(operator, "/") == 0) return object_new_integer(left_val / right_val);
    if (strcmp(operator, "<") == 0) return object_new_boolean(left_val < right_val);
    if (strcmp(operator, ">") == 0) return object_new_boolean(left_val > right_val);
    if (strcmp(operator, "==") == 0) return object_new_boolean(left_val == right_val);
    if (strcmp(operator, "!=") == 0) return object_new_boolean(left_val != right_val);

    return object_new_null();
}

static Object* eval_expression(Expression* expr, Environment* env) {
    switch (expr->node_type) {
        case EXPR_INTEGER_LITERAL:
            return object_new_integer(expr->data.integer_literal.value);
        case EXPR_FLOAT_LITERAL:
            return object_new_float(expr->data.float_literal.value);
        case EXPR_STRING_LITERAL:
            return object_new_string(expr->data.string_literal.value);
        case EXPR_BOOLEAN_LITERAL:
            return object_new_boolean(expr->data.boolean_literal.value);
        case EXPR_IDENTIFIER:
            return environment_get(env, expr->data.identifier.value);
        case EXPR_PREFIX: {
            Object* right = eval_expression(expr->data.prefix.right, env);
            return eval_prefix_expression(expr->data.prefix.operator, right);
        }
        case EXPR_INFIX: {
            Object* left = eval_expression(expr->data.infix.left, env);
            Object* right = eval_expression(expr->data.infix.right, env);
            
            if (left->type == OBJ_INTEGER && right->type == OBJ_INTEGER) {
                return eval_integer_infix_expression(expr->data.infix.operator, left, right);
            }
            
            return object_new_null();
        }
        case EXPR_IF: {
            Object* condition = eval_expression(expr->data.if_expr.condition, env);
            
            if (is_truthy(condition)) {
                return eval_block_statement(expr->data.if_expr.then_branch, expr->data.if_expr.then_count, env);
            } else if (expr->data.if_expr.else_branch != NULL) {
                return eval_block_statement(expr->data.if_expr.else_branch, expr->data.if_expr.else_count, env);
            } else {
                return object_new_null();
            }
        }
        case EXPR_FUNCTION_LITERAL: {
            Parameter** params = expr->data.function_literal.parameters;
            int p_count = expr->data.function_literal.parameter_count;
            Statement** body = expr->data.function_literal.body;
            int b_count = expr->data.function_literal.body_count;
            
            return object_new_function(params, p_count, body, b_count, env);
        }
        case EXPR_CALL: {
            Object* function_obj = eval_expression(expr->data.call.function, env);

            Object** args = malloc(sizeof(Object*) * expr->data.call.argument_count);
            for (int i = 0; i < expr->data.call.argument_count; i++) {
                args[i] = eval_expression(expr->data.call.arguments[i], env);
            }
            
            Object* result = apply_function(function_obj, args, expr->data.call.argument_count);
            free(args);
            return result;
        }
        default:
            return NULL;
    }
}

static Object* eval_block_statement(Statement** statements, int count, Environment* env) {
    Object* result = NULL;
    
    Environment* enclosed_env = environment_new_enclosed(env);

    for (int i = 0; i < count; i++) {
        result = eval_statement(statements[i], enclosed_env);

        if (result != NULL && result->type == OBJ_RETURN_VALUE) {
            environment_free(enclosed_env);
            return result;
        }
    }
    
    environment_free(enclosed_env);
    return result;
}