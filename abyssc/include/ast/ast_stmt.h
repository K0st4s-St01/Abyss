#ifndef AST_STMT_H
#define AST_STMT_H

#include "ast_expr.h"

typedef enum {
    STMT_EXPR,
    STMT_RETURN,
    STMT_BLOCK,
    STMT_VAR_DECL,
    STMT_IF,
    STMT_SWITCH,
    STMT_WHILE,
    STMT_FOR,
    STMT_DO_WHILE,
    STMT_BREAK,
    STMT_CONTINUE,
} StmtType;

typedef struct Stmt Stmt;
typedef struct StmtList { Stmt *stmt; struct StmtList *next; } StmtList;

typedef struct { Expr *expr; } ExprStmt;
typedef struct { Expr *value; } ReturnStmt;
typedef struct { StmtList *stmts; } BlockStmt;
typedef struct { char *type_name; char *name; Expr *init; int is_ptr; } VarDeclStmt;

typedef struct ElifClause {
    Expr *condition;
    Stmt *body;
    struct ElifClause *next;
} ElifClause;

typedef struct {
    Expr *condition;
    Stmt *then_block;
    ElifClause *elifs;
    Stmt *else_block;
} IfStmt;

typedef struct SwitchCase {
    Expr *value;
    StmtList *stmts;
    struct SwitchCase *next;
} SwitchCase;

typedef struct {
    Expr *expr;
    SwitchCase *cases;
    StmtList *default_stmts;
} SwitchStmt;

typedef struct { Expr *condition; Stmt *body; } WhileStmt;
typedef struct { Stmt *init; Expr *condition; Stmt *increment; Stmt *body; } ForStmt;
typedef struct { Stmt *body; Expr *condition; } DoWhileStmt;

struct Stmt {
    StmtType type;
    SourceLocation loc;
    union {
        ExprStmt expr_stmt;
        ReturnStmt return_stmt;
        BlockStmt block;
        VarDeclStmt var_decl;
        IfStmt if_stmt;
        SwitchStmt switch_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        DoWhileStmt do_while_stmt;
    } data;
};

Stmt *stmt_new_expr(Expr *expr, SourceLocation loc);
Stmt *stmt_new_return(Expr *value, SourceLocation loc);
Stmt *stmt_new_block(StmtList *stmts, SourceLocation loc);
Stmt *stmt_new_var_decl(char *type_name, char *name, Expr *init, int is_ptr, SourceLocation loc);
Stmt *stmt_new_if(Expr *condition, Stmt *then_block, ElifClause *elifs, Stmt *else_block, SourceLocation loc);
Stmt *stmt_new_switch(Expr *expr, SwitchCase *cases, StmtList *default_stmts, SourceLocation loc);
Stmt *stmt_new_while(Expr *condition, Stmt *body, SourceLocation loc);
Stmt *stmt_new_for(Stmt *init, Expr *condition, Stmt *increment, Stmt *body, SourceLocation loc);
Stmt *stmt_new_do_while(Stmt *body, Expr *condition, SourceLocation loc);
Stmt *stmt_new_break(SourceLocation loc);
Stmt *stmt_new_continue(SourceLocation loc);
void stmt_free(Stmt *stmt);
StmtList *stmt_list_new(void);
void stmt_list_append(StmtList **list, Stmt *stmt);
void stmt_list_free(StmtList *list);
ElifClause *elif_clause_new(Expr *condition, Stmt *body);
void elif_clause_free(ElifClause *clause);
SwitchCase *switch_case_new(Expr *value, StmtList *stmts);
void switch_case_free(SwitchCase *case_list);

#endif
