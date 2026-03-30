#ifndef ERRORS_H
#define ERRORS_H

#include <stddef.h>

#define MAX_ERRORS 256

typedef enum {
  ERR_KIND_LEXICAL,
  ERR_KIND_PARSE,
  ERR_KIND_SEMANTIC
} ErrorKind;

typedef enum {
  ERRTYPE_NONE,
  // Parser-guided
  ERRTYPE_MISSING_SEMICOLON,
  // Semantic-guided
  ERRTYPE_UNDECLARED_VAR,
  ERRTYPE_TYPE_MISMATCH
} ErrorType;

typedef struct {
  ErrorKind kind;
  ErrorType type;
  int line;
  int col;
  char message[256];
  char suggestion[256]; // filled by IR/error-suggestion module
} Error;

typedef struct {
  Error items[MAX_ERRORS];
  size_t count;
} ErrorList;

static inline void errors_init(ErrorList *list) {
  if (!list) return;
  list->count = 0;
}

static inline int errors_add(ErrorList *list, Error e) { // 0 if overflow, 1 if added
  if (!list) return 0;
  if (list->count >= MAX_ERRORS) return 0;
  list->items[list->count++] = e;
  return 1;
}

#endif 

void print_errors(const ErrorList *list);// ERRORS_H

