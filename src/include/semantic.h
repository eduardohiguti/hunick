#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include <stdint.h>

typedef struct SemanticAnalyzer SemanticAnalyzer;
typedef struct Symbol Symbol;
typedef struct Scope Scope;
typedef struct TypeInfo TypeInfo;

typedef enum {
    BUILTIN_INT,
    BUILTIN_FLOAT, 
    BUILTIN_STRING,
    BUILTIN_BOOL,
    BUILTIN_UNIT,      
    BUILTIN_UNKNOWN,
    BUILTIN_REF,       
    BUILTIN_MUT_REF 
} BuiltinType;

typedef enum {
    TYPECAT_BUILTIN,
    TYPECAT_FUNCTION,
    TYPECAT_STRUCT,
    TYPECAT_GENERIC,
    TYPECAT_ERROR
} TypeCategory;

typedef struct TypeInfo {
    TypeCategory category;
    union {
        BuiltinType builtin;
        struct {
            struct TypeInfo** param_types;
            int param_count;
            struct TypeInfo* return_type;
        } function;
        struct {
            char* name;
            struct TypeInfo** field_types;
            char** field_names;
            int field_count;
        } struct_info;
    } data;
    struct TypeInfo* pointed_to;
    
    int is_owned;      
    int is_borrowed;   
    int lifetime_id; 
} TypeInfo;

typedef enum {
    BORROW_STATE_NONE,
    BORROW_STATE_SHARED,    
    BORROW_STATE_MUTABLE    
} BorrowState;

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE,
    SYMBOL_PARAMETER
} SymbolKind;

typedef struct Symbol {
    char* name;
    SymbolKind kind;
    TypeInfo* type;
    int is_const;
    int is_mutable;
    
    int scope_level;
    int declaration_line;
    int first_use_line;
    int last_use_line;
    
    int is_initialized;
    int is_used;
    
    BorrowState borrow_state;
    int shared_borrow_count;
    int borrow_lifetime_id;
    int lifetime_id;
    
    struct Symbol* next; 
} Symbol;

typedef struct Scope {
    Symbol** symbols;     
    int symbol_count;
    int table_size;
    int scope_level;
    struct Scope* parent;
    struct Scope* next;
    int lifetime_id; 
} Scope;

typedef enum {
    ERROR_TYPE_MISMATCH,
    ERROR_UNDEFINED_VARIABLE,
    ERROR_UNDEFINED_FUNCTION,
    ERROR_UNDEFINED_TYPE,
    ERROR_REDEFINITION,
    ERROR_IMMUTABLE_ASSIGNMENT,
    ERROR_UNINITIALIZED_VARIABLE,
    ERROR_INVALID_OPERATION,
    ERROR_WRONG_ARGUMENT_COUNT,
    ERROR_RETURN_TYPE_MISMATCH,
    ERROR_MEMORY_SAFETY, 
    ERROR_LIFETIME_VIOLATION 
} SemanticErrorType;

typedef struct SemanticError {
    SemanticErrorType type;
    char* message;
    int line;
    int column;
    struct SemanticError* next;
} SemanticError;

typedef struct SemanticAnalyzer {
    Scope* current_scope;
    Scope* global_scope;
    
    TypeInfo* builtin_types[8]; 
    
    SemanticError* errors;
    int error_count;
    
    TypeInfo* current_function_return_type;
    
    int current_scope_level;
    
    int next_lifetime_id; 
} SemanticAnalyzer;

SemanticAnalyzer* semantic_analyzer_new(void);
void semantic_analyzer_free(SemanticAnalyzer* analyzer);

int semantic_analyze_program(SemanticAnalyzer* analyzer, Program* program);
TypeInfo* semantic_analyze_expression(SemanticAnalyzer* analyzer, Expression* expr);
int semantic_analyze_statement(SemanticAnalyzer* analyzer, Statement* stmt);

TypeInfo* type_info_new_builtin(BuiltinType builtin);
TypeInfo* type_info_new_function(TypeInfo** params, int param_count, TypeInfo* return_type);
TypeInfo* type_info_new_struct(char* name, TypeInfo** field_types, char** field_names, int field_count);
void type_info_free(TypeInfo* type);
int type_info_equals(TypeInfo* a, TypeInfo* b);
int type_info_is_assignable(TypeInfo* from, TypeInfo* to);
char* type_info_to_string(TypeInfo* type);

Symbol* symbol_new(char* name, SymbolKind kind, TypeInfo* type);
void symbol_free(Symbol* symbol);
int symbol_table_add(SemanticAnalyzer* analyzer, Symbol* symbol);
Symbol* symbol_table_lookup(SemanticAnalyzer* analyzer, const char* name);
Symbol* symbol_table_lookup_current_scope(SemanticAnalyzer* analyzer, const char* name);

void semantic_push_scope(SemanticAnalyzer* analyzer);
void semantic_pop_scope(SemanticAnalyzer* analyzer);

void semantic_add_error(SemanticAnalyzer* analyzer, SemanticErrorType type, const char* message, int line, int column);
void semantic_print_errors(SemanticAnalyzer* analyzer);

TypeInfo* convert_ast_type_to_type_info(SemanticAnalyzer* analyzer, Type* ast_type);
int is_numeric_type(TypeInfo* type);
int is_comparable_type(TypeInfo* type);
TypeInfo* get_binary_operation_result_type(TypeInfo* left, TypeInfo* right, const char* operator);

int check_lifetime_safety(SemanticAnalyzer* analyzer, Expression* expr);
int check_borrow_safety(SemanticAnalyzer* analyzer, Expression* expr);
int check_borrowing_rules(SemanticAnalyzer* analyzer, Symbol* symbol, int is_mutable_borrow, int line, int col);
void release_borrows_in_scope(SemanticAnalyzer* analyzer, Scope* scope);

#endif