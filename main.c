#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// main.c: Integrates all modules and prints tokens, validation, IR, and errors.

#include "errors.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

static int count_errors_by_kind(const ErrorList *list, ErrorKind kind) {
  if (!list) return 0;
  int c = 0;
  for (size_t i = 0; i < list->count; i++) {
    if (list->items[i].kind == kind) c++;
  }
  return c;
}

int main(int argc, char **argv) {
  const char *input_path = (argc >= 2) ? argv[1] : "sample_ok.txt";

  Token tokens[MAX_TOKENS];
  ErrorList errors;

  // Proper init:
  errors_init(&errors);

  int token_count = lexer_tokenize_file(input_path, tokens, MAX_TOKENS, &errors);
  if (token_count <= 0) {
    printf("Failed to tokenize input.\n");
    print_errors(&errors);
    return 1;
  }
    
  print_summary(tokens, token_count);


  int lexical_errors = count_errors_by_kind(&errors, ERR_KIND_LEXICAL);
  if (lexical_errors > 0) {
    ir_apply_error_suggestions(&errors);
    print_errors(&errors);
    return 1;
  }

  Stmt *ast = parse_program(tokens, token_count, &errors);
  if (!ast) {
    ir_apply_error_suggestions(&errors);
    print_errors(&errors);
    return 1;
  }

  int parse_errors = count_errors_by_kind(&errors, ERR_KIND_PARSE);
  printf("=== Parse Validation ===\n");
  if (parse_errors > 0) printf("Parse Result: invalid (%d error(s))\n", parse_errors);
  else printf("Parse Result: valid\n");

  if (parse_errors > 0) {
    ir_apply_error_suggestions(&errors);
    print_errors(&errors);
    parser_free_ast(ast);
    return 1;
  }

  semantic_analyze(ast, &errors);
  int semantic_errors = count_errors_by_kind(&errors, ERR_KIND_SEMANTIC);
  printf("=== Semantic Validation ===\n");
  if (semantic_errors > 0) printf("Semantic Result: invalid (%d error(s))\n", semantic_errors);
  else printf("Semantic Result: valid\n");

  if (semantic_errors > 0) {
    ir_apply_error_suggestions(&errors);
    print_errors(&errors);
    parser_free_ast(ast);
    return 1;
  }

  IRList ir;
  memset(&ir, 0, sizeof(ir));
  if (!ir_generate(ast, &ir)) {
    printf("IR generation failed.\n");
    parser_free_ast(ast);
    return 1;
  }

  printf("=== Intermediate Representation (TAC) ===\n");
  for (size_t i = 0; i < ir.count; i++) {
    printf("%zu: %s\n", i + 1, ir.lines[i]);
  }

  printf("\n");
  print_errors(&errors);

  parser_free_ast(ast);
  return 0;
}

