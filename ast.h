#ifndef AST_H
#define AST_H

#include <stddef.h>

// ======= AST node definitions (used by parser, semantic, and IR) =======

typedef enum {
  VALTYPE_INT,
  VALTYPE_FLOAT,
  VALTYPE_UNKNOWN
} ValType;

typedef enum {
  EXPR_INT_LIT,
  EXPR_FLOAT_LIT,
  EXPR_ID,
  EXPR_BIN,
  EXPR_NEG
} ExprKind;

typedef enum {
  BIN_ADD,
  BIN_SUB,
  BIN_MUL,
  BIN_DIV
} BinOp;

typedef struct Expr Expr;
struct Expr {
  ExprKind kind;
  int line;
  ValType inferred_type; // filled by semantic analyzer
  union {
    long long ival;
    double fval;
    char id[64];
    struct {
      BinOp op;
      Expr *left;
      Expr *right;
    } bin;
    struct {
      Expr *sub;
    } neg;
  } u;
};

typedef enum {
  STMT_DECL,
  STMT_ASSIGN,
  STMT_IF,
  STMT_WHILE,
  STMT_BLOCK
} StmtKind;

typedef struct Stmt Stmt;
struct Stmt {
  StmtKind kind;
  int line;
  ValType decl_type; // for STMT_DECL

  // for STMT_DECL and STMT_ASSIGN
  char name[64];

  // for STMT_ASSIGN (value) and STMT_IF/STMT_WHILE (condition)
  Expr *expr;

  // for STMT_IF
  Stmt *then_branch;
  Stmt *else_branch;

  // for STMT_WHILE
  Stmt *body;

  // for STMT_BLOCK
  struct {
    Stmt **items;
    size_t count;
  } block;
};

#endif // AST_H

