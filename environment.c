#include "environment.h"
#include <stdlib.h>
#include <string.h>

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

Environment* environment_new(void) {
    Environment* env = malloc(sizeof(Environment));
    if (!env) return NULL;
    
    env->entries = calloc(HASH_TABLE_SIZE, sizeof(EnvEntry*));
    env->outer = NULL;
    
    return env;
}

Environment* environment_new_enclosed(Environment* outer) {
    Environment* env = environment_new();
    env->outer = outer;
    return env;
}

void environment_free(Environment* env) {
    if (!env) return;
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        EnvEntry* entry = env->entries[i];
        while (entry) {
            EnvEntry* next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    free(env->entries);
    free(env);
}

Object* environment_get(Environment* env, const char* name) {
    if (!env || !name) return NULL;
    
    unsigned int index = hash_string(name);
    EnvEntry* entry = env->entries[index];

    while (entry) {
        if (strcmp(entry->key, name) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }

    if (env->outer != NULL) {
        return environment_get(env->outer, name);
    }
    
    return NULL;
}

Object* environment_set(Environment* env, const char* name, Object* value) {
    if (!env || !name) return NULL;

    unsigned int index = hash_string(name);

    EnvEntry* entry = env->entries[index];
    while (entry) {
        if (strcmp(entry->key, name) == 0) {
            entry->value = value;
            return value;
        }
        entry = entry->next;
    }

    EnvEntry* new_entry = malloc(sizeof(EnvEntry));
    new_entry->key = string_duplicate(name);
    new_entry->value = value;
    new_entry->next = env->entries[index];
    env->entries[index] = new_entry;

    return value;
}