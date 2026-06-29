#ifndef PARSER_H
#define PARSER_H

#include "../lexer/vec_token.h"
#include "../ast/ast.h"
#include <stdbool.h>

typedef struct {
    Tokens *tokens;
    size_t pos;
    bool had_error;
    bool panic_mode;
    char error_msg[256];
} Parser;

Parser *parser_new(Tokens *tokens);
void parser_free(Parser *p);

Program *parse_program(Parser *p, char *filename);

#endif
