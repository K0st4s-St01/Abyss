#include "../../include/semantic/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ── Hash table / symbol table ──────────────────────────────── */

#define BUCKETS 64

typedef struct Symbol {
    char *name;
    char *type_name;
    int is_ptr;
    int is_func;
    int used;
    FuncParamList *params;
    SourceLocation loc;
    struct Symbol *next;
} Symbol;

typedef struct SemScope {
    Symbol *buckets[BUCKETS];
    struct SemScope *parent;
} SemScope;

static unsigned long djb2(const char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        h = ((h << 5) + h) + (unsigned long)c;
    return h % BUCKETS;
}

static SemScope *scope_new(SemScope *parent) {
    SemScope *s = calloc(1, sizeof(SemScope));
    s->parent = parent;
    return s;
}

static void scope_free(SemScope *s) {
    for (int i = 0; i < BUCKETS; i++) {
        Symbol *sym = s->buckets[i];
        while (sym) {
            Symbol *next = sym->next;
            free(sym->name);
            free(sym->type_name);
            free(sym);
            sym = next;
        }
    }
    free(s);
}

static int scope_insert(SemScope *s, const char *name, const char *type_name,
                        int is_ptr, int is_func, FuncParamList *params,
                        SourceLocation loc) {
    for (Symbol *sym = s->buckets[djb2(name)]; sym; sym = sym->next) {
        if (strcmp(sym->name, name) == 0)
            return 0;
    }
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type_name = type_name ? strdup(type_name) : NULL;
    sym->is_ptr = is_ptr;
    sym->is_func = is_func;
    sym->used = 0;
    sym->params = params;
    sym->loc = loc;
    unsigned long h = djb2(name);
    sym->next = s->buckets[h];
    s->buckets[h] = sym;
    return 1;
}

static Symbol *scope_lookup(SemScope *s, const char *name) {
    for (SemScope *cur = s; cur; cur = cur->parent) {
        for (Symbol *sym = cur->buckets[djb2(name)]; sym; sym = sym->next) {
            if (strcmp(sym->name, name) == 0)
                return sym;
        }
    }
    return NULL;
}

static Symbol *scope_lookup_current(SemScope *s, const char *name) {
    for (Symbol *sym = s->buckets[djb2(name)]; sym; sym = sym->next) {
        if (strcmp(sym->name, name) == 0)
            return sym;
    }
    return NULL;
}

/* ── Struct / interface type registry ──────────────────────── */

typedef struct {
    char *name;
    StructFieldList *fields;
    GenericParamList *generic_params;
    DeclList *methods;
} StructInfo;

typedef struct {
    char *name;
    InterfaceMethodList *methods;
    GenericParamList *generic_params;
} InterfaceInfo;

typedef struct TypeRegistry {
    StructInfo **structs;
    int struct_count;
    int struct_cap;
    InterfaceInfo **interfaces;
    int iface_count;
    int iface_cap;
} TypeRegistry;

static TypeRegistry *type_registry_new(void) {
    TypeRegistry *tr = calloc(1, sizeof(TypeRegistry));
    tr->struct_cap = 16;
    tr->structs = malloc(sizeof(StructInfo*) * tr->struct_cap);
    tr->iface_cap = 16;
    tr->interfaces = malloc(sizeof(InterfaceInfo*) * tr->iface_cap);
    return tr;
}

static void type_registry_free(TypeRegistry *tr) {
    for (int i = 0; i < tr->struct_count; i++) {
        free(tr->structs[i]->name);
        free(tr->structs[i]);
    }
    free(tr->structs);
    for (int i = 0; i < tr->iface_count; i++) {
        free(tr->interfaces[i]->name);
        free(tr->interfaces[i]);
    }
    free(tr->interfaces);
    free(tr);
}

static void registry_add_struct(TypeRegistry *tr, const char *name,
                                StructFieldList *fields,
                                GenericParamList *generic_params,
                                DeclList *methods) {
    if (tr->struct_count == tr->struct_cap) {
        tr->struct_cap *= 2;
        tr->structs = realloc(tr->structs, sizeof(StructInfo*) * tr->struct_cap);
    }
    StructInfo *si = malloc(sizeof(StructInfo));
    si->name = strdup(name);
    si->fields = fields;
    si->generic_params = generic_params;
    si->methods = methods;
    tr->structs[tr->struct_count++] = si;
}

static StructInfo *registry_find_struct(TypeRegistry *tr, const char *name) {
    for (int i = 0; i < tr->struct_count; i++) {
        if (strcmp(tr->structs[i]->name, name) == 0)
            return tr->structs[i];
    }
    return NULL;
}

static void registry_add_interface(TypeRegistry *tr, const char *name,
                                   InterfaceMethodList *methods,
                                   GenericParamList *generic_params) {
    if (tr->iface_count == tr->iface_cap) {
        tr->iface_cap *= 2;
        tr->interfaces = realloc(tr->interfaces, sizeof(InterfaceInfo*) * tr->iface_cap);
    }
    InterfaceInfo *ii = malloc(sizeof(InterfaceInfo));
    ii->name = strdup(name);
    ii->methods = methods;
    ii->generic_params = generic_params;
    tr->interfaces[tr->iface_count++] = ii;
}

/* ── Error list ────────────────────────────────────────────── */

typedef struct Error {
    char *message;
    SourceLocation loc;
    struct Error *next;
} Error;

/* ── Semantic analyzer ─────────────────────────────────────── */

struct SemanticAnalyzer {
    SemScope *scope;
    TypeRegistry *types;
    char *current_func_name;
    char *current_func_return;
    char *current_struct_name;
    int loop_depth;
    int had_error;
    Error *errors;
    Error *error_tail;
};

SemanticAnalyzer *semantic_analyzer_new(void) {
    SemanticAnalyzer *a = calloc(1, sizeof(SemanticAnalyzer));
    a->scope = scope_new(NULL);
    a->types = type_registry_new();
    return a;
}

void semantic_analyzer_free(SemanticAnalyzer *a) {
    if (!a) return;
    SemScope *cur = a->scope;
    while (cur) {
        SemScope *parent = cur->parent;
        scope_free(cur);
        cur = parent;
    }
    type_registry_free(a->types);
    Error *e = a->errors;
    while (e) {
        Error *next = e->next;
        free(e->message);
        free(e);
        e = next;
    }
    free(a->current_func_name);
    free(a->current_func_return);
    free(a->current_struct_name);
    free(a);
}

static void sem_error(SemanticAnalyzer *a, SourceLocation loc,
                      const char *fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Error *e = malloc(sizeof(Error));
    e->message = strdup(buf);
    e->loc = loc;
    e->next = NULL;

    if (a->error_tail) {
        a->error_tail->next = e;
    } else {
        a->errors = e;
    }
    a->error_tail = e;
    a->had_error = 1;
}

static void sem_warn(SemanticAnalyzer *a, SourceLocation loc,
                     const char *fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    fprintf(stderr, "Warning at %s:%d:%d: %s\n",
            loc.filename ? loc.filename : "<unknown>",
            loc.line, loc.col, buf);
}

void semantic_print_errors(SemanticAnalyzer *a) {
    for (Error *e = a->errors; e; e = e->next) {
        fprintf(stderr, "Semantic error at %s:%d:%d: %s\n",
                e->loc.filename ? e->loc.filename : "<unknown>",
                e->loc.line, e->loc.col, e->message);
    }
}

/* ── Type helpers ──────────────────────────────────────────── */

static int is_numeric_type(const char *name) {
    return strcmp(name, "i8") == 0 || strcmp(name, "i16") == 0 ||
           strcmp(name, "i32") == 0 || strcmp(name, "i64") == 0 ||
           strcmp(name, "u8") == 0 || strcmp(name, "u16") == 0 ||
           strcmp(name, "u32") == 0 || strcmp(name, "u64") == 0 ||
           strcmp(name, "f32") == 0 || strcmp(name, "f64") == 0;
}

static int is_float_type(const char *name) {
    return strcmp(name, "f32") == 0 || strcmp(name, "f64") == 0;
}

static int is_signed_type(const char *name) {
    return strcmp(name, "i8") == 0 || strcmp(name, "i16") == 0 ||
           strcmp(name, "i32") == 0 || strcmp(name, "i64") == 0;
}

static int is_unsigned_type(const char *name) {
    return strcmp(name, "u8") == 0 || strcmp(name, "u16") == 0 ||
           strcmp(name, "u32") == 0 || strcmp(name, "u64") == 0;
}

static int type_is_ptr(const char *type) {
    if (!type) return 0;
    size_t len = strlen(type);
    return len > 0 && type[len - 1] == '*';
}

static char *type_deref(const char *type) {
    if (!type || !type_is_ptr(type)) return NULL;
    size_t len = strlen(type);
    char *result = malloc(len);
    memcpy(result, type, len - 1);
    result[len - 1] = '\0';
    return result;
}

static char *type_make_ptr(const char *type) {
    if (!type) return NULL;
    size_t len = strlen(type);
    char *result = malloc(len + 2);
    memcpy(result, type, len);
    result[len] = '*';
    result[len + 1] = '\0';
    return result;
}

static int types_equal(const char *a, const char *b) {
    if (!a || !b) return a == b;
    return strcmp(a, b) == 0;
}

static int types_compatible(const char *expected, const char *actual) {
    if (!expected || !actual) return 0;
    if (types_equal(expected, actual)) return 1;

    if (is_numeric_type(expected) && is_numeric_type(actual)) {
        if (is_float_type(expected) || is_float_type(actual))
            return 1;
        if (is_signed_type(expected) && is_signed_type(actual))
            return 1;
        if (is_unsigned_type(expected) && is_unsigned_type(actual))
            return 1;
    }

    if (type_is_ptr(expected) && strcmp(actual, "void") == 0)
        return 1;
    if (type_is_ptr(actual) && strcmp(expected, "void") == 0)
        return 1;

    return 0;
}

/* ── Forward declarations ──────────────────────────────────── */

static char *analyze_expr(SemanticAnalyzer *a, Expr *expr);
static void analyze_stmt(SemanticAnalyzer *a, Stmt *stmt);
static void analyze_stmt_list(SemanticAnalyzer *a, StmtList *list);
static void analyze_decl(SemanticAnalyzer *a, Decl *decl);

/* ── Expression analysis ───────────────────────────────────── */

static char *analyze_expr(SemanticAnalyzer *a, Expr *expr) {
    if (!expr) return NULL;

    switch (expr->type) {
    case EXPR_INT_LIT:
        return strdup("i32");

    case EXPR_FLOAT_LIT:
        return strdup("f64");

    case EXPR_CHAR_LIT:
        return strdup("char");

    case EXPR_STRING_LIT:
        return strdup("str");

    case EXPR_IDENTIFIER: {
        const char *name = expr->data.identifier.name;
        Symbol *sym = scope_lookup(a->scope, name);
        if (!sym) {
            sem_error(a, expr->loc, "undeclared identifier '%s'", name);
            return NULL;
        }
        sym->used = 1;
        return sym->type_name ? strdup(sym->type_name) : NULL;
    }

    case EXPR_BINARY: {
        TokenType op = expr->data.binary.op;

        if (op == Equals) {
            Expr *lhs = expr->data.binary.left;
            char *rt = analyze_expr(a, expr->data.binary.right);
            if (!rt) return NULL;

            if (lhs->type == EXPR_IDENTIFIER) {
                const char *lname = lhs->data.identifier.name;
                Symbol *lsym = scope_lookup(a->scope, lname);
                if (!lsym) {
                    sem_error(a, expr->loc, "undeclared variable '%s' in assignment", lname);
                    free(rt); return NULL;
                }
                lsym->used = 1;
                if (lsym->type_name && !types_compatible(lsym->type_name, rt)) {
                    sem_error(a, expr->loc, "type mismatch in assignment: '%s' = '%s'",
                              lsym->type_name, rt);
                    free(rt); return NULL;
                }
                char *result = lsym->type_name ? strdup(lsym->type_name) : NULL;
                free(rt); return result;
            }

            if (lhs->type == EXPR_MEMBER) {
                char *lt = analyze_expr(a, lhs);
                if (!lt) { free(rt); return NULL; }
                if (!types_compatible(lt, rt)) {
                    sem_error(a, expr->loc, "type mismatch in field assignment: '%s' = '%s'", lt, rt);
                    free(lt); free(rt); return NULL;
                }
                { char *result = strdup(lt); free(lt); free(rt); return result; }
            }

            if (lhs->type == EXPR_DEREF) {
                char *lt = analyze_expr(a, lhs);
                if (!lt) { free(rt); return NULL; }
                if (!types_compatible(lt, rt)) {
                    sem_error(a, expr->loc, "type mismatch in deref assignment: '%s' = '%s'", lt, rt);
                    free(lt); free(rt); return NULL;
                }
                { char *result = strdup(lt); free(lt); free(rt); return result; }
            }

            sem_error(a, expr->loc, "assignment target is not assignable");
            free(rt); return NULL;
        }

        char *lt = analyze_expr(a, expr->data.binary.left);
        char *rt = analyze_expr(a, expr->data.binary.right);
        if (!lt || !rt) { free(lt); free(rt); return NULL; }

        switch (op) {
        case Plus: case Minus: case Star: case Slash: case Percent:
            if (!is_numeric_type(lt)) {
                sem_error(a, expr->loc, "operator applied to non-numeric type '%s'", lt);
                free(lt); free(rt); return NULL;
            }
            if (!types_compatible(lt, rt)) {
                sem_error(a, expr->loc, "type mismatch: '%s' and '%s'", lt, rt);
                free(lt); free(rt); return NULL;
            }
            { char *result = strdup(lt); free(lt); free(rt); return result; }

        case EqualsEquals:
            if (!types_compatible(lt, rt)) {
                sem_error(a, expr->loc, "comparison between incompatible types '%s' and '%s'", lt, rt);
                free(lt); free(rt); return NULL;
            }
            { char *result = strdup("bool"); free(lt); free(rt); return result; }

        case Less: case LessEquals: case Greater: case GreaterEquals:
            if (!is_numeric_type(lt) || !is_numeric_type(rt)) {
                sem_error(a, expr->loc, "relational operator requires numeric types");
                free(lt); free(rt); return NULL;
            }
            { char *result = strdup("bool"); free(lt); free(rt); return result; }

        case AmpersandAmpersand:
            if (!types_equal(lt, "bool") || !types_equal(rt, "bool")) {
                sem_error(a, expr->loc, "logical AND requires bool operands");
                free(lt); free(rt); return NULL;
            }
            { char *result = strdup("bool"); free(lt); free(rt); return result; }

        case Ampersand: case Caret:
            if (!is_numeric_type(lt) || !is_numeric_type(rt)) {
                sem_error(a, expr->loc, "bitwise operator requires numeric types");
                free(lt); free(rt); return NULL;
            }
            { char *result = strdup(lt); free(lt); free(rt); return result; }

        default:
            sem_error(a, expr->loc, "unsupported binary operator");
            free(lt); free(rt); return NULL;
        }
    }

    case EXPR_UNARY: {
        char *ot = analyze_expr(a, expr->data.unary.operand);
        if (!ot) return NULL;

        switch (expr->data.unary.op) {
        case Minus:
            if (!is_numeric_type(ot)) {
                sem_error(a, expr->loc, "negation applied to non-numeric type '%s'", ot);
                free(ot); return NULL;
            }
            return ot;
        case Star:
            if (!type_is_ptr(ot)) {
                sem_error(a, expr->loc, "dereference of non-pointer type '%s'", ot);
                free(ot); return NULL;
            }
            { char *result = type_deref(ot); free(ot); return result; }
        case Ampersand:
            return ot;
        default:
            return ot;
        }
    }

    case EXPR_CALL: {
        Expr *callee = expr->data.call.callee;
        if (callee->type != EXPR_IDENTIFIER) {
            sem_error(a, expr->loc, "invalid call target");
            return NULL;
        }
        const char *fname = callee->data.identifier.name;
        Symbol *fsym = scope_lookup(a->scope, fname);
        if (!fsym) {
            sem_error(a, expr->loc, "undeclared function '%s'", fname);
            return NULL;
        }
        if (!fsym->is_func) {
            sem_error(a, expr->loc, "'%s' is not a function", fname);
            return NULL;
        }

        int argc = 0;
        for (ExprList *al = expr->data.call.args; al; al = al->next) argc++;

        if (fsym->params) {
            int param_count = 0;
            for (FuncParamList *pl = fsym->params; pl; pl = pl->next) param_count++;

            if (argc != param_count) {
                sem_error(a, expr->loc,
                    "function '%s' expects %d arguments, got %d",
                    fname, param_count, argc);
            } else {
                ExprList *al = expr->data.call.args;
                FuncParamList *pl = fsym->params;
                int idx = 0;
                while (al && pl) {
                    char *at = analyze_expr(a, al->expr);
                    if (at && !types_compatible(pl->param.type_name, at)) {
                        sem_error(a, expr->loc,
                            "argument %d of '%s': expected '%s', got '%s'",
                            idx + 1, fname, pl->param.type_name, at);
                    }
                    free(at);
                    al = al->next;
                    pl = pl->next;
                    idx++;
                }
            }
        }

        if (fsym->type_name)
            return strdup(fsym->type_name);
        return NULL;
    }

    case EXPR_MEMBER: {
        char *objt = analyze_expr(a, expr->data.member.object);
        if (!objt) return NULL;

        const char *field = expr->data.member.field;
        char base[256] = {0};

        if (type_is_ptr(objt)) {
            char *d = type_deref(objt);
            if (d) { strncpy(base, d, sizeof(base) - 1); free(d); }
        } else {
            strncpy(base, objt, sizeof(base) - 1);
        }
        free(objt);

        char *angle = strchr(base, '<');
        if (angle) *angle = '\0';

        if (base[0] == '\0') return NULL;

        StructInfo *si = registry_find_struct(a->types, base);
        if (!si) {
            sem_error(a, expr->loc, "cannot access member on non-struct type '%s'", base);
            return NULL;
        }

        for (StructFieldList *fl = si->fields; fl; fl = fl->next) {
            if (strcmp(fl->field.name, field) == 0)
                return strdup(fl->field.type_name);
        }

        for (DeclList *ml = si->methods; ml; ml = ml->next) {
            if (ml->decl->type == DECL_FUNC &&
                strcmp(ml->decl->data.func.name, field) == 0)
                return ml->decl->data.func.return_type
                    ? strdup(ml->decl->data.func.return_type) : NULL;
        }

        sem_error(a, expr->loc, "no field or method '%s' in struct '%s'", field, base);
        return NULL;
    }

    case EXPR_DEREF: {
        char *ot = analyze_expr(a, expr->data.deref.operand);
        if (!ot) return NULL;
        if (!type_is_ptr(ot)) {
            sem_error(a, expr->loc, "dereference of non-pointer type '%s'", ot);
            free(ot); return NULL;
        }
        { char *result = type_deref(ot); free(ot); return result; }
    }

    case EXPR_ADDROF: {
        char *ot = analyze_expr(a, expr->data.addrof.operand);
        if (!ot) return NULL;
        char *result = type_make_ptr(ot);
        free(ot);
        return result;
    }

    case EXPR_CAST: {
        char *ot = analyze_expr(a, expr->data.cast.operand);
        free(ot);
        return expr->data.cast.type_name ? strdup(expr->data.cast.type_name) : NULL;
    }

    case EXPR_ASSIGN: {
        Symbol *sym = scope_lookup(a->scope, expr->data.assign.name);
        if (!sym) {
            sem_error(a, expr->loc, "undeclared variable '%s' in assignment",
                      expr->data.assign.name);
            return NULL;
        }
        sym->used = 1;
        char *vt = analyze_expr(a, expr->data.assign.value);
        if (!vt) return NULL;
        if (sym->type_name && !types_compatible(sym->type_name, vt)) {
            sem_error(a, expr->loc, "type mismatch in assignment to '%s': '%s' = '%s'",
                      expr->data.assign.name, sym->type_name, vt);
            free(vt); return NULL;
        }
        { char *result = sym->type_name ? strdup(sym->type_name) : NULL; free(vt); return result; }
    }

    case EXPR_INDEX: {
        char *at = analyze_expr(a, expr->data.index.array);
        char *it = analyze_expr(a, expr->data.index.index);
        free(it);
        if (!at) return NULL;
        if (type_is_ptr(at)) {
            char *result = type_deref(at); free(at); return result;
        }
        free(at);
        return NULL;
    }
    }

    return NULL;
}

/* ── Statement analysis ────────────────────────────────────── */

static void analyze_block(SemanticAnalyzer *a, Stmt *stmt) {
    if (!stmt || stmt->type != STMT_BLOCK) return;

    SemScope *block_scope = scope_new(a->scope);
    a->scope = block_scope;

    analyze_stmt_list(a, stmt->data.block.stmts);

    a->scope = block_scope->parent;
    scope_free(block_scope);
}

static void analyze_stmt(SemanticAnalyzer *a, Stmt *stmt) {
    if (!stmt) return;

    switch (stmt->type) {
    case STMT_EXPR:
        if (stmt->data.expr_stmt.expr)
            { char *t = analyze_expr(a, stmt->data.expr_stmt.expr); free(t); }
        break;

    case STMT_RETURN:
        if (stmt->data.return_stmt.value) {
            char *vt = analyze_expr(a, stmt->data.return_stmt.value);
            if (vt && a->current_func_return &&
                !types_compatible(a->current_func_return, vt)) {
                sem_error(a, stmt->loc,
                    "return type mismatch: function returns '%s', got '%s'",
                    a->current_func_return, vt);
            }
            free(vt);
        } else if (a->current_func_return &&
                   strcmp(a->current_func_return, "void") != 0) {
            sem_error(a, stmt->loc,
                "function '%s' must return a value of type '%s'",
                a->current_func_name ? a->current_func_name : "?",
                a->current_func_return);
        }
        break;

    case STMT_BLOCK:
        analyze_block(a, stmt);
        break;

    case STMT_VAR_DECL: {
        const char *name = stmt->data.var_decl.name;
        const char *ty = stmt->data.var_decl.type_name;

        if (scope_lookup_current(a->scope, name)) {
            sem_error(a, stmt->loc, "redeclaration of '%s'", name);
            break;
        }

        if (stmt->data.var_decl.init) {
            char *it = analyze_expr(a, stmt->data.var_decl.init);
            if (it && ty && !types_compatible(ty, it)) {
                sem_error(a, stmt->loc,
                    "type mismatch in initialization of '%s': expected '%s', got '%s'",
                    name, ty, it);
            }
            free(it);
        }

        scope_insert(a->scope, name, ty, stmt->data.var_decl.is_ptr, 0, NULL, stmt->loc);
        break;
    }

    case STMT_IF:
        if (stmt->data.if_stmt.condition) {
            char *ct = analyze_expr(a, stmt->data.if_stmt.condition);
            if (ct && !types_equal(ct, "bool") && !is_numeric_type(ct)) {
                sem_error(a, stmt->loc, "if condition must be bool or numeric, got '%s'", ct);
            }
            free(ct);
        }
        analyze_block(a, stmt->data.if_stmt.then_block);
        for (ElifClause *ec = stmt->data.if_stmt.elifs; ec; ec = ec->next) {
            if (ec->condition) {
                char *ct = analyze_expr(a, ec->condition);
                free(ct);
            }
            analyze_block(a, ec->body);
        }
        analyze_block(a, stmt->data.if_stmt.else_block);
        break;

    case STMT_WHILE:
        if (stmt->data.while_stmt.condition) {
            char *ct = analyze_expr(a, stmt->data.while_stmt.condition);
            free(ct);
        }
        a->loop_depth++;
        analyze_block(a, stmt->data.while_stmt.body);
        a->loop_depth--;
        break;

    case STMT_FOR: {
        SemScope *for_scope = scope_new(a->scope);
        a->scope = for_scope;

        if (stmt->data.for_stmt.init)
            analyze_stmt(a, stmt->data.for_stmt.init);

        if (stmt->data.for_stmt.condition) {
            char *ct = analyze_expr(a, stmt->data.for_stmt.condition);
            free(ct);
        }

        if (stmt->data.for_stmt.increment) {
            analyze_stmt(a, stmt->data.for_stmt.increment);
        }

        a->loop_depth++;
        analyze_block(a, stmt->data.for_stmt.body);
        a->loop_depth--;

        a->scope = for_scope->parent;
        scope_free(for_scope);
        break;
    }

    case STMT_DO_WHILE:
        a->loop_depth++;
        analyze_block(a, stmt->data.do_while_stmt.body);
        a->loop_depth--;
        if (stmt->data.do_while_stmt.condition) {
            char *ct = analyze_expr(a, stmt->data.do_while_stmt.condition);
            free(ct);
        }
        break;

    case STMT_BREAK:
        if (a->loop_depth == 0) {
            sem_error(a, stmt->loc, "break outside of loop");
        }
        break;

    case STMT_CONTINUE:
        if (a->loop_depth == 0) {
            sem_error(a, stmt->loc, "continue outside of loop");
        }
        break;
    }
}

static void analyze_stmt_list(SemanticAnalyzer *a, StmtList *list) {
    for (StmtList *sl = list; sl; sl = sl->next) {
        analyze_stmt(a, sl->stmt);
    }
}

/* ── Declaration analysis ──────────────────────────────────── */

static void check_unused_vars(SemanticAnalyzer *a, SemScope *scope) {
    for (int i = 0; i < BUCKETS; i++) {
        for (Symbol *sym = scope->buckets[i]; sym; sym = sym->next) {
            if (!sym->is_func && !sym->used && sym->type_name) {
                sem_warn(a, sym->loc, "variable '%s' is unused", sym->name);
            }
        }
    }
}

static void analyze_func_decl(SemanticAnalyzer *a, Decl *decl) {
    FuncDecl *f = &decl->data.func;

    Symbol *existing = scope_lookup_current(a->scope, f->name);
    if (existing) {
        sem_error(a, decl->loc, "redeclaration of function '%s'", f->name);
        return;
    }

    scope_insert(a->scope, f->name, f->return_type, 0, 1, f->params, decl->loc);

    if (f->body) {
        SemScope *func_scope = scope_new(a->scope);
        SemScope *prev_scope = a->scope;
        a->scope = func_scope;

        char *prev_func_name = a->current_func_name;
        char *prev_func_return = a->current_func_return;
        a->current_func_name = strdup(f->name);
        a->current_func_return = f->return_type ? strdup(f->return_type) : NULL;

        for (FuncParamList *pl = f->params; pl; pl = pl->next) {
            if (scope_lookup_current(a->scope, pl->param.name)) {
                sem_error(a, decl->loc, "redeclaration of parameter '%s'",
                          pl->param.name);
            } else {
                scope_insert(a->scope, pl->param.name, pl->param.type_name,
                             pl->param.is_ptr, 0, NULL, decl->loc);
                Symbol *psym = scope_lookup_current(a->scope, pl->param.name);
                if (psym) psym->used = 1;
            }
        }

        analyze_stmt_list(a, f->body->data.block.stmts);

        check_unused_vars(a, func_scope);

        free(a->current_func_name);
        free(a->current_func_return);
        a->current_func_name = prev_func_name;
        a->current_func_return = prev_func_return;

        a->scope = prev_scope;
        scope_free(func_scope);
    }
}

static void analyze_struct_decl(SemanticAnalyzer *a, Decl *decl) {
    StructDecl *s = &decl->data.struct_decl;

    Symbol *existing = scope_lookup_current(a->scope, s->name);
    if (existing) {
        sem_error(a, decl->loc, "redeclaration of struct '%s'", s->name);
        return;
    }

    for (StructFieldList *fl = s->fields; fl; fl = fl->next) {
        for (StructFieldList *fl2 = fl->next; fl2; fl2 = fl2->next) {
            if (strcmp(fl->field.name, fl2->field.name) == 0) {
                sem_error(a, decl->loc, "duplicate field '%s' in struct '%s'",
                          fl->field.name, s->name);
            }
        }
    }

    scope_insert(a->scope, s->name, s->name, 0, 0, NULL, decl->loc);

    registry_add_struct(a->types, s->name, s->fields,
                        s->generic_params, s->methods);

    char *prev_struct_name = a->current_struct_name;
    a->current_struct_name = strdup(s->name);

    for (DeclList *ml = s->methods; ml; ml = ml->next) {
        if (ml->decl->type == DECL_FUNC) {
            FuncDecl *m = &ml->decl->data.func;
            char full_name[256];
            snprintf(full_name, sizeof(full_name), "%s.%s", s->name, m->name);
            scope_insert(a->scope, full_name, m->return_type, 0, 1,
                         m->params, ml->decl->loc);
        }
    }

    for (DeclList *ml = s->methods; ml; ml = ml->next) {
        analyze_decl(a, ml->decl);
    }

    free(a->current_struct_name);
    a->current_struct_name = prev_struct_name;
}

static void analyze_interface_decl(SemanticAnalyzer *a, Decl *decl) {
    InterfaceDecl *iface = &decl->data.interface;

    Symbol *existing = scope_lookup_current(a->scope, iface->name);
    if (existing) {
        sem_error(a, decl->loc, "redeclaration of interface '%s'", iface->name);
        return;
    }

    scope_insert(a->scope, iface->name, iface->name, 0, 0, NULL, decl->loc);

    registry_add_interface(a->types, iface->name, iface->methods,
                           iface->generic_params);
}

static void analyze_decl(SemanticAnalyzer *a, Decl *decl) {
    if (!decl) return;

    switch (decl->type) {
    case DECL_FUNC:
        analyze_func_decl(a, decl);
        break;
    case DECL_STRUCT:
        analyze_struct_decl(a, decl);
        break;
    case DECL_INTERFACE:
        analyze_interface_decl(a, decl);
        break;
    case DECL_IMPORT:
        break;
    default:
        break;
    }
}

/* ── Program analysis ──────────────────────────────────────── */

bool semantic_analyze(SemanticAnalyzer *a, Program *program) {
    if (!program) return true;

    for (DeclList *dl = program->decls; dl; dl = dl->next) {
        analyze_decl(a, dl->decl);
    }

    return !a->had_error;
}
