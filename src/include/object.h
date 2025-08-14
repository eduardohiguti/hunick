#ifndef OBJECT_H
#define OBJECT_H

#include "ast.h"
#include <stdint.h>

typedef struct Environment Environment; 

typedef enum {
    OBJ_INTEGER,
    OBJ_FLOAT,
    OBJ_BOOLEAN,
    OBJ_STRING,
    OBJ_NULL,
    OBJ_RETURN_VALUE,
    OBJ_FUNCTION
} ObjectType;

typedef struct Object {
    ObjectType type;
    union {
        int64_t integer;
        double float_val;
        int boolean;
        char* string;
        struct Object* return_value;
        struct {
            Parameter** parameters;
            int parameter_count;
            Statement** body;
            int body_count;
            Environment* env;
        } function;
    } value;
} Object;

Object* object_new_integer(int64_t value);
Object* object_new_float(double value);
Object* object_new_boolean(int value);
Object* object_new_string(const char* value);
Object* object_new_null(void);
Object* object_new_return_value(Object* value);
Object* object_new_function(Parameter** params, int p_count, Statement** body, int b_count, Environment* env);
void object_free(Object* obj);
void object_print(Object* obj);

#endif
