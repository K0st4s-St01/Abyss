#ifndef TOKEN_H
#define TOKEN_H
#include "../utils/source_location.h"
#include <stdbool.h>

typedef enum{
  EndOfFile
  ,Invalid
  //literals
  ,CharLiteral
  ,StringLiteral
  ,IntLiteral
  ,FloatLiteral
  //Keywords
  ,I8
  ,I16
  ,I32
  ,I64
  ,U8
  ,U16
  ,U32
  ,U64
  ,F32
  ,F64
  ,Char
  ,Str
  ,Void
  ,Bool
  ,If
  ,Else
  ,Elif
  ,Switch
  ,While
  ,For
  ,Do
  ,Static
  ,Struct
  ,Interface
  ,Import
  ,Return
  ,Break
  ,Continue
  ,Self
  //operator
  ,Generic       // <>
  ,Arrow
  ,Plus
  ,PlusPlus
  ,PlusEquals
  ,Minus
  ,MinusMinus
  ,MinusEquals
  ,Star
  ,StarEquals
  ,Slash
  ,SlashEquals
  ,Percent
  ,Ampersand
  ,AmpersandEquals
  ,AmpersandAmpersand
  ,Caret
  ,Less
  ,LessEquals
  ,LeftShift
  ,Greater
  ,GreaterEquals
  ,RightShift
  ,Equals
  ,EqualsEquals
  //punctuation
  ,Comma
  ,Semicolon
  ,Dot
  ,Scope
  ,Tilde
  ,At
  ,LParen
  ,RParen
  ,LBrace
  ,RBrace
  ,LBracket
  ,RBracket
  //
  ,Identifier
}TokenType;

typedef struct{
  SourceLocation loc;
  char* text;
  TokenType type;  
}Token;

Token* token_new(SourceLocation loc,char* text,TokenType type);
void token_free(Token *token);
bool token_is_primitive_type(Token *token);
bool token_is_literal(Token *token);
bool token_is_keyword(Token *token);
bool token_is_operator(Token *token);
bool token_is_punctuation(Token *token);
bool token_is_identifier(Token *token);



#endif
