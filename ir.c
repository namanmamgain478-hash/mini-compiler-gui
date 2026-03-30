#include "ir.h"

// Member 4: Intermediate Code Generator (TAC) + error suggestion module.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef struct {
  IRList *out;
  int temp_counter;
  int label_counter;
} IRBuilder;

static void emit(IRBuilder *b, const char *fmt, ...) {
  if (!b || !b->out) return;
  if (b->out->count >= MAX_IR_LINES) return;

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b->out->lines[b->out->count], sizeof(b->out->lines[b->out->count]), fmt, ap);
  va_end(ap);
  b->out->count += 1;
}

// We generate operands as plain strings (either literal, variable name, or temp).
typedef struct {
  char text[64];
} Operand;

static Operand op_make(const char *s) {
  Operand o;
  memset(&o, 0, sizeof(o));
  snprintf(o.text, sizeof(o.text), "%s", s ? s : "");
  return o;
}

static Operand gen_expr(IRBuilder *b, Expr *e);

static Operand new_temp(IRBuilder *b) {
  Operand o;
  memset(&o, 0, sizeof(o));
  snprintf(o.text, sizeof(o.text), "t%d", b->temp_counter++);
  return o;
}

static Operand new_label(IRBuilder *b) {
  Operand o;
  memset(&o, 0, sizeof(o));
  snprintf(o.text, sizeof(o.text), "L%d", b->label_counter++);
  return o;
}

static Operand gen_expr(IRBuilder *b, Expr *e) {
  if (!e) return op_make("0");

  switch (e->kind) {
    case EXPR_INT_LIT: {
      char buf[64];
      snprintf(buf, sizeof(buf), "%lld", e->u.ival);
      return op_make(buf);
    }
    case EXPR_FLOAT_LIT: {
      char buf[64];
      snprintf(buf, sizeof(buf), "%g", e->u.fval);
      return op_make(buf);
    }
    case EXPR_ID:
      return op_make(e->u.id);

    case EXPR_NEG: {
      Operand sub = gen_expr(b, e->u.neg.sub);
      Operand t = new_temp(b);
      emit(b, "%s = -%s", t.text, sub.text);
      return t;
    }

    case EXPR_BIN: {
      Operand left = gen_expr(b, e->u.bin.left);
      Operand right = gen_expr(b, e->u.bin.right);
      Operand t = new_temp(b);

      char op = '+';
      switch (e->u.bin.op) {
        case BIN_ADD: op = '+'; break;
        case BIN_SUB: op = '-'; break;
        case BIN_MUL: op = '*'; break;
        case BIN_DIV: op = '/'; break;
      }
      emit(b, "%s = %s %c %s", t.text, left.text, op, right.text);
      return t;
    }
    default:
      return op_make("0");
  }
}

static void gen_stmt(IRBuilder *b, Stmt *s) {
  if (!s) return;

  switch (s->kind) {
    case STMT_DECL:
      emit(b, "DECL %s %s", s->name, (s->decl_type == VALTYPE_INT) ? "int" : "float");
      return;
    case STMT_ASSIGN: {
      Operand rhs = gen_expr(b, s->expr);
      emit(b, "%s = %s", s->name, rhs.text);
      return;
    }
    case STMT_BLOCK:
      for (size_t i = 0; i < s->block.count; i++) gen_stmt(b, s->block.items[i]);
      return;
    case STMT_IF: {
      Operand cond = gen_expr(b, s->expr);
      Operand lbl_else = new_label(b);
      Operand lbl_end = new_label(b);
      emit(b, "IFZ %s GOTO %s", cond.text, lbl_else.text);
      gen_stmt(b, s->then_branch);
      if (s->else_branch) {
        emit(b, "GOTO %s", lbl_end.text);
      }
      emit(b, "LABEL %s", lbl_else.text);
      if (s->else_branch) {
        gen_stmt(b, s->else_branch);
        emit(b, "LABEL %s", lbl_end.text);
      }
      return;
    }
    case STMT_WHILE: {
      Operand lbl_start = new_label(b);
      Operand lbl_end = new_label(b);
      emit(b, "LABEL %s", lbl_start.text);
      Operand cond = gen_expr(b, s->expr);
      emit(b, "IFZ %s GOTO %s", cond.text, lbl_end.text);
      gen_stmt(b, s->body);
      emit(b, "GOTO %s", lbl_start.text);
      emit(b, "LABEL %s", lbl_end.text);
      return;
    }
    default:
      return;
  }
}

void ir_init(IRList *ir) {
  if (!ir) return;
  ir->count = 0;
}

int ir_generate(Stmt *program, IRList *ir) {
  if (!program || !ir) return 0;
  ir_init(ir);

  IRBuilder b;
  memset(&b, 0, sizeof(b));
  b.out = ir;
  b.temp_counter = 1;
  b.label_counter = 1;

  gen_stmt(&b, program);
  return 1;
}

static void suggest_for_undeclared(char *out, size_t out_sz, const char *msg) {
  // msg format: "Undeclared variable: name"
  const char *p = strstr(msg, ":");
  if (!p) {
    snprintf(out, out_sz, "Check for spelling and declare the variable before use.");
    return;
  }
  p += 1;
  while (*p == ' ') p++;
  snprintf(out, out_sz, "Declare variable '%s' before use.", p);
}

void ir_apply_error_suggestions(ErrorList *errors) {
  if (!errors) return;

  for (size_t i = 0; i < errors->count; i++) {
    Error *e = &errors->items[i];
    if (e->suggestion[0] != '\0') continue;

    if (e->kind == ERR_KIND_PARSE && e->type == ERRTYPE_MISSING_SEMICOLON) {
      // Generic hint works well for both declarations and assignments.
      snprintf(e->suggestion, sizeof(e->suggestion), "Add ';' at the end of the statement.");
    } else if (e->kind == ERR_KIND_SEMANTIC && e->type == ERRTYPE_UNDECLARED_VAR) {
      suggest_for_undeclared(e->suggestion, sizeof(e->suggestion), e->message);
    } else if (e->kind == ERR_KIND_SEMANTIC && e->type == ERRTYPE_TYPE_MISMATCH) {
      snprintf(e->suggestion, sizeof(e->suggestion), "Make sure the RHS expression type matches the LHS variable type (int vs float).");
    }
  }
}

