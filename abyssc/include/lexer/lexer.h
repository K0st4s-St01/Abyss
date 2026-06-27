# ifndef LEXER_H
# define LEXER_H

#include <stdlib.h>
#include "token.h"

typedef struct {
     size_t index;
    int line,col;
    size_t source_code_len;
    char* filename;
    char* source_code;
} Lexer;

Lexer *lexer_new(char *filename,char *source_code);
char lexer_peek(Lexer* lexer,int offset);
char lexer_advance(Lexer* lexer);
void lexer_skip_whitespace_and_comments(Lexer* lexer);
SourceLocation lexer_current_location(Lexer* lexer);

Token *lexer_next_token(Lexer* lexer);
Token *lexer_make_identifier_or_keyword(Lexer *lexer);
Token *lexer_make_literal(Lexer* lexer);
Token *lexer_make_punctuation(Lexer* lexer);
Token *lexer_make_operator(Lexer* lexer);

Lexer *lexer_delete(Lexer* lexer);



# endif