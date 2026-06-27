#include "../../include/lexer/token.h"
#include <stdlib.h>
#include <string.h>

static bool between(size_t item,size_t starting_bound,size_t end_bound){
  return item>=starting_bound && item <= end_bound;
}

Token *token_new(SourceLocation loc, char *text, TokenType type) {
  Token *token = (Token *)malloc(sizeof(Token));
  token->loc = token->loc;
  strcpy(token->text, text);
  token->type = type;
  return token;
}

bool token_is_primitive_type(Token *token){
  return between(token->type,I8,Bool);
}
bool token_is_literal(Token *token){
  return between(token->type,CharLiteral,StringLiteral);
}
bool token_is_keyword(Token *token){
  return token_is_primitive_type(token) ||
          between(token->type,If,Import);
}
bool token_is_operator(Token *token){
  return between(token->type,Arrow,EqualsEquals);
}
bool token_is_punctuation(Token *token){
  return between(token->type,Comma,RBracket);
}
bool token_is_identifier(Token *token){
  return token->type == Identifier;
}
