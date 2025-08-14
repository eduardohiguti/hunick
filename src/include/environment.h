#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

typedef struct Object Object;

#define HASH_TABLE_SIZE 128

typedef struct EnvEntry {
    char* key;
    Object* value;
    struct EnvEntry* next;
} EnvEntry;

typedef struct Environment {
    EnvEntry** entries;
    struct Environment* outer;
} Environment;

Environment* environment_new(void);
Environment* environment_new_enclosed(Environment* outer);

Object* environment_get(Environment* env, const char* name);
Object* environment_set(Environment* env, const char* name, Object* value);
void environment_free(Environment* env);

#endif