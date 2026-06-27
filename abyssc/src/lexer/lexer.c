#include "../../include/lexer/lexer.h"
#include <string.h>

Lexer *lexer_new(char *filename, char *source_code) {
  Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
  lexer->filename = filename;
  lexer->source_code = source_code;
  lexer->line = 1;
  lexer->col = 1;
  lexer->index = 0;
  lexer-> source_code_len=  strlen(source_code);
  return lexer;
}
char lexer_peek(Lexer *lexer, int offset){
    if(lexer->index + offset >  lexer->source_code_len){
        return '\0';
    }
    return lexer-> source_code[lexer->index+offset];
}
char lexer_advance(Lexer *lexer){

}
void lexer_skip_whitespace_and_comments(Lexer *lexer);
SourceLocation lexer_current_location(Lexer *lexer);

Token *lexer_next_token(Lexer *lexer);
Token *lexer_make_identifier_or_keyword(Lexer *lexer);
Token *lexer_make_literal(Lexer *lexer);
Token *lexer_make_punctuation(Lexer *lexer);
Token *lexer_make_operator(Lexer *lexer);
