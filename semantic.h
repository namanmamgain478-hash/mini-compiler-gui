#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "errors.h"

// Performs semantic checks:
// - undeclared variables
// - simple type mismatch checks for assignment
void semantic_analyze(Stmt *program, ErrorList *errors);

#endif // SEMANTIC_H

