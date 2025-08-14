#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "ast.h"
#include "object.h"
#include "environment.h"

Object* Eval(Statement* stmt, Environment* env);
Object* eval_program(Program* program, Environment* env);

#endif