#include "ast.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* string_duplicate(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* dup = malloc(len + 1);
    if (!dup) return NULL;
    strcpy(dup, str);
    return dup;
}

Object* object_new_integer(int64_t value) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_INTEGER;
    obj->value.integer = value;
    return obj;
}

Object* object_new_float(double value) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_FLOAT;
    obj->value.float_val = value;
    return obj;
}

Object* object_new_boolean(int value) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_BOOLEAN;
    obj->value.boolean = value;
    return obj;
}

Object* object_new_string(const char* value) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_STRING;
    obj->value.string = string_duplicate(value);
    return obj;
}

Object* object_new_null(void) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_NULL;
    return obj;
}

Object* object_new_return_value(Object* value) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_RETURN_VALUE;
    obj->value.return_value = value;
    return obj;
}

Object* object_new_function(Parameter** params, int p_count, Statement** body, int b_count, Environment* env) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_FUNCTION;
    obj->value.function.parameters = params;
    obj->value.function.parameter_count = p_count;
    obj->value.function.body = body;
    obj->value.function.body_count = b_count;
    obj->value.function.env = env;
    return obj;
}

void object_free(Object* obj) {
    if (!obj) return;
    if (obj->type == OBJ_STRING) {
        free(obj->value.string);
    }
    free(obj);
}

void object_print(Object* obj) {
    if (!obj) {
        printf("NULL object\n");
        return;
    }
    switch (obj->type) {
        case OBJ_INTEGER: printf("%lld", (long long)obj->value.integer); break;
        case OBJ_FLOAT:   printf("%f", obj->value.float_val); break;
        case OBJ_BOOLEAN: printf("%s", obj->value.boolean ? "true" : "false"); break;
        case OBJ_STRING:  printf("\"%s\"", obj->value.string); break;
        case OBJ_NULL:    printf("null"); break;
        case OBJ_RETURN_VALUE: object_print(obj->value.return_value); break;
        case OBJ_FUNCTION:
            printf("<func(%d params)>", obj->value.function.parameter_count);
            break;
        default:          printf("Unknown object type\n"); break;
    }
}
