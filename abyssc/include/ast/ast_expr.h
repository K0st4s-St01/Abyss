#ifndef AST_EXPR_H
#define AST_EXPR_H

#include "../utils/source_location.h"
#include "../lexer/token.h"

typedef enum {
    EXPR_INT_LIT,
    EXPR_FLOAT_LIT,
    EXPR_CHAR_LIT,
    EXPR_STRING_LIT,
    EXPR_IDENTIFIER,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_INDEX,
    EXPR_MEMBER,
    EXPR_DEREF,
    EXPR_ADDROF,
    EXPR_CAST,
    EXPR_ASSIGN,
} ExprType;

typedef struct Expr Expr;
typedef struct ExprList { Expr *expr; struct ExprList *next; } ExprList;

typedef struct { char *value; } IntLitExpr;
typedef struct { char *value; } FloatLitExpr;
typedef struct { char *value; } CharLitExpr;
typedef struct { char *value; } StringLitExpr;
typedef struct { char *name; } IdentifierExpr;
typedef struct { Expr *left; TokenType op; Expr *right; } BinaryExpr;
typedef struct { TokenType op; Expr *operand; } UnaryExpr;
typedef struct { Expr *callee; ExprList *args; } CallExpr;
typedef struct { Expr *object; char *field; } MemberExpr;
typedef struct { Expr *array; Expr *index; } IndexExpr;
typedef struct { Expr *operand; } DerefExpr;
typedef struct { Expr *operand; } AddrOfExpr;
typedef struct { char *type_name; Expr *operand; } CastExpr;
typedef struct { char *name; Expr *value; } AssignExpr;

struct Expr {
    ExprType type;
    SourceLocation loc;
    union {
        IntLitExpr int_lit;
        FloatLitExpr float_lit;
        CharLitExpr char_lit;
        StringLitExpr string_lit;
        IdentifierExpr identifier;
        BinaryExpr binary;
        UnaryExpr unary;
        CallExpr call;
        MemberExpr member;
        IndexExpr index;
        DerefExpr deref;
        AddrOfExpr addrof;
        CastExpr cast;
        AssignExpr assign;
    } data;
};

Expr *expr_new_int(char *value, SourceLocation loc);
Expr *expr_new_float(char *value, SourceLocation loc);
Expr *expr_new_char(char *value, SourceLocation loc);
Expr *expr_new_string(char *value, SourceLocation loc);
Expr *expr_new_identifier(char *name, SourceLocation loc);
Expr *expr_new_binary(Expr *left, TokenType op, Expr *right, SourceLocation loc);
Expr *expr_new_unary(TokenType op, Expr *operand, SourceLocation loc);
Expr *expr_new_call(Expr *callee, ExprList *args, SourceLocation loc);
Expr *expr_new_member(Expr *object, char *field, SourceLocation loc);
Expr *expr_new_index(Expr *array, Expr *index, SourceLocation loc);
Expr *expr_new_deref(Expr *operand, SourceLocation loc);
Expr *expr_new_addrof(Expr *operand, SourceLocation loc);
Expr *expr_new_cast(char *type_name, Expr *operand, SourceLocation loc);
Expr *expr_new_assign(char *name, Expr *value, SourceLocation loc);
void expr_free(Expr *expr);
ExprList *expr_list_new(void);
void expr_list_append(ExprList **list, Expr *expr);
void expr_list_free(ExprList *list);

#endif
