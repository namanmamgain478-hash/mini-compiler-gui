#ifndef IR_H
#define IR_H

#include "ast.h"
#include "errors.h"

#define MAX_IR_LINES 512

typedef struct {
  char lines[MAX_IR_LINES][256];
  size_t count;
} IRList;

void ir_init(IRList *ir);

// Generates TAC from AST. Returns 1 on success.
int ir_generate(Stmt *program, IRList *ir);

// Maps errors to friendly suggestions (simulated AI module).
void ir_apply_error_suggestions(ErrorList *errors);

#endif // IR_H

