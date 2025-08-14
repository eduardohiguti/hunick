#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define HASH_TABLE_SIZE 128
#define MAX_ERROR_MESSAGE_LENGTH 512

static unsigned int hash_string(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_TABLE_SIZE;
}

static char* string_duplicate(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* dup = malloc(len + 1);
    if (!dup) return NULL;
    strcpy(dup, str);
    return dup;
}

TypeInfo* type_info_new_builtin(BuiltinType builtin) {
    TypeInfo* type = malloc(sizeof(TypeInfo));
    if (!type) return NULL;
    
    type->category = TYPECAT_BUILTIN;
    type->data.builtin = builtin;
    type->is_owned = 1;
    type->is_borrowed = 0;
    type->lifetime_id = 0;
    
    return type;
}

TypeInfo* type_info_new_function(TypeInfo** params, int param_count, TypeInfo* return_type) {
    TypeInfo* type = malloc(sizeof(TypeInfo));
    if (!type) return NULL;
    
    type->category = TYPECAT_FUNCTION;
    type->data.function.param_types = params;
    type->data.function.param_count = param_count;
    type->data.function.return_type = return_type;
    type->is_owned = 1;
    type->is_borrowed = 0;
    type->lifetime_id = 0;
    
    return type;
}

TypeInfo* type_info_new_struct(char* name, TypeInfo** field_types, char** field_names, int field_count) {
    TypeInfo* type = malloc(sizeof(TypeInfo));
    if (!type) return NULL;
    
    type->category = TYPECAT_STRUCT;
    type->data.struct_info.name = string_duplicate(name);
    type->data.struct_info.field_types = field_types;
    type->data.struct_info.field_names = field_names;
    type->data.struct_info.field_count = field_count;
    type->is_owned = 1;
    type->is_borrowed = 0;
    type->lifetime_id = 0;
    
    return type;
}

TypeInfo* type_info_new_reference(TypeInfo* pointed_to, int is_mutable) {
    TypeInfo* type = malloc(sizeof(TypeInfo));
    if (!type) return NULL;
    
    type->category = TYPECAT_BUILTIN;
    type->data.builtin = is_mutable ? BUILTIN_MUT_REF : BUILTIN_REF;
    type->pointed_to = pointed_to;
    
    type->is_owned = 0;
    type->is_borrowed = 1;
    type->lifetime_id = 0;
    
    return type;
}

void type_info_free(TypeInfo* type) {
    if (!type) return;
    
    switch (type->category) {
        case TYPECAT_FUNCTION:
            if (type->data.function.param_types) {
                for (int i = 0; i < type->data.function.param_count; i++) {
                    type_info_free(type->data.function.param_types[i]);
                }
                free(type->data.function.param_types);
            }
            type_info_free(type->data.function.return_type);
            break;
            
        case TYPECAT_STRUCT:
            free(type->data.struct_info.name);
            if (type->data.struct_info.field_types) {
                for (int i = 0; i < type->data.struct_info.field_count; i++) {
                    type_info_free(type->data.struct_info.field_types[i]);
                    free(type->data.struct_info.field_names[i]);
                }
                free(type->data.struct_info.field_types);
                free(type->data.struct_info.field_names);
            }
            break;
            
        default:
            break;
    }
    
    free(type);
}

int type_info_equals(TypeInfo* a, TypeInfo* b) {
    if (!a || !b) return 0;
    if (a->category != b->category) return 0;
    
    switch (a->category) {
        case TYPECAT_BUILTIN:
            return a->data.builtin == b->data.builtin;
            
        case TYPECAT_FUNCTION:
            if (a->data.function.param_count != b->data.function.param_count) return 0;
            if (!type_info_equals(a->data.function.return_type, b->data.function.return_type)) return 0;
            
            for (int i = 0; i < a->data.function.param_count; i++) {
                if (!type_info_equals(a->data.function.param_types[i], b->data.function.param_types[i])) {
                    return 0;
                }
            }
            return 1;
            
        case TYPECAT_STRUCT:
            return strcmp(a->data.struct_info.name, b->data.struct_info.name) == 0;
            
        default:
            return 0;
    }
}

int type_info_is_assignable(TypeInfo* from, TypeInfo* to) {
    return type_info_equals(from, to);
}

char* type_info_to_string(TypeInfo* type) {
    if (!type) return string_duplicate("unknown");
    
    char* result = malloc(256);
    if (!result) return NULL;
    
    switch (type->category) {
        case TYPECAT_BUILTIN:
            switch (type->data.builtin) {
                case BUILTIN_INT: strcpy(result, "int"); break;
                case BUILTIN_FLOAT: strcpy(result, "float"); break;
                case BUILTIN_STRING: strcpy(result, "string"); break;
                case BUILTIN_BOOL: strcpy(result, "bool"); break;
                case BUILTIN_UNIT: strcpy(result, "()"); break;
                case BUILTIN_UNKNOWN: strcpy(result, "unknown"); break;
                case BUILTIN_REF:
                {
                    char* pointed_to_str = type_info_to_string(type->pointed_to);
                    snprintf(result, 256, "&%s", pointed_to_str);
                    free(pointed_to_str);
                }
                break;
            case BUILTIN_MUT_REF:
                {
                    char* pointed_to_str = type_info_to_string(type->pointed_to);
                    snprintf(result, 256, "&mut %s", pointed_to_str);
                    free(pointed_to_str);
                }
                break;
            }
            break;
            
        case TYPECAT_FUNCTION:
            strcpy(result, "func(");
            for (int i = 0; i < type->data.function.param_count; i++) {
                if (i > 0) strcat(result, ", ");
                char* param_str = type_info_to_string(type->data.function.param_types[i]);
                strcat(result, param_str);
                free(param_str);
            }
            strcat(result, ") -> ");
            char* return_str = type_info_to_string(type->data.function.return_type);
            strcat(result, return_str);
            free(return_str);
            break;
            
        case TYPECAT_STRUCT:
            snprintf(result, 256, "struct %s", type->data.struct_info.name);
            break;
            
        default:
            strcpy(result, "unknown");
            break;
    }
    
    return result;
}

Symbol* symbol_new(char* name, SymbolKind kind, TypeInfo* type) {
    Symbol* symbol = malloc(sizeof(Symbol));
    if (!symbol) return NULL;
    
    symbol->name = string_duplicate(name);
    symbol->kind = kind;
    symbol->type = type;
    symbol->is_const = 0;
    symbol->is_mutable = 1;
    symbol->scope_level = 0;
    symbol->declaration_line = 0;
    symbol->first_use_line = 0;
    symbol->last_use_line = 0;
    symbol->is_initialized = 0;
    symbol->is_used = 0;
    symbol->next = NULL;
    symbol->is_mutable = (kind == SYMBOL_VARIABLE) ? 1 : 0;

    symbol->borrow_state = BORROW_STATE_NONE;
    symbol->shared_borrow_count = 0;
    symbol->borrow_lifetime_id = 0;

    symbol->next = NULL;
    return symbol;
}

void symbol_free(Symbol* symbol) {
    if (!symbol) return;
    
    free(symbol->name);
    type_info_free(symbol->type);
    free(symbol);
}

static Scope* scope_new(int level, int lifetime_id) {
    Scope* scope = malloc(sizeof(Scope));
    if (!scope) return NULL;
    
    scope->symbols = calloc(HASH_TABLE_SIZE, sizeof(Symbol*));
    scope->symbol_count = 0;
    scope->table_size = HASH_TABLE_SIZE;
    scope->scope_level = level;
    scope->parent = NULL;
    scope->next = NULL;
    scope->scope_level = level;
    scope->lifetime_id = lifetime_id;
    scope->parent = NULL;
    scope->next = NULL;
    
    return scope;
}

static void scope_free(Scope* scope) {
    if (!scope) return;
    
    for (int i = 0; i < scope->table_size; i++) {
        Symbol* symbol = scope->symbols[i];
        while (symbol) {
            Symbol* next = symbol->next;
            symbol_free(symbol);
            symbol = next;
        }
    }
    
    free(scope->symbols);
    free(scope);
}

SemanticAnalyzer* semantic_analyzer_new(void) {
    SemanticAnalyzer* analyzer = malloc(sizeof(SemanticAnalyzer));
    if (!analyzer) return NULL;
    
    analyzer->global_scope = scope_new(0, 0);
    analyzer->current_scope = analyzer->global_scope;
    analyzer->next_lifetime_id = 1;
    
    analyzer->builtin_types[BUILTIN_INT] = type_info_new_builtin(BUILTIN_INT);
    analyzer->builtin_types[BUILTIN_FLOAT] = type_info_new_builtin(BUILTIN_FLOAT);
    analyzer->builtin_types[BUILTIN_STRING] = type_info_new_builtin(BUILTIN_STRING);
    analyzer->builtin_types[BUILTIN_BOOL] = type_info_new_builtin(BUILTIN_BOOL);
    analyzer->builtin_types[BUILTIN_UNIT] = type_info_new_builtin(BUILTIN_UNIT);
    analyzer->builtin_types[BUILTIN_UNKNOWN] = type_info_new_builtin(BUILTIN_UNKNOWN);
    analyzer->builtin_types[BUILTIN_REF] = type_info_new_builtin(BUILTIN_REF);
    analyzer->builtin_types[BUILTIN_MUT_REF] = type_info_new_builtin(BUILTIN_MUT_REF);
    
    analyzer->errors = NULL;
    analyzer->error_count = 0;
    analyzer->current_function_return_type = NULL;
    analyzer->current_scope_level = 0;
    analyzer->next_lifetime_id = 1;
    
    return analyzer;
}

void semantic_analyzer_free(SemanticAnalyzer* analyzer) {
    if (!analyzer) return;
    
    while (analyzer->global_scope) {
        Scope* next = analyzer->global_scope->next;
        scope_free(analyzer->global_scope);
        analyzer->global_scope = next;
    }
    
    for (int i = 0; i < 6; i++) {
        type_info_free(analyzer->builtin_types[i]);
    }
    
    SemanticError* error = analyzer->errors;
    while (error) {
        SemanticError* next = error->next;
        free(error->message);
        free(error);
        error = next;
    }
    
    free(analyzer);
}

int symbol_table_add(SemanticAnalyzer* analyzer, Symbol* symbol) {
    if (!analyzer || !symbol) return 0;
    
    Symbol* existing = symbol_table_lookup_current_scope(analyzer, symbol->name);
    if (existing) {
        semantic_add_error(analyzer, ERROR_REDEFINITION, "Symbol already defined in current scope", symbol->declaration_line, 0);
        return 0;
    }
    
    unsigned int hash = hash_string(symbol->name);
    symbol->scope_level = analyzer->current_scope_level;
    symbol->next = analyzer->current_scope->symbols[hash];
    analyzer->current_scope->symbols[hash] = symbol;
    analyzer->current_scope->symbol_count++;
    
    return 1;
}

Symbol* symbol_table_lookup(SemanticAnalyzer* analyzer, const char* name) {
    if (!analyzer || !name) return NULL;
    
    unsigned int hash = hash_string(name);
    Scope* scope = analyzer->current_scope;
    
    while (scope) {
        Symbol* symbol = scope->symbols[hash];
        while (symbol) {
            if (strcmp(symbol->name, name) == 0) {
                return symbol;
            }
            symbol = symbol->next;
        }
        scope = scope->parent;
    }
    
    return NULL;
}

Symbol* symbol_table_lookup_current_scope(SemanticAnalyzer* analyzer, const char* name) {
    if (!analyzer || !name) return NULL;
    
    unsigned int hash = hash_string(name);
    Symbol* symbol = analyzer->current_scope->symbols[hash];
    
    while (symbol) {
        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }
    
    return NULL;
}

void semantic_push_scope(SemanticAnalyzer* analyzer) {
    if (!analyzer) return;
    
    analyzer->current_scope_level++;
    Scope* new_scope = scope_new(analyzer->current_scope_level, analyzer->next_lifetime_id++);
    new_scope->parent = analyzer->current_scope;
    analyzer->current_scope = new_scope;
}

void semantic_pop_scope(SemanticAnalyzer* analyzer) {
    if (!analyzer || !analyzer->current_scope->parent) return;
    
    release_borrows_in_scope(analyzer, analyzer->current_scope);

    Scope* old_scope = analyzer->current_scope;
    analyzer->current_scope = analyzer->current_scope->parent;
    analyzer->current_scope_level--;
    
    scope_free(old_scope);
}

void semantic_add_error(SemanticAnalyzer* analyzer, SemanticErrorType type, const char* message, int line, int column) {
    if (!analyzer) return;
    
    SemanticError* error = malloc(sizeof(SemanticError));
    error->type = type;
    error->message = string_duplicate(message);
    error->line = line;
    error->column = column;
    error->next = analyzer->errors;
    
    analyzer->errors = error;
    analyzer->error_count++;
}

void semantic_print_errors(SemanticAnalyzer* analyzer) {
    if (!analyzer) return;
    
    printf("Semantic errors (%d):\n", analyzer->error_count);
    SemanticError* error = analyzer->errors;
    while (error) {
        printf("  Line %d:%d - %s\n", error->line, error->column, error->message);
        error = error->next;
    }
}

int is_numeric_type(TypeInfo* type) {
    return type && type->category == TYPECAT_BUILTIN &&
           (type->data.builtin == BUILTIN_INT || type->data.builtin == BUILTIN_FLOAT);
}

int is_comparable_type(TypeInfo* type) {
    return type && type->category == TYPECAT_BUILTIN &&
           (type->data.builtin == BUILTIN_INT || type->data.builtin == BUILTIN_FLOAT ||
            type->data.builtin == BUILTIN_STRING || type->data.builtin == BUILTIN_BOOL);
}

TypeInfo* get_binary_operation_result_type(TypeInfo* left, TypeInfo* right, const char* operator) {
    if (!left || !right) return NULL;
    
    if (strcmp(operator, "+") == 0 || strcmp(operator, "-") == 0 ||
        strcmp(operator, "*") == 0 || strcmp(operator, "/") == 0 ||
        strcmp(operator, "%") == 0) {
        
        if (!is_numeric_type(left) || !is_numeric_type(right)) return NULL;
        
        if (left->data.builtin == BUILTIN_FLOAT || right->data.builtin == BUILTIN_FLOAT) {
            return type_info_new_builtin(BUILTIN_FLOAT);
        }
        return type_info_new_builtin(BUILTIN_INT);
    }
    
    if (strcmp(operator, "==") == 0 || strcmp(operator, "!=") == 0 ||
        strcmp(operator, "<") == 0 || strcmp(operator, ">") == 0 ||
        strcmp(operator, "<=") == 0 || strcmp(operator, ">=") == 0) {
        
        if (!is_comparable_type(left) || !is_comparable_type(right)) return NULL;
        if (!type_info_equals(left, right)) return NULL;
        
        return type_info_new_builtin(BUILTIN_BOOL);
    }
    
    if (strcmp(operator, "&&") == 0 || strcmp(operator, "||") == 0) {
        if (left->category == TYPECAT_BUILTIN && left->data.builtin == BUILTIN_BOOL &&
            right->category == TYPECAT_BUILTIN && right->data.builtin == BUILTIN_BOOL) {
            return type_info_new_builtin(BUILTIN_BOOL);
        }
        return NULL;
    }
    
    return NULL;
}

TypeInfo* convert_ast_type_to_type_info(SemanticAnalyzer* analyzer, Type* ast_type) {
    if (!ast_type || !analyzer) return analyzer->builtin_types[BUILTIN_UNKNOWN];
    
    switch (ast_type->node_type) {
        case TYPE_IDENTIFIER:
            if (strcmp(ast_type->data.identifier.name, "int") == 0) {
                return type_info_new_builtin(BUILTIN_INT);
            } else if (strcmp(ast_type->data.identifier.name, "float") == 0) {
                return type_info_new_builtin(BUILTIN_FLOAT);
            } else if (strcmp(ast_type->data.identifier.name, "string") == 0) {
                return type_info_new_builtin(BUILTIN_STRING);
            } else if (strcmp(ast_type->data.identifier.name, "bool") == 0) {
                return type_info_new_builtin(BUILTIN_BOOL);
            }
            return analyzer->builtin_types[BUILTIN_UNKNOWN];
            
        case TYPE_FUNCTION:
            {
                TypeInfo** param_types = malloc(sizeof(TypeInfo*) * ast_type->data.function.param_count);
                for (int i = 0; i < ast_type->data.function.param_count; i++) {
                    param_types[i] = convert_ast_type_to_type_info(analyzer, ast_type->data.function.params[i]);
                }
                TypeInfo* return_type = convert_ast_type_to_type_info(analyzer, ast_type->data.function.return_type);
                return type_info_new_function(param_types, ast_type->data.function.param_count, return_type);
            }
            
        default:
            return analyzer->builtin_types[BUILTIN_UNKNOWN];
    }
}

int semantic_analyze_program(SemanticAnalyzer* analyzer, Program* program) {
    if (!analyzer || !program) return 0;
    
    for (int i = 0; i < program->statement_count; i++) {
        if (!semantic_analyze_statement(analyzer, program->statements[i])) {
            return 0;
        }
    }
    
    return analyzer->error_count == 0;
}

int semantic_analyze_statement(SemanticAnalyzer* analyzer, Statement* stmt) {
    if (!analyzer || !stmt) return 0;
    
    switch (stmt->node_type) {
        case STMT_LET:
        case STMT_CONST:
            {
                TypeInfo* value_type = semantic_analyze_expression(analyzer, stmt->data.let_stmt.value);
                if (!value_type) return 0;
                
                TypeInfo* var_type;
                if (stmt->data.let_stmt.type) {
                    var_type = convert_ast_type_to_type_info(analyzer, stmt->data.let_stmt.type);
                    
                    if (!type_info_is_assignable(value_type, var_type)) {
                        char error_msg[MAX_ERROR_MESSAGE_LENGTH];
                        char* value_type_str = type_info_to_string(value_type);
                        char* var_type_str = type_info_to_string(var_type);
                        snprintf(error_msg, MAX_ERROR_MESSAGE_LENGTH, "Cannot assign value of type %s to variable of type %s", value_type_str, var_type_str);
                        free(value_type_str);
                        free(var_type_str);
                        semantic_add_error(analyzer, ERROR_TYPE_MISMATCH, error_msg, 0, 0);
                        return 0;
                    }
                } else {
                    var_type = value_type;
                }
                
                Symbol* symbol = symbol_new(stmt->data.let_stmt.name, SYMBOL_VARIABLE, var_type);

                if (stmt->data.let_stmt.value != NULL) {
                    symbol->is_initialized = 1;
                } else {
                    symbol->is_initialized = 0;
                }

                symbol->is_const = stmt->data.let_stmt.is_const;
                symbol->is_mutable = !stmt->data.let_stmt.is_const;
                symbol->is_initialized = 1;
                symbol->lifetime_id = analyzer->current_scope->lifetime_id;
                
                if (!symbol_table_add(analyzer, symbol)) {
                    symbol_free(symbol);
                    return 0;
                }
                
                return 1;
            }
            
        case STMT_RETURN:
            {
                TypeInfo* return_type = analyzer->builtin_types[BUILTIN_UNIT];
                
                if (stmt->data.return_stmt.return_value) {
                    return_type = semantic_analyze_expression(analyzer, stmt->data.return_stmt.return_value);
                    if (!return_type) return 0;
                }
                
                if (analyzer->current_function_return_type) {
                    if (!type_info_is_assignable(return_type, analyzer->current_function_return_type)) {
                        char error_msg[MAX_ERROR_MESSAGE_LENGTH];
                        char* actual_type_str = type_info_to_string(return_type);
                        char* expected_type_str = type_info_to_string(analyzer->current_function_return_type);
                        snprintf(error_msg, MAX_ERROR_MESSAGE_LENGTH, "Return type %s does not match expected type %s", actual_type_str, expected_type_str);
                        free(actual_type_str);
                        free(expected_type_str);
                        semantic_add_error(analyzer, ERROR_RETURN_TYPE_MISMATCH, error_msg, 0, 0);
                        return 0;
                    }
                }
                
                return 1;
            }
            
        case STMT_EXPRESSION:
            return semantic_analyze_expression(analyzer, stmt->data.expression_stmt.expression) != NULL;
        
        case STMT_BLOCK:
            {
                semantic_push_scope(analyzer);
                
                for (int i = 0; i < stmt->data.block_stmt.statement_count; i++) {
                    if (!semantic_analyze_statement(analyzer, stmt->data.block_stmt.statements[i])) {
                        semantic_pop_scope(analyzer);
                        return 0;
                    }
                }
                
                semantic_pop_scope(analyzer);
                return 1;
            }
        default:
            semantic_add_error(analyzer, ERROR_INVALID_OPERATION, "Unknown statement type", 0, 0);
            return 0;
    }
}

TypeInfo* semantic_analyze_expression(SemanticAnalyzer* analyzer, Expression* expr) {
    if (!analyzer || !expr) return NULL;
    
    switch (expr->node_type) {
        case EXPR_IDENTIFIER:
            {
                Symbol* symbol = symbol_table_lookup(analyzer, expr->data.identifier.value);
                if (!symbol) {
                    char error_msg[MAX_ERROR_MESSAGE_LENGTH];
                    snprintf(error_msg, MAX_ERROR_MESSAGE_LENGTH, "Undefined variable: %s", expr->data.identifier.value);
                    semantic_add_error(analyzer, ERROR_UNDEFINED_VARIABLE, error_msg, 0, 0);
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }

                if (!symbol->is_initialized) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "use of uninitialized variable '%s'", symbol->name);
                    semantic_add_error(analyzer, ERROR_UNINITIALIZED_VARIABLE, error_msg, 0, 0);
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
            
                symbol->is_used = 1;
                return symbol->type;
            }
            
        case EXPR_INTEGER_LITERAL:
            return type_info_new_builtin(BUILTIN_INT);
            
        case EXPR_FLOAT_LITERAL:
            return type_info_new_builtin(BUILTIN_FLOAT);
            
        case EXPR_STRING_LITERAL:
            return type_info_new_builtin(BUILTIN_STRING);
            
        case EXPR_BOOLEAN_LITERAL:
            return type_info_new_builtin(BUILTIN_BOOL);
            
        case EXPR_FUNCTION_LITERAL:
            {
                TypeInfo** param_types = malloc(sizeof(TypeInfo*) * expr->data.function_literal.parameter_count);
                
                semantic_push_scope(analyzer);
                
                for (int i = 0; i < expr->data.function_literal.parameter_count; i++) {
                    Parameter* param = expr->data.function_literal.parameters[i];
                    TypeInfo* param_type = convert_ast_type_to_type_info(analyzer, param->type);
                    param_types[i] = param_type;
                    
                    Symbol* param_symbol = symbol_new(param->name, SYMBOL_PARAMETER, param_type);
                    param_symbol->is_initialized = 1;
                    symbol_table_add(analyzer, param_symbol);
                }
                
                TypeInfo* return_type = analyzer->builtin_types[BUILTIN_UNIT];
                if (expr->data.function_literal.return_type) {
                    return_type = convert_ast_type_to_type_info(analyzer, expr->data.function_literal.return_type);
                }
                
                TypeInfo* old_return_type = analyzer->current_function_return_type;
                analyzer->current_function_return_type = return_type;
                
                for (int i = 0; i < expr->data.function_literal.body_count; i++) {
                    if (!semantic_analyze_statement(analyzer, expr->data.function_literal.body[i])) {
                        semantic_pop_scope(analyzer);
                        analyzer->current_function_return_type = old_return_type;
                        return analyzer->builtin_types[BUILTIN_UNKNOWN];
                    }
                }
                
                analyzer->current_function_return_type = old_return_type;
                semantic_pop_scope(analyzer);
                
                return type_info_new_function(param_types, expr->data.function_literal.parameter_count, return_type);
            }
            
        case EXPR_CALL:
            {
                TypeInfo* function_type = semantic_analyze_expression(analyzer, expr->data.call.function);
                if (!function_type || function_type->category != TYPECAT_FUNCTION) {
                    semantic_add_error(analyzer, ERROR_INVALID_OPERATION, "Cannot call non-function", 0, 0);
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                if (expr->data.call.argument_count != function_type->data.function.param_count) {
                    char error_msg[MAX_ERROR_MESSAGE_LENGTH];
                    snprintf(error_msg, MAX_ERROR_MESSAGE_LENGTH,
                            "Wrong number of arguments: expected %d, got %d",
                            function_type->data.function.param_count, expr->data.call.argument_count);
                    semantic_add_error(analyzer, ERROR_WRONG_ARGUMENT_COUNT, error_msg, 0, 0);
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                for (int i = 0; i < expr->data.call.argument_count; i++) {
                    TypeInfo* arg_type = semantic_analyze_expression(analyzer, expr->data.call.arguments[i]);
                    TypeInfo* expected_type = function_type->data.function.param_types[i];
                    
                    if (!type_info_is_assignable(arg_type, expected_type)) {
                        char error_msg[MAX_ERROR_MESSAGE_LENGTH];
                        char* arg_type_str = type_info_to_string(arg_type);
                        char* expected_type_str = type_info_to_string(expected_type);
                        snprintf(error_msg, MAX_ERROR_MESSAGE_LENGTH,
                                "Argument %d type mismatch: expected %s, got %s",
                                i + 1, expected_type_str, arg_type_str);
                        free(arg_type_str);
                        free(expected_type_str);
                        semantic_add_error(analyzer, ERROR_TYPE_MISMATCH, error_msg, 0, 0);
                        return analyzer->builtin_types[BUILTIN_UNKNOWN];
                    }
                }
                
                return function_type->data.function.return_type;
            }
            
        case EXPR_INFIX:
            {
                TypeInfo* left_type = semantic_analyze_expression(analyzer, expr->data.infix.left);
                TypeInfo* right_type = semantic_analyze_expression(analyzer, expr->data.infix.right);
                
                if (!left_type || !right_type) {
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                TypeInfo* result_type = get_binary_operation_result_type(left_type, right_type, expr->data.infix.operator);
                if (!result_type) {
                    char error_msg[MAX_ERROR_MESSAGE_LENGTH];
                    char* left_type_str = type_info_to_string(left_type);
                    char* right_type_str = type_info_to_string(right_type);
                    snprintf(error_msg, MAX_ERROR_MESSAGE_LENGTH,
                            "Invalid binary operation: %s %s %s",
                            left_type_str, expr->data.infix.operator, right_type_str);
                    free(left_type_str);
                    free(right_type_str);
                    semantic_add_error(analyzer, ERROR_INVALID_OPERATION, error_msg, 0, 0);
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                return result_type;
            }
            
        case EXPR_PREFIX:
            {
                int is_mutable_borrow = (strcmp(expr->data.prefix.operator, "&mut") == 0);
                int is_ref_op = is_mutable_borrow || (strcmp(expr->data.prefix.operator, "&") == 0);

                if (is_ref_op) {
                    if (expr->data.prefix.right->node_type != EXPR_IDENTIFIER) {
                        semantic_add_error(analyzer, ERROR_INVALID_OPERATION, "reference operator can only be used on variables", 0, 0);
                        return analyzer->builtin_types[BUILTIN_UNKNOWN];
                    }

                    char* var_name = expr->data.prefix.right->data.identifier.value;
                    Symbol* symbol = symbol_table_lookup(analyzer, var_name);
                    if (!symbol) {
                        return analyzer->builtin_types[BUILTIN_UNKNOWN];
                    }

                    if (!check_borrowing_rules(analyzer, symbol, is_mutable_borrow, 0, 0)) {
                        return analyzer->builtin_types[BUILTIN_UNKNOWN];
                    }

                    if (symbol->lifetime_id > analyzer->current_scope->lifetime_id) {
                        semantic_add_error(analyzer, ERROR_LIFETIME_VIOLATION, "borrowed value does not live long enough", 0, 0);
                        return analyzer->builtin_types[BUILTIN_UNKNOWN];
                    }

                    TypeInfo* ref_type = type_info_new_reference(symbol->type, is_mutable_borrow);
                    ref_type->lifetime_id = analyzer->current_scope->lifetime_id;
                    return ref_type;
                }
                TypeInfo* operand_type = semantic_analyze_expression(analyzer, expr->data.prefix.right);
                if (!operand_type) {
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                if (strcmp(expr->data.prefix.operator, "-") == 0) {
                    if (!is_numeric_type(operand_type)) {
                        semantic_add_error(analyzer, ERROR_INVALID_OPERATION, "Unary minus can only be applied to numeric types", 0, 0);
                        return analyzer->builtin_types[BUILTIN_UNKNOWN];
                    }
                    return operand_type;
                } else if (strcmp(expr->data.prefix.operator, "!") == 0) {
                    if (operand_type->category != TYPECAT_BUILTIN || operand_type->data.builtin != BUILTIN_BOOL) {
                        semantic_add_error(analyzer, ERROR_INVALID_OPERATION, "Logical not can only be applied to boolean types", 0, 0);
                        return analyzer->builtin_types[BUILTIN_UNKNOWN];
                    }
                    return type_info_new_builtin(BUILTIN_BOOL);
                } else if (strcmp(expr->data.prefix.operator, "*") == 0) {
                    if (operand_type->category != TYPECAT_BUILTIN || 
                       (operand_type->data.builtin != BUILTIN_REF && operand_type->data.builtin != BUILTIN_MUT_REF)) {
                        
                        char error_msg[256];
                        char* type_str = type_info_to_string(operand_type);
                        snprintf(error_msg, sizeof(error_msg), "Cannot dereference non-reference type '%s'", type_str);
                        free(type_str);
                        semantic_add_error(analyzer, ERROR_TYPE_MISMATCH, error_msg, 0, 0);
                        return analyzer->builtin_types[BUILTIN_UNKNOWN];
                    }
                    return operand_type->pointed_to;
                }
                
                semantic_add_error(analyzer, ERROR_INVALID_OPERATION, "Unknown prefix operator", 0, 0);
                return analyzer->builtin_types[BUILTIN_UNKNOWN];
            }
            
        case EXPR_IF:
            {
                TypeInfo* condition_type = semantic_analyze_expression(analyzer, expr->data.if_expr.condition);
                if (!condition_type || condition_type->category != TYPECAT_BUILTIN || 
                    condition_type->data.builtin != BUILTIN_BOOL) {
                    semantic_add_error(analyzer, ERROR_TYPE_MISMATCH,
                                     "If condition must be boolean", 0, 0);
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                semantic_push_scope(analyzer);
                TypeInfo* then_type = analyzer->builtin_types[BUILTIN_UNIT];
                
                for (int i = 0; i < expr->data.if_expr.then_count; i++) {
                    if (!semantic_analyze_statement(analyzer, expr->data.if_expr.then_branch[i])) {
                        semantic_pop_scope(analyzer);
                        return analyzer->builtin_types[BUILTIN_UNKNOWN];
                    }
                    
                    if (i == expr->data.if_expr.then_count - 1 &&
                        expr->data.if_expr.then_branch[i]->node_type == STMT_EXPRESSION) {
                        then_type = semantic_analyze_expression(analyzer, 
                            expr->data.if_expr.then_branch[i]->data.expression_stmt.expression);
                    }
                }
                semantic_pop_scope(analyzer);
                
                TypeInfo* else_type = analyzer->builtin_types[BUILTIN_UNIT];
                if (expr->data.if_expr.else_branch) {
                    semantic_push_scope(analyzer);
                    
                    for (int i = 0; i < expr->data.if_expr.else_count; i++) {
                        if (!semantic_analyze_statement(analyzer, expr->data.if_expr.else_branch[i])) {
                            semantic_pop_scope(analyzer);
                            return analyzer->builtin_types[BUILTIN_UNKNOWN];
                        }
                        
                        if (i == expr->data.if_expr.else_count - 1 &&
                            expr->data.if_expr.else_branch[i]->node_type == STMT_EXPRESSION) {
                            else_type = semantic_analyze_expression(analyzer,
                                expr->data.if_expr.else_branch[i]->data.expression_stmt.expression);
                        }
                    }
                    semantic_pop_scope(analyzer);
                }
                
                if (!type_info_equals(then_type, else_type)) {
                    char error_msg[MAX_ERROR_MESSAGE_LENGTH];
                    char* then_type_str = type_info_to_string(then_type);
                    char* else_type_str = type_info_to_string(else_type);
                    snprintf(error_msg, MAX_ERROR_MESSAGE_LENGTH,
                            "If branches have different types: %s vs %s",
                            then_type_str, else_type_str);
                    free(then_type_str);
                    free(else_type_str);
                    semantic_add_error(analyzer, ERROR_TYPE_MISMATCH, error_msg, 0, 0);
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                return then_type;
            }
            
        case EXPR_PIPE:
            {
                TypeInfo* left_type = semantic_analyze_expression(analyzer, expr->data.pipe.left);
                if (!left_type) {
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                TypeInfo* right_type = semantic_analyze_expression(analyzer, expr->data.pipe.right);
                if (!right_type || right_type->category != TYPECAT_FUNCTION) {
                    semantic_add_error(analyzer, ERROR_INVALID_OPERATION,
                                     "Right side of pipe must be a function", 0, 0);
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                if (right_type->data.function.param_count != 1) {
                    semantic_add_error(analyzer, ERROR_INVALID_OPERATION,
                                     "Piped function must take exactly one argument", 0, 0);
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                if (!type_info_is_assignable(left_type, right_type->data.function.param_types[0])) {
                    char error_msg[MAX_ERROR_MESSAGE_LENGTH];
                    char* left_type_str = type_info_to_string(left_type);
                    char* param_type_str = type_info_to_string(right_type->data.function.param_types[0]);
                    snprintf(error_msg, MAX_ERROR_MESSAGE_LENGTH,
                            "Cannot pipe %s to function expecting %s",
                            left_type_str, param_type_str);
                    free(left_type_str);
                    free(param_type_str);
                    semantic_add_error(analyzer, ERROR_TYPE_MISMATCH, error_msg, 0, 0);
                    return analyzer->builtin_types[BUILTIN_UNKNOWN];
                }
                
                return right_type->data.function.return_type;
            }
            
        case EXPR_MATCH:
            semantic_add_error(analyzer, ERROR_INVALID_OPERATION, "Match expressions not yet implemented", 0, 0);
            return analyzer->builtin_types[BUILTIN_UNKNOWN];
            
        default:
            semantic_add_error(analyzer, ERROR_INVALID_OPERATION, "Unknown expression type", 0, 0);
            return analyzer->builtin_types[BUILTIN_UNKNOWN];
    }
}

int check_borrowing_rules(SemanticAnalyzer* analyzer, Symbol* symbol, int is_mutable_borrow, int line, int col) {
    if (is_mutable_borrow) {
        if (symbol->borrow_state != BORROW_STATE_NONE) {
            char msg[MAX_ERROR_MESSAGE_LENGTH];
            snprintf(msg, sizeof(msg), "cannot borrow '%s' as mutable because it is already borrowed", symbol->name);
            semantic_add_error(analyzer, ERROR_MEMORY_SAFETY, msg, line, col);
            return 0;
        }
        if (!symbol->is_mutable) {
             char msg[MAX_ERROR_MESSAGE_LENGTH];
            snprintf(msg, sizeof(msg), "cannot mutably borrow immutable variable '%s'", symbol->name);
            semantic_add_error(analyzer, ERROR_IMMUTABLE_ASSIGNMENT, msg, line, col);
            return 0;
        }
        symbol->borrow_state = BORROW_STATE_MUTABLE;
    } else {
        if (symbol->borrow_state == BORROW_STATE_MUTABLE) {
            char msg[MAX_ERROR_MESSAGE_LENGTH];
            snprintf(msg, sizeof(msg), "cannot borrow '%s' as immutable because it is already borrowed as mutable", symbol->name);
            semantic_add_error(analyzer, ERROR_MEMORY_SAFETY, msg, line, col);
            return 0;
        }
        symbol->borrow_state = BORROW_STATE_SHARED;
        symbol->shared_borrow_count++;
    }

    symbol->borrow_lifetime_id = analyzer->current_scope->lifetime_id;
    return 1;
}

void release_borrows_in_scope(SemanticAnalyzer* analyzer, Scope* dying_scope) {
    Scope* scope_to_check = analyzer->current_scope;
    
    while (scope_to_check != NULL) {
        for (int i = 0; i < scope_to_check->table_size; i++) {
            Symbol* symbol = scope_to_check->symbols[i];
            while (symbol) {
                if (symbol->borrow_lifetime_id == dying_scope->lifetime_id) {
                    if (symbol->borrow_state == BORROW_STATE_MUTABLE) {
                        symbol->borrow_state = BORROW_STATE_NONE;
                    } else if (symbol->borrow_state == BORROW_STATE_SHARED) {
                        symbol->shared_borrow_count--;
                        if (symbol->shared_borrow_count == 0) {
                            symbol->borrow_state = BORROW_STATE_NONE;
                        }
                    }
                    symbol->borrow_lifetime_id = 0;
                }
                symbol = symbol->next;
            }
        }
        scope_to_check = scope_to_check->parent;
    }
}

int check_lifetime_safety(SemanticAnalyzer* analyzer, Expression* expr) {
    (void)analyzer; (void)expr;
    return 1;
}

int check_borrow_safety(SemanticAnalyzer* analyzer, Expression* expr) {
    (void)analyzer; (void)expr;
    return 1;
}