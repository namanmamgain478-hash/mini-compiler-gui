#include "parser.h"

// Member 2: Syntax Analyzer (recursive-descent parser) + grammar validation.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  Token *tokens;
  int token_count;
  int pos;
  ErrorList *errors;
} ParserCtx;

static Token *cur(ParserCtx *ctx) {
  if (ctx->pos < 0) ctx->pos = 0;
  if (ctx->pos >= ctx->token_count) return &ctx->tokens[ctx->token_count - 1];
  return &ctx->tokens[ctx->pos];
}

static int accept(ParserCtx *ctx, TokenType t) {
  if (cur(ctx)->type == t) {
    ctx->pos += 1;
    return 1;
  }
  return 0;
}

static void parser_error(ParserCtx *ctx, ErrorKind kind, ErrorType type, int line, int col,
                          const char *message) {
  if (!ctx->errors) return;
  Error e;
  memset(&e, 0, sizeof(e));
  e.kind = kind;
  e.type = type;
  e.line = line;
  e.col = col;
  snprintf(e.message, sizeof(e.message), "%s", message ? message : "Parse error");
  e.suggestion[0] = '\0';
  errors_add(ctx->errors, e);
}

static void synchronize(ParserCtx *ctx) {
  // Simple recovery: skip until ';' or '}' or start of a new statement.
  Token *t = cur(ctx);
  while (t->type != TOK_EOF && t->type != TOK_SEMI && t->type != TOK_RBRACE &&
         t->type != TOK_INT && t->type != TOK_FLOAT && t->type != TOK_IF &&
         t->type != TOK_WHILE && t->type != TOK_ID && t->type != TOK_LBRACE) {
    ctx->pos += 1;
    t = cur(ctx);
  }
  if (t->type == TOK_SEMI) ctx->pos += 1;
}

static Expr *new_expr(ExprKind kind, int line) {
  Expr *e = (Expr *)malloc(sizeof(Expr));
  if (!e) return NULL;
  memset(e, 0, sizeof(*e));
  e->kind = kind;
  e->line = line;
  e->inferred_type = VALTYPE_UNKNOWN;
  return e;
}

static Stmt *new_stmt(StmtKind kind, int line) {
  Stmt *s = (Stmt *)malloc(sizeof(Stmt));
  if (!s) return NULL;
  memset(s, 0, sizeof(*s));
  s->kind = kind;
  s->line = line;
  return s;
}

static Expr *parse_expr(ParserCtx *ctx);
static Stmt *parse_statement(ParserCtx *ctx);
static Stmt *parse_block(ParserCtx *ctx);

static Expr *parse_factor(ParserCtx *ctx) {
  Token *t = cur(ctx);
  int line = t->line;

  if (accept(ctx, TOK_MINUS)) {
    Expr *sub = parse_factor(ctx);
    Expr *e = new_expr(EXPR_NEG, line);
    if (!e) return NULL;
    e->u.neg.sub = sub;
    return e;
  }

  if (accept(ctx, TOK_LPAREN)) {
    Expr *inside = parse_expr(ctx);
    if (!accept(ctx, TOK_RPAREN)) {
      parser_error(ctx, ERR_KIND_PARSE, ERRTYPE_NONE, cur(ctx)->line, cur(ctx)->col, "Expected ')'");
      synchronize(ctx);
    }
    return inside;
  }

  if (t->type == TOK_INT_LIT) {
    ctx->pos += 1;
    Expr *e = new_expr(EXPR_INT_LIT, line);
    if (e) e->u.ival = strtoll(t->lexeme, NULL, 10);
    return e;
  }

  if (t->type == TOK_FLOAT_LIT) {
    ctx->pos += 1;
    Expr *e = new_expr(EXPR_FLOAT_LIT, line);
    if (e) e->u.fval = strtod(t->lexeme, NULL);
    return e;
  }

  if (t->type == TOK_ID) {
    ctx->pos += 1;
    Expr *e = new_expr(EXPR_ID, line);
    if (e) snprintf(e->u.id, sizeof(e->u.id), "%s", t->lexeme);
    return e;
  }

  parser_error(ctx, ERR_KIND_PARSE, ERRTYPE_NONE, t->line, t->col, "Unexpected token in expression");
  synchronize(ctx);
  return NULL;
}

static Expr *parse_term(ParserCtx *ctx) {
  Expr *left = parse_factor(ctx);
  while (1) {
    TokenType op = cur(ctx)->type;
    if (op != TOK_STAR && op != TOK_SLASH) break;

    int line = cur(ctx)->line;
    ctx->pos += 1;
    Expr *right = parse_factor(ctx);

    Expr *e = new_expr(EXPR_BIN, line);
    if (!e) return NULL;
    e->u.bin.left = left;
    e->u.bin.right = right;
    e->u.bin.op = (op == TOK_STAR) ? BIN_MUL : BIN_DIV;
    left = e;
  }
  return left;
}

static Expr *parse_expr(ParserCtx *ctx) {
  Expr *left = parse_term(ctx);
  while (1) {
    TokenType op = cur(ctx)->type;
    if (op != TOK_PLUS && op != TOK_MINUS) break;

    int line = cur(ctx)->line;
    ctx->pos += 1;
    Expr *right = parse_term(ctx);

    Expr *e = new_expr(EXPR_BIN, line);
    if (!e) return NULL;
    e->u.bin.left = left;
    e->u.bin.right = right;
    e->u.bin.op = (op == TOK_PLUS) ? BIN_ADD : BIN_SUB;
    left = e;
  }
  return left;
}

static void parse_expect(ParserCtx *ctx, TokenType expected, ErrorType errType, const char *message) {
  Token *t = cur(ctx);
  if (t->type == expected) {
    ctx->pos += 1;
    return;
  }
  parser_error(ctx, ERR_KIND_PARSE, errType, t->line, t->col, message);
  synchronize(ctx);
}

static Stmt *parse_declaration(ParserCtx *ctx) {
  Token *t = cur(ctx);
  ValType vt = (t->type == TOK_INT) ? VALTYPE_INT : VALTYPE_FLOAT;
  ctx->pos += 1;

  Token *id = cur(ctx);
  Stmt *s = new_stmt(STMT_DECL, t->line);
  if (!s) return NULL;
  s->decl_type = vt;

  if (id->type != TOK_ID) {
    parser_error(ctx, ERR_KIND_PARSE, ERRTYPE_NONE, id->line, id->col, "Expected identifier after type");
    synchronize(ctx);
    return s;
  }
  ctx->pos += 1;
  snprintf(s->name, sizeof(s->name), "%s", id->lexeme);

  parse_expect(ctx, TOK_SEMI, ERRTYPE_MISSING_SEMICOLON, "Missing semicolon ';' after declaration.");
  return s;
}

static Stmt *parse_assignment(ParserCtx *ctx) {
  Token *id = cur(ctx);
  Stmt *s = new_stmt(STMT_ASSIGN, id->line);
  if (!s) return NULL;

  snprintf(s->name, sizeof(s->name), "%s", id->lexeme);
  ctx->pos += 1;

  // '='
  if (!accept(ctx, TOK_ASSIGN)) {
    parser_error(ctx, ERR_KIND_PARSE, ERRTYPE_NONE, cur(ctx)->line, cur(ctx)->col, "Expected '=' in assignment.");
    synchronize(ctx);
  }

  s->expr = parse_expr(ctx);
  parse_expect(ctx, TOK_SEMI, ERRTYPE_MISSING_SEMICOLON, "Missing semicolon ';' after assignment.");
  return s;
}

static Stmt *parse_if(ParserCtx *ctx) {
  Token *t = cur(ctx);
  ctx->pos += 1;
  parse_expect(ctx, TOK_LPAREN, ERRTYPE_NONE, "Expected '(' after if.");
  Expr *cond = parse_expr(ctx);
  parse_expect(ctx, TOK_RPAREN, ERRTYPE_NONE, "Expected ')' after if condition.");

  Stmt *then_branch = parse_statement(ctx);

  Stmt *s = new_stmt(STMT_IF, t->line);
  if (!s) return NULL;
  s->expr = cond;
  s->then_branch = then_branch;

  if (accept(ctx, TOK_ELSE)) {
    s->else_branch = parse_statement(ctx);
  } else {
    s->else_branch = NULL;
  }
  return s;
}

static Stmt *parse_while(ParserCtx *ctx) {
  Token *t = cur(ctx);
  ctx->pos += 1;
  parse_expect(ctx, TOK_LPAREN, ERRTYPE_NONE, "Expected '(' after while.");
  Expr *cond = parse_expr(ctx);
  parse_expect(ctx, TOK_RPAREN, ERRTYPE_NONE, "Expected ')' after while condition.");

  Stmt *body = parse_statement(ctx);
  Stmt *s = new_stmt(STMT_WHILE, t->line);
  if (!s) return NULL;
  s->expr = cond;
  s->body = body;
  return s;
}

static Stmt *parse_block(ParserCtx *ctx) {
  Token *t = cur(ctx);
  if (!accept(ctx, TOK_LBRACE)) {
    parser_error(ctx, ERR_KIND_PARSE, ERRTYPE_NONE, t->line, t->col, "Expected '{' to start block.");
    synchronize(ctx);
    return NULL;
  }

  Stmt *blockStmt = new_stmt(STMT_BLOCK, t->line);
  if (!blockStmt) return NULL;
  blockStmt->block.items = NULL;
  blockStmt->block.count = 0;

  while (cur(ctx)->type != TOK_RBRACE && cur(ctx)->type != TOK_EOF) {
    Stmt *s = parse_statement(ctx);
    if (s) {
      Stmt **newItems = (Stmt **)realloc(blockStmt->block.items, (blockStmt->block.count + 1) * sizeof(Stmt *));
      if (!newItems) break;
      blockStmt->block.items = newItems;
      blockStmt->block.items[blockStmt->block.count++] = s;
    } else {
      // If parsing failed, move on to avoid infinite loops.
      if (cur(ctx)->type == TOK_SEMI) ctx->pos += 1;
      else ctx->pos += 1;
    }
  }

  parse_expect(ctx, TOK_RBRACE, ERRTYPE_NONE, "Expected '}' to close block.");
  return blockStmt;
}

static Stmt *parse_statement(ParserCtx *ctx) {
  Token *t = cur(ctx);

  if (t->type == TOK_LBRACE) return parse_block(ctx);
  if (t->type == TOK_INT || t->type == TOK_FLOAT) return parse_declaration(ctx);
  if (t->type == TOK_ID) return parse_assignment(ctx);
  if (t->type == TOK_IF) return parse_if(ctx);
  if (t->type == TOK_WHILE) return parse_while(ctx);

  parser_error(ctx, ERR_KIND_PARSE, ERRTYPE_NONE, t->line, t->col, "Unexpected token at statement level.");
  synchronize(ctx);
  return NULL;
}

Stmt *parse_program(Token *tokens, int token_count, ErrorList *errors) {
  ParserCtx ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.tokens = tokens;
  ctx.token_count = token_count;
  ctx.pos = 0;
  ctx.errors = errors;

  // root is a block without requiring braces
  Stmt *root = new_stmt(STMT_BLOCK, 1);
  if (!root) return NULL;
  root->block.items = NULL;
  root->block.count = 0;

  while (cur(&ctx)->type != TOK_EOF) {
    Stmt *s = parse_statement(&ctx);
    if (s) {
      Stmt **newItems = (Stmt **)realloc(root->block.items, (root->block.count + 1) * sizeof(Stmt *));
      if (!newItems) break;
      root->block.items = newItems;
      root->block.items[root->block.count++] = s;
    } else {
      if (cur(&ctx)->type == TOK_SEMI) ctx.pos += 1;
      else ctx.pos += 1;
    }
  }

  return root;
}

static void free_expr(Expr *e) {
  if (!e) return;
  if (e->kind == EXPR_BIN) {
    free_expr(e->u.bin.left);
    free_expr(e->u.bin.right);
  } else if (e->kind == EXPR_NEG) {
    free_expr(e->u.neg.sub);
  }
  free(e);
}

static void free_stmt(Stmt *s) {
  if (!s) return;
  switch (s->kind) {
    case STMT_DECL:
      break;
    case STMT_ASSIGN:
      free_expr(s->expr);
      break;
    case STMT_IF:
      free_expr(s->expr);
      free_stmt(s->then_branch);
      free_stmt(s->else_branch);
      break;
    case STMT_WHILE:
      free_expr(s->expr);
      free_stmt(s->body);
      break;
    case STMT_BLOCK:
      for (size_t i = 0; i < s->block.count; i++) free_stmt(s->block.items[i]);
      free(s->block.items);
      break;
    default:
      break;
  }
  free(s);
}

void parser_free_ast(Stmt *root) {
  free_stmt(root);
}

