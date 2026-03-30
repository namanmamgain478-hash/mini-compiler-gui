#include "semantic.h"

// Member 3: Semantic Analyzer (symbol table + type checks).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char name[64];
  ValType type;
  int line;
} Symbol;

typedef struct {
  Symbol symbols[256];
  size_t count;
} SymbolTable;

static void symtab_init(SymbolTable *st) {
  if (!st) return;
  st->count = 0;
}

static Symbol *symtab_find(SymbolTable *st, const char *name) {
  if (!st || !name) return NULL;
  for (size_t i = 0; i < st->count; i++) {
    if (strcmp(st->symbols[i].name, name) == 0) return &st->symbols[i];
  }
  return NULL;
}

static int symtab_add(SymbolTable *st, const char *name, ValType type, int line) {
  if (!st || !name) return 0;
  if (st->count >= 256) return 0;
  if (symtab_find(st, name)) return 0;
  Symbol *s = &st->symbols[st->count++];
  memset(s, 0, sizeof(*s));
  snprintf(s->name, sizeof(s->name), "%s", name);
  s->type = type;
  s->line = line;
  return 1;
}

static void semantic_error(ErrorList *errors, ErrorType type, int line, int col, const char *msg) {
  if (!errors) return;
  Error e;
  memset(&e, 0, sizeof(e));
  e.kind = ERR_KIND_SEMANTIC;
  e.type = type;
  e.line = line;
  e.col = col;
  snprintf(e.message, sizeof(e.message), "%s", msg ? msg : "Semantic error");
  e.suggestion[0] = '\0';
  errors_add(errors, e);
}

static ValType infer_expr_type(Expr *expr, SymbolTable *st, ErrorList *errors) {
  if (!expr) return VALTYPE_UNKNOWN;

  switch (expr->kind) {
    case EXPR_INT_LIT:
      expr->inferred_type = VALTYPE_INT;
      return VALTYPE_INT;
    case EXPR_FLOAT_LIT:
      expr->inferred_type = VALTYPE_FLOAT;
      return VALTYPE_FLOAT;
    case EXPR_ID: {
      Symbol *sym = symtab_find(st, expr->u.id);
      if (!sym) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Undeclared variable: %s", expr->u.id);
        semantic_error(errors, ERRTYPE_UNDECLARED_VAR, expr->line, 0, msg);
        expr->inferred_type = VALTYPE_UNKNOWN;
        return VALTYPE_UNKNOWN;
      }
      expr->inferred_type = sym->type;
      return sym->type;
    }
    case EXPR_NEG: {
      ValType sub = infer_expr_type(expr->u.neg.sub, st, errors);
      expr->inferred_type = sub;
      return sub;
    }
    case EXPR_BIN: {
      ValType lt = infer_expr_type(expr->u.bin.left, st, errors);
      ValType rt = infer_expr_type(expr->u.bin.right, st, errors);
      if (lt == VALTYPE_UNKNOWN || rt == VALTYPE_UNKNOWN) {
        expr->inferred_type = VALTYPE_UNKNOWN;
        return VALTYPE_UNKNOWN;
      }
      if (lt == VALTYPE_FLOAT || rt == VALTYPE_FLOAT) expr->inferred_type = VALTYPE_FLOAT;
      else expr->inferred_type = VALTYPE_INT;
      return expr->inferred_type;
    }
    default:
      expr->inferred_type = VALTYPE_UNKNOWN;
      return VALTYPE_UNKNOWN;
  }
}

static void semantic_stmt(Stmt *s, SymbolTable *st, ErrorList *errors) {
  if (!s) return;

  switch (s->kind) {
    case STMT_DECL: {
      // Add to symbol table.
      if (!symtab_add(st, s->name, s->decl_type, s->line)) {
        // For simplicity: redeclaration ignored (no error type defined).
      }
      return;
    }
    case STMT_ASSIGN: {
      Symbol *lhs = symtab_find(st, s->name);
      if (!lhs) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Undeclared variable: %s", s->name);
        semantic_error(errors, ERRTYPE_UNDECLARED_VAR, s->line, 0, msg);
        // Still infer RHS to report additional errors.
      }

      ValType rhs = infer_expr_type(s->expr, st, errors);
      if (lhs && rhs != VALTYPE_UNKNOWN) {
        if (lhs->type == VALTYPE_INT && rhs == VALTYPE_FLOAT) {
          char msg[256];
          snprintf(msg, sizeof(msg), "Type mismatch: cannot assign FLOAT to INT variable '%s'", s->name);
          semantic_error(errors, ERRTYPE_TYPE_MISMATCH, s->line, 0, msg);
        }
      }
      return;
    }
    case STMT_IF:
      (void)infer_expr_type(s->expr, st, errors);
      semantic_stmt(s->then_branch, st, errors);
      semantic_stmt(s->else_branch, st, errors);
      return;
    case STMT_WHILE:
      (void)infer_expr_type(s->expr, st, errors);
      semantic_stmt(s->body, st, errors);
      return;
    case STMT_BLOCK:
      for (size_t i = 0; i < s->block.count; i++) semantic_stmt(s->block.items[i], st, errors);
      return;
    default:
      return;
  }
}

void semantic_analyze(Stmt *program, ErrorList *errors) {
  if (!program) return;
  SymbolTable st;
  symtab_init(&st);
  semantic_stmt(program, &st, errors);
}

