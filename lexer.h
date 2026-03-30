#ifndef LEXER_H
#define LEXER_H

#include "ast.h"
#include "errors.h"

#define MAX_TOKENS 2048

typedef enum
{
  TOK_EOF = 0,
  TOK_INT,
  TOK_FLOAT,
  TOK_IF,
  TOK_ELSE,
  TOK_WHILE,
  TOK_ID,
  TOK_INT_LIT,
  TOK_FLOAT_LIT,
  TOK_PLUS,
  TOK_MINUS,
  TOK_STAR,
  TOK_SLASH,
  TOK_ASSIGN,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_LBRACE,
  TOK_RBRACE,
  TOK_SEMI
} TokenType;

typedef struct
{
  TokenType type;
  char lexeme[64];
  int line;
  int col;
} Token;

const char *token_type_name(TokenType t);
int lexer_tokenize_file(const char *path, Token *tokens, int max_tokens, ErrorList *errors);

#endif

void print_summary(Token *tokens, int count); // LEXER_H
