#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "errors.h"
#include "lexer.h"

// Parse the token stream into an AST (program is represented as a STMT_BLOCK root).
// Returns NULL on severe failure; callers should check errors.
Stmt *parse_program(Token *tokens, int token_count, ErrorList *errors);

// Frees the AST nodes recursively.
void parser_free_ast(Stmt *root);

#endif // PARSER_H

