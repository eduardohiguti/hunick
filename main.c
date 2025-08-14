#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "evaluator.h"

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(fileSize + 1);
    fread(buffer, sizeof(char), fileSize, file);
    buffer[fileSize] = '\0';
    fclose(file);
    return buffer;
}

Object* eval_program(Program* program, Environment* env);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: interpreter <file_path>\n");
        return 1;
    }

    char* source = read_file(argv[1]);

    Lexer* lexer = lexer_new(source);
    Parser* parser = parser_new(lexer);
    Program* program = parser_parse_program(parser);

    if (parser->error_count > 0) {
        parser_print_errors(parser);
        return 1;
    }

    Environment* env = environment_new();
    
    Object* evaluated = eval_program(program, env);

    if (evaluated != NULL) {
        printf("=> ");
        object_print(evaluated);
        printf("\n");
        object_free(evaluated);
    }

    program_free(program);
    environment_free(env);
    free(source);

    return 0;
}