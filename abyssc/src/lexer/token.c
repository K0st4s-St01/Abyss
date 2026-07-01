#include "../../include/lexer/token.h"
#include <stdlib.h>
#include <string.h>

static bool between(size_t item,size_t starting_bound,size_t end_bound){
  return item>=starting_bound && item <= end_bound;
}

Token *token_new(SourceLocation loc, char *text, TokenType type) {
  Token *token = (Token *)malloc(sizeof(Token));
  if (!token) return NULL;
  token->loc = loc;
  token->text = strdup(text);
  token->type = type;
  return token;
}

void token_free(Token *token) {
  if (token) {
    free(token->text);
    free(token);
  }
}

const char *token_type_name(TokenType type) {
  switch (type) {
    case EndOfFile: return "end of file";
    case Invalid: return "invalid token";
    case CharLiteral: return "char literal";
    case StringLiteral: return "string literal";
    case IntLiteral: return "integer literal";
    case FloatLiteral: return "float literal";
    case I8: return "i8";
    case I16: return "i16";
    case I32: return "i32";
    case I64: return "i64";
    case U8: return "u8";
    case U16: return "u16";
    case U32: return "u32";
    case U64: return "u64";
    case F32: return "f32";
    case F64: return "f64";
    case Char: return "char";
    case Str: return "str";
    case Void: return "void";
    case Bool: return "bool";
    case If: return "if";
    case Else: return "else";
    case Elif: return "elif";
    case Switch: return "switch";
    case Case: return "case";
    case Default: return "default";
    case While: return "while";
    case For: return "for";
    case Do: return "do";
    case Static: return "static";
    case Type: return "type";
    case Enum: return "enum";
    case Struct: return "struct";
    case Interface: return "interface";
    case Import: return "import";
    case Return: return "return";
    case Break: return "break";
    case Continue: return "continue";
    case Self: return "self";
    case Extern: return "extern";
    case New: return "new";
    case Delete: return "delete";
    case Null: return "null";
    case True: return "true";
    case False: return "false";
    case Sizeof: return "sizeof";
    case Generic: return "generic delimiter";
    case Arrow: return "->";
    case Plus: return "+";
    case PlusPlus: return "++";
    case PlusEquals: return "+=";
    case Minus: return "-";
    case MinusMinus: return "--";
    case MinusEquals: return "-=";
    case Star: return "*";
    case StarEquals: return "*=";
    case Slash: return "/";
    case SlashEquals: return "/=";
    case Percent: return "%";
    case Ampersand: return "&";
    case AmpersandEquals: return "&=";
    case AmpersandAmpersand: return "&&";
    case Caret: return "^";
    case Less: return "<";
    case LessEquals: return "<=";
    case LeftShift: return "<<";
    case LeftShiftEquals: return "<<=";
    case Greater: return ">";
    case GreaterEquals: return ">=";
    case RightShift: return ">>";
    case RightShiftEquals: return ">>=";
    case Equals: return "=";
    case EqualsEquals: return "==";
    case BangEquals: return "!=";
    case Bang: return "!";
    case PipePipe: return "||";
    case Pipe: return "|";
    case PipeEquals: return "|=";
    case PercentEquals: return "%=";
    case CaretEquals: return "^=";
    case Tilde: return "~";
    case Comma: return ",";
    case Semicolon: return ";";
    case Dot: return ".";
    case Ellipsis: return "...";
    case Question: return "?";
    case Scope: return ":";
    case At: return "@";
    case LParen: return "(";
    case RParen: return ")";
    case LBrace: return "{";
    case RBrace: return "}";
    case LBracket: return "[";
    case RBracket: return "]";
    case Identifier: return "identifier";
    default: return "unknown token";
  }
}

bool token_is_primitive_type(Token *token){
  return between(token->type,I8,Bool);
}
bool token_is_literal(Token *token){
  return between(token->type,CharLiteral,StringLiteral);
}
bool token_is_keyword(Token *token){
  return token_is_primitive_type(token) ||
          between(token->type,If,Sizeof);
}
bool token_is_operator(Token *token){
  return between(token->type,Arrow,Tilde);
}
bool token_is_punctuation(Token *token){
  return between(token->type,Comma,RBracket);
}
bool token_is_identifier(Token *token){
  return token->type == Identifier;
}
