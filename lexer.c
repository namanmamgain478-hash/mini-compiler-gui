#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void lex_error(ErrorList *errors, int line, int col, const char *msg)
{
  if (!errors)
    return;
  Error e;
  memset(&e, 0, sizeof(e));
  e.kind = ERR_KIND_LEXICAL;
  e.line = line;
  e.col = col;
  snprintf(e.message, sizeof(e.message), "%s", msg);
  errors_add(errors, e);
}

const char *token_type_name(TokenType t)
{
  switch (t)
  {
  case TOK_EOF:
    return "EOF";
  case TOK_INT:
    return "KEYWORD";
  case TOK_FLOAT:
    return "KEYWORD";
  case TOK_IF:
    return "KEYWORD";
  case TOK_ELSE:
    return "KEYWORD";
  case TOK_WHILE:
    return "KEYWORD";
  case TOK_ID:
    return "IDENTIFIER";
  case TOK_INT_LIT:
    return "NUMBER";
  case TOK_FLOAT_LIT:
    return "NUMBER";
  case TOK_PLUS:
    return "OPERATOR";
  case TOK_MINUS:
    return "OPERATOR";
  case TOK_STAR:
    return "OPERATOR";
  case TOK_SLASH:
    return "OPERATOR";
  case TOK_ASSIGN:
    return "OPERATOR";
  default:
    return "UNKNOWN";
  }
}

static int is_id_start(int c)
{
  return (isalpha(c) || c == '_');
}

static int is_id_part(int c)
{
  return (isalnum(c) || c == '_');
}

static int add_token(Token *tokens, int max_tokens, int *count,
                     TokenType type, const char *lexeme,
                     int line, int col)
{
  if (*count >= max_tokens)
    return 0;

  Token *t = &tokens[*count];
  t->type = type;
  strcpy(t->lexeme, lexeme);
  t->line = line;
  t->col = col;

  (*count)++;
  return 1;
}

int lexer_tokenize_file(const char *path, Token *tokens, int max_tokens, ErrorList *errors)
{
  FILE *f = fopen(path, "r");
  if (!f)
  {
    printf("Error: Cannot open file\n");
    return 0;
  }

  int ch, line = 1, col = 1;
  int count = 0;

  while ((ch = fgetc(f)) != EOF)
  {

    // Skip spaces
    if (isspace(ch))
    {
      if (ch == '\n')
      {
        line++;
        col = 1;
      }
      else
        col++;
      continue;
    }

    // IDENTIFIER / KEYWORD
    if (is_id_start(ch))
    {
      char buf[50];
      int i = 0;
      int start_col = col;

      do
      {
        buf[i++] = ch;
        ch = fgetc(f);
        col++;
      } while (is_id_part(ch));

      buf[i] = '\0';
      ungetc(ch, f);

      TokenType type = TOK_ID;

      if (strcmp(buf, "int") == 0)
        type = TOK_INT;
      else if (strcmp(buf, "float") == 0)
        type = TOK_FLOAT;
      else if (strcmp(buf, "if") == 0)
        type = TOK_IF;
      else if (strcmp(buf, "else") == 0)
        type = TOK_ELSE;
      else if (strcmp(buf, "while") == 0)
        type = TOK_WHILE;

      add_token(tokens, max_tokens, &count, type, buf, line, start_col);
      continue;
    }

    // NUMBER
    if (isdigit(ch))
    {
      char buf[50];
      int i = 0;
      int start_col = col;
      int dot = 0;

      do
      {
        if (ch == '.')
          dot++;
        buf[i++] = ch;
        ch = fgetc(f);
        col++;
      } while (isdigit(ch) || ch == '.');

      buf[i] = '\0';
      ungetc(ch, f);

      TokenType type = (dot == 0) ? TOK_INT_LIT : TOK_FLOAT_LIT;
      add_token(tokens, max_tokens, &count, type, buf, line, start_col);
      continue;
    }

    // OPERATORS
    char op[2] = {ch, '\0'};
    int start_col = col;

    switch (ch)
    {
    case '+':
      add_token(tokens, max_tokens, &count, TOK_PLUS, op, line, col);
      break;
    case '-':
      add_token(tokens, max_tokens, &count, TOK_MINUS, op, line, col);
      break;
    case '*':
      add_token(tokens, max_tokens, &count, TOK_STAR, op, line, col);
      break;
    case '/':
      add_token(tokens, max_tokens, &count, TOK_SLASH, op, line, col);
      break;
    case '=':
      add_token(tokens, max_tokens, &count, TOK_ASSIGN, op, line, col);
      break;
    case ';':
      add_token(tokens, max_tokens, &count, TOK_SEMI, op, line, col);
      break;
    case '(':
      add_token(tokens, max_tokens, &count, TOK_LPAREN, op, line, col);
      break;
    case ')':
      add_token(tokens, max_tokens, &count, TOK_RPAREN, op, line, col);
      break;
    case '{':
      add_token(tokens, max_tokens, &count, TOK_LBRACE, op, line, col);
      break;
    case '}':
      add_token(tokens, max_tokens, &count, TOK_RBRACE, op, line, col);
      break;
    default:
      lex_error(errors, line, start_col, "Unknown character");
    }

    col++;
  }

  add_token(tokens, max_tokens, &count, TOK_EOF, "EOF", line, col);
  fclose(f);

  return count;
}
void print_summary(Token *tokens, int count)
{

  char kw[100][50];
  int k = 0;
  char id[100][50];
  int i_c = 0;
  char num[100][50];
  int n = 0;
  char op[100][10];
  int o = 0;

  for (int i = 0; i < count; i++)
  {
    Token t = tokens[i];

    if (t.type == TOK_EOF)
      continue;

    if (t.type == TOK_INT || t.type == TOK_FLOAT ||
        t.type == TOK_IF || t.type == TOK_ELSE ||
        t.type == TOK_WHILE)
    {

      int found = 0;
      for (int j = 0; j < k; j++)
        if (strcmp(kw[j], t.lexeme) == 0)
          found = 1;

      if (!found)
        strcpy(kw[k++], t.lexeme);
    }

    else if (t.type == TOK_ID)
    {
      int found = 0;
      for (int j = 0; j < i_c; j++)
        if (strcmp(id[j], t.lexeme) == 0)
          found = 1;

      if (!found)
        strcpy(id[i_c++], t.lexeme);
    }

    else if (t.type == TOK_INT_LIT || t.type == TOK_FLOAT_LIT)
    {
      int found = 0;
      for (int j = 0; j < n; j++)
        if (strcmp(num[j], t.lexeme) == 0)
          found = 1;

      if (!found)
        strcpy(num[n++], t.lexeme);
    }

    else
    {
      int found = 0;
      for (int j = 0; j < o; j++)
        if (strcmp(op[j], t.lexeme) == 0)
          found = 1;

      if (!found)
        strcpy(op[o++], t.lexeme);
    }
  }

  printf("\n===== SIMPLE OUTPUT =====\n");

  printf("Keywords: ");
  for (int i = 0; i < k; i++)
    printf("%s ", kw[i]);

  printf("\nIdentifiers: ");
  for (int i = 0; i < i_c; i++)
    printf("%s ", id[i]);

  printf("\nNumbers: ");
  for (int i = 0; i < n; i++)
    printf("%s ", num[i]);

  printf("\nOperators: ");
  for (int i = 0; i < o; i++)
    printf("%s ", op[i]);

  printf("\n=========================\n");
}