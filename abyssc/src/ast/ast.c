#include "../../include/ast/ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char *str_dup(const char *s) {
    if (!s) return NULL;
    char *d = malloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

/* ── Expressions ────────────────────────────────────────────── */

Expr *expr_new_int(char *value, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_INT_LIT;
    e->loc = loc;
    e->data.int_lit.value = str_dup(value);
    return e;
}

Expr *expr_new_float(char *value, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_FLOAT_LIT;
    e->loc = loc;
    e->data.float_lit.value = str_dup(value);
    return e;
}

Expr *expr_new_char(char *value, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_CHAR_LIT;
    e->loc = loc;
    e->data.char_lit.value = str_dup(value);
    return e;
}

Expr *expr_new_string(char *value, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_STRING_LIT;
    e->loc = loc;
    e->data.string_lit.value = str_dup(value);
    return e;
}

Expr *expr_new_identifier(char *name, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_IDENTIFIER;
    e->loc = loc;
    e->data.identifier.name = str_dup(name);
    return e;
}

Expr *expr_new_binary(Expr *left, TokenType op, Expr *right, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_BINARY;
    e->loc = loc;
    e->data.binary.left = left;
    e->data.binary.op = op;
    e->data.binary.right = right;
    return e;
}

Expr *expr_new_unary(TokenType op, Expr *operand, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_UNARY;
    e->loc = loc;
    e->data.unary.op = op;
    e->data.unary.operand = operand;
    return e;
}

Expr *expr_new_call(Expr *callee, ExprList *args, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_CALL;
    e->loc = loc;
    e->data.call.callee = callee;
    e->data.call.args = args;
    return e;
}

Expr *expr_new_member(Expr *object, char *field, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_MEMBER;
    e->loc = loc;
    e->data.member.object = object;
    e->data.member.field = str_dup(field);
    return e;
}

Expr *expr_new_index(Expr *array, Expr *index, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_INDEX;
    e->loc = loc;
    e->data.index.array = array;
    e->data.index.index = index;
    return e;
}

Expr *expr_new_deref(Expr *operand, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_DEREF;
    e->loc = loc;
    e->data.deref.operand = operand;
    return e;
}

Expr *expr_new_addrof(Expr *operand, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_ADDROF;
    e->loc = loc;
    e->data.addrof.operand = operand;
    return e;
}

Expr *expr_new_cast(char *type_name, Expr *operand, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_CAST;
    e->loc = loc;
    e->data.cast.type_name = str_dup(type_name);
    e->data.cast.operand = operand;
    return e;
}

Expr *expr_new_assign(char *name, Expr *value, SourceLocation loc) {
    Expr *e = malloc(sizeof(Expr));
    e->type = EXPR_ASSIGN;
    e->loc = loc;
    e->data.assign.name = str_dup(name);
    e->data.assign.value = value;
    return e;
}

void expr_free(Expr *expr) {
    if (!expr) return;
    switch (expr->type) {
        case EXPR_INT_LIT:    free(expr->data.int_lit.value); break;
        case EXPR_FLOAT_LIT:  free(expr->data.float_lit.value); break;
        case EXPR_CHAR_LIT:   free(expr->data.char_lit.value); break;
        case EXPR_STRING_LIT: free(expr->data.string_lit.value); break;
        case EXPR_IDENTIFIER: free(expr->data.identifier.name); break;
        case EXPR_BINARY:
            expr_free(expr->data.binary.left);
            expr_free(expr->data.binary.right);
            break;
        case EXPR_UNARY:
            expr_free(expr->data.unary.operand);
            break;
        case EXPR_CALL:
            expr_free(expr->data.call.callee);
            expr_list_free(expr->data.call.args);
            break;
        case EXPR_MEMBER:
            expr_free(expr->data.member.object);
            free(expr->data.member.field);
            break;
        case EXPR_INDEX:
            expr_free(expr->data.index.array);
            expr_free(expr->data.index.index);
            break;
        case EXPR_DEREF:
            expr_free(expr->data.deref.operand);
            break;
        case EXPR_ADDROF:
            expr_free(expr->data.addrof.operand);
            break;
        case EXPR_CAST:
            free(expr->data.cast.type_name);
            expr_free(expr->data.cast.operand);
            break;
        case EXPR_ASSIGN:
            free(expr->data.assign.name);
            expr_free(expr->data.assign.value);
            break;
    }
    free(expr);
}

ExprList *expr_list_new(void) { return NULL; }

void expr_list_append(ExprList **list, Expr *expr) {
    ExprList *node = malloc(sizeof(ExprList));
    node->expr = expr;
    node->next = NULL;
    if (!*list) {
        *list = node;
        return;
    }
    ExprList *cur = *list;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

void expr_list_free(ExprList *list) {
    while (list) {
        ExprList *next = list->next;
        expr_free(list->expr);
        free(list);
        list = next;
    }
}

/* ── Statements ─────────────────────────────────────────────── */

Stmt *stmt_new_expr(Expr *expr, SourceLocation loc) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_EXPR;
    s->loc = loc;
    s->data.expr_stmt.expr = expr;
    return s;
}

Stmt *stmt_new_return(Expr *value, SourceLocation loc) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_RETURN;
    s->loc = loc;
    s->data.return_stmt.value = value;
    return s;
}

Stmt *stmt_new_block(StmtList *stmts, SourceLocation loc) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_BLOCK;
    s->loc = loc;
    s->data.block.stmts = stmts;
    return s;
}

Stmt *stmt_new_var_decl(char *type_name, char *name, Expr *init, int is_ptr, SourceLocation loc) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_VAR_DECL;
    s->loc = loc;
    s->data.var_decl.type_name = str_dup(type_name);
    s->data.var_decl.name = str_dup(name);
    s->data.var_decl.init = init;
    s->data.var_decl.is_ptr = is_ptr;
    return s;
}

Stmt *stmt_new_if(Expr *condition, Stmt *then_block, ElifClause *elifs, Stmt *else_block, SourceLocation loc) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_IF;
    s->loc = loc;
    s->data.if_stmt.condition = condition;
    s->data.if_stmt.then_block = then_block;
    s->data.if_stmt.elifs = elifs;
    s->data.if_stmt.else_block = else_block;
    return s;
}

Stmt *stmt_new_while(Expr *condition, Stmt *body, SourceLocation loc) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_WHILE;
    s->loc = loc;
    s->data.while_stmt.condition = condition;
    s->data.while_stmt.body = body;
    return s;
}

Stmt *stmt_new_for(Stmt *init, Expr *condition, Stmt *increment, Stmt *body, SourceLocation loc) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_FOR;
    s->loc = loc;
    s->data.for_stmt.init = init;
    s->data.for_stmt.condition = condition;
    s->data.for_stmt.increment = increment;
    s->data.for_stmt.body = body;
    return s;
}

Stmt *stmt_new_do_while(Stmt *body, Expr *condition, SourceLocation loc) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_DO_WHILE;
    s->loc = loc;
    s->data.do_while_stmt.body = body;
    s->data.do_while_stmt.condition = condition;
    return s;
}

Stmt *stmt_new_break(SourceLocation loc) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_BREAK;
    s->loc = loc;
    return s;
}

Stmt *stmt_new_continue(SourceLocation loc) {
    Stmt *s = malloc(sizeof(Stmt));
    s->type = STMT_CONTINUE;
    s->loc = loc;
    return s;
}

void stmt_free(Stmt *stmt) {
    if (!stmt) return;
    switch (stmt->type) {
        case STMT_EXPR:
            expr_free(stmt->data.expr_stmt.expr);
            break;
        case STMT_RETURN:
            expr_free(stmt->data.return_stmt.value);
            break;
        case STMT_BLOCK:
            stmt_list_free(stmt->data.block.stmts);
            break;
        case STMT_VAR_DECL:
            free(stmt->data.var_decl.type_name);
            free(stmt->data.var_decl.name);
            expr_free(stmt->data.var_decl.init);
            break;
        case STMT_IF:
            expr_free(stmt->data.if_stmt.condition);
            stmt_free(stmt->data.if_stmt.then_block);
            elif_clause_free(stmt->data.if_stmt.elifs);
            stmt_free(stmt->data.if_stmt.else_block);
            break;
        case STMT_WHILE:
            expr_free(stmt->data.while_stmt.condition);
            stmt_free(stmt->data.while_stmt.body);
            break;
        case STMT_FOR:
            stmt_free(stmt->data.for_stmt.init);
            expr_free(stmt->data.for_stmt.condition);
            stmt_free(stmt->data.for_stmt.increment);
            stmt_free(stmt->data.for_stmt.body);
            break;
        case STMT_DO_WHILE:
            stmt_free(stmt->data.do_while_stmt.body);
            expr_free(stmt->data.do_while_stmt.condition);
            break;
        case STMT_BREAK:
        case STMT_CONTINUE:
            break;
    }
    free(stmt);
}

StmtList *stmt_list_new(void) { return NULL; }

void stmt_list_append(StmtList **list, Stmt *stmt) {
    StmtList *node = malloc(sizeof(StmtList));
    node->stmt = stmt;
    node->next = NULL;
    if (!*list) {
        *list = node;
        return;
    }
    StmtList *cur = *list;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

void stmt_list_free(StmtList *list) {
    while (list) {
        StmtList *next = list->next;
        stmt_free(list->stmt);
        free(list);
        list = next;
    }
}

ElifClause *elif_clause_new(Expr *condition, Stmt *body) {
    ElifClause *c = malloc(sizeof(ElifClause));
    c->condition = condition;
    c->body = body;
    c->next = NULL;
    return c;
}

void elif_clause_free(ElifClause *clause) {
    while (clause) {
        ElifClause *next = clause->next;
        expr_free(clause->condition);
        stmt_free(clause->body);
        free(clause);
        clause = next;
    }
}

/* ── Decorators ─────────────────────────────────────────────── */

Decorator *decorator_new(char *name, ExprList *args) {
    Decorator *d = malloc(sizeof(Decorator));
    d->name = str_dup(name);
    d->args = args;
    return d;
}

void decorator_free(Decorator *dec) {
    if (!dec) return;
    free(dec->name);
    expr_list_free(dec->args);
    free(dec);
}

DecoratorList *decorator_list_new(void) { return NULL; }

void decorator_list_append(DecoratorList **list, Decorator *dec) {
    DecoratorList *node = malloc(sizeof(DecoratorList));
    node->dec = *dec;
    node->next = NULL;
    free(dec);
    if (!*list) {
        *list = node;
        return;
    }
    DecoratorList *cur = *list;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

void decorator_list_free(DecoratorList *list) {
    while (list) {
        DecoratorList *next = list->next;
        free(list->dec.name);
        expr_list_free(list->dec.args);
        free(list);
        list = next;
    }
}

/* ── Function Parameters ────────────────────────────────────── */

FuncParam func_param_new(char *type_name, char *name, int is_ptr) {
    FuncParam p;
    p.type_name = str_dup(type_name);
    p.name = str_dup(name);
    p.is_ptr = is_ptr;
    return p;
}

void func_param_free(FuncParam *param) {
    free(param->type_name);
    free(param->name);
}

FuncParamList *func_param_list_new(void) { return NULL; }

void func_param_list_append(FuncParamList **list, FuncParam param) {
    FuncParamList *node = malloc(sizeof(FuncParamList));
    node->param = param;
    node->next = NULL;
    if (!*list) {
        *list = node;
        return;
    }
    FuncParamList *cur = *list;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

void func_param_list_free(FuncParamList *list) {
    while (list) {
        FuncParamList *next = list->next;
        func_param_free(&list->param);
        free(list);
        list = next;
    }
}

/* ── Struct Fields ──────────────────────────────────────────── */

StructField struct_field_new(char *type_name, char *name, int is_ptr) {
    StructField f;
    f.type_name = str_dup(type_name);
    f.name = str_dup(name);
    f.is_ptr = is_ptr;
    return f;
}

StructFieldList *struct_field_list_new(void) { return NULL; }

void struct_field_list_append(StructFieldList **list, StructField field) {
    StructFieldList *node = malloc(sizeof(StructFieldList));
    node->field = field;
    node->next = NULL;
    if (!*list) {
        *list = node;
        return;
    }
    StructFieldList *cur = *list;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

void struct_field_list_free(StructFieldList *list) {
    while (list) {
        StructFieldList *next = list->next;
        free(list->field.type_name);
        free(list->field.name);
        free(list);
        list = next;
    }
}

/* ── Interface Methods ──────────────────────────────────────── */

InterfaceMethod interface_method_new(char *return_type, char *name, FuncParamList *params) {
    InterfaceMethod m;
    m.return_type = str_dup(return_type);
    m.name = str_dup(name);
    m.params = params;
    return m;
}

InterfaceMethodList *interface_method_list_new(void) { return NULL; }

void interface_method_list_append(InterfaceMethodList **list, InterfaceMethod method) {
    InterfaceMethodList *node = malloc(sizeof(InterfaceMethodList));
    node->method = method;
    node->next = NULL;
    if (!*list) {
        *list = node;
        return;
    }
    InterfaceMethodList *cur = *list;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

void interface_method_list_free(InterfaceMethodList *list) {
    while (list) {
        InterfaceMethodList *next = list->next;
        free(list->method.return_type);
        free(list->method.name);
        func_param_list_free(list->method.params);
        free(list);
        list = next;
    }
}

/* ── Generic Parameters ─────────────────────────────────────── */

GenericParam generic_param_new(char *name) {
    GenericParam p;
    p.name = str_dup(name);
    return p;
}

void generic_param_free(GenericParam *param) {
    free(param->name);
}

GenericParamList *generic_param_list_new(void) { return NULL; }

void generic_param_list_append(GenericParamList **list, GenericParam param) {
    GenericParamList *node = malloc(sizeof(GenericParamList));
    node->param = param;
    node->next = NULL;
    if (!*list) {
        *list = node;
        return;
    }
    GenericParamList *cur = *list;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

void generic_param_list_free(GenericParamList *list) {
    while (list) {
        GenericParamList *next = list->next;
        generic_param_free(&list->param);
        free(list);
        list = next;
    }
}

/* ── Declarations ───────────────────────────────────────────── */

Decl *decl_new_func(char *return_type, char *name, FuncParamList *params,
                    GenericParamList *generic_params, Stmt *body,
                    DecoratorList *decorators, SourceLocation loc) {
    Decl *d = malloc(sizeof(Decl));
    d->type = DECL_FUNC;
    d->loc = loc;
    d->data.func.return_type = str_dup(return_type);
    d->data.func.name = str_dup(name);
    d->data.func.params = params;
    d->data.func.generic_params = generic_params;
    d->data.func.body = body;
    d->data.func.decorators = decorators;
    return d;
}

Decl *decl_new_struct(char *name, StructFieldList *fields,
                      GenericParamList *generic_params, DeclList *methods,
                      DecoratorList *decorators, SourceLocation loc) {
    Decl *d = malloc(sizeof(Decl));
    d->type = DECL_STRUCT;
    d->loc = loc;
    d->data.struct_decl.name = str_dup(name);
    d->data.struct_decl.fields = fields;
    d->data.struct_decl.generic_params = generic_params;
    d->data.struct_decl.methods = methods;
    d->data.struct_decl.decorators = decorators;
    return d;
}

Decl *decl_new_interface(char *name, InterfaceMethodList *methods,
                         GenericParamList *generic_params, SourceLocation loc) {
    Decl *d = malloc(sizeof(Decl));
    d->type = DECL_INTERFACE;
    d->loc = loc;
    d->data.interface.name = str_dup(name);
    d->data.interface.methods = methods;
    d->data.interface.generic_params = generic_params;
    return d;
}

Decl *decl_new_import(char *module_name, SourceLocation loc) {
    Decl *d = malloc(sizeof(Decl));
    d->type = DECL_IMPORT;
    d->loc = loc;
    d->data.import.module_name = str_dup(module_name);
    return d;
}

void decl_free(Decl *decl) {
    if (!decl) return;
    switch (decl->type) {
        case DECL_FUNC:
            free(decl->data.func.return_type);
            free(decl->data.func.name);
            func_param_list_free(decl->data.func.params);
            generic_param_list_free(decl->data.func.generic_params);
            stmt_free(decl->data.func.body);
            decorator_list_free(decl->data.func.decorators);
            break;
        case DECL_STRUCT:
            free(decl->data.struct_decl.name);
            struct_field_list_free(decl->data.struct_decl.fields);
            generic_param_list_free(decl->data.struct_decl.generic_params);
            decl_list_free(decl->data.struct_decl.methods);
            decorator_list_free(decl->data.struct_decl.decorators);
            break;
        case DECL_INTERFACE:
            free(decl->data.interface.name);
            interface_method_list_free(decl->data.interface.methods);
            generic_param_list_free(decl->data.interface.generic_params);
            break;
        case DECL_IMPORT:
            free(decl->data.import.module_name);
            break;
        default: break;
    }
    free(decl);
}

DeclList *decl_list_new(void) { return NULL; }

void decl_list_append(DeclList **list, Decl *decl) {
    DeclList *node = malloc(sizeof(DeclList));
    node->decl = decl;
    node->next = NULL;
    if (!*list) {
        *list = node;
        return;
    }
    DeclList *cur = *list;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

void decl_list_free(DeclList *list) {
    while (list) {
        DeclList *next = list->next;
        decl_free(list->decl);
        free(list);
        list = next;
    }
}

/* ── Program ────────────────────────────────────────────────── */

Program *program_new(char *filename) {
    Program *p = malloc(sizeof(Program));
    p->filename = str_dup(filename);
    p->decls = NULL;
    return p;
}

void program_add_decl(Program *program, Decl *decl) {
    decl_list_append(&program->decls, decl);
}

void program_free(Program *program) {
    if (!program) return;
    free(program->filename);
    decl_list_free(program->decls);
    free(program);
}
