#include "../../include/semantic/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static bool is_assignment_op(TokenType op) {
    return op == Equals || op == PlusEquals || op == MinusEquals ||
           op == StarEquals || op == SlashEquals || op == AmpersandEquals ||
           op == PercentEquals || op == PipeEquals || op == CaretEquals;
}

/* ── Hash table / symbol table ──────────────────────────────── */

#define BUCKETS 64

typedef struct Symbol {
    char *name;
    char *type_name;
    int is_ptr;
    int is_func;
    int is_variadic;
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
                        int is_ptr, int is_func, int is_variadic, FuncParamList *params,
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
    sym->is_variadic = is_variadic;
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
    int switch_depth;
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

static char *resolve_alias_type(SemanticAnalyzer *a, const char *type);

static int is_condition_type(const char *name) {
    if (!name) return 0;
    size_t len = strlen(name);
    return strcmp(name, "bool") == 0 || is_numeric_type(name) ||
           (len > 0 && name[len - 1] == '*');
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

static char *type_make_array(const char *type, int size) {
    if (!type || size <= 0) return NULL;
    int digits = snprintf(NULL, 0, "%d", size);
    size_t len = strlen(type);
    char *result = malloc(len + (size_t)digits + 3);
    sprintf(result, "%s[%d]", type, size);
    return result;
}

static int type_is_array(const char *type) {
    if (!type) return 0;
    const char *lb = strrchr(type, '[');
    size_t len = strlen(type);
    return lb && len > 2 && type[len - 1] == ']';
}

static char *type_array_elem(const char *type) {
    if (!type_is_array(type)) return NULL;
    const char *lb = strrchr(type, '[');
    size_t len = (size_t)(lb - type);
    char *result = malloc(len + 1);
    memcpy(result, type, len);
    result[len] = '\0';
    return result;
}

static int type_name_known(SemanticAnalyzer *a, const char *type) {
    if (!type) return 0;
    if (type_is_array(type)) {
        char *elem = type_array_elem(type);
        int known = type_name_known(a, elem);
        free(elem);
        return known;
    }
    if (type_is_ptr(type)) return 1;
    if (strcmp(type, "void") == 0 || strcmp(type, "char") == 0 ||
        strcmp(type, "str") == 0 || strcmp(type, "bool") == 0 ||
        is_numeric_type(type))
        return 1;
    if (strcmp(type, "self") == 0 && a->current_struct_name)
        return 1;
    if (registry_find_struct(a->types, type))
        return 1;
    Symbol *sym = scope_lookup(a->scope, type);
    if (sym && sym->type_name) {
        sym->used = 1;
        if (strcmp(sym->type_name, type) == 0)
            return 1;
        return type_name_known(a, sym->type_name);
    }
    return 0;
}

static int is_receiver_param(SemanticAnalyzer *a, FuncParamList *pl) {
    if (!a || !a->current_struct_name || !pl) return 0;
    if (!pl->param.name || strcmp(pl->param.name, "self") != 0) return 0;
    char *resolved = resolve_alias_type(a, pl->param.type_name);
    char *expected = type_make_ptr(a->current_struct_name);
    int ok = resolved && expected && strcmp(resolved, expected) == 0;
    free(resolved);
    free(expected);
    return ok;
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
        return 1;
    }

    if (type_is_ptr(expected) && strcmp(actual, "void") == 0)
        return 1;
    if (type_is_ptr(actual) && strcmp(expected, "void") == 0)
        return 1;

    if (type_is_ptr(expected) && type_is_array(actual)) {
        char *elem = type_array_elem(actual);
        char *elem_ptr = type_make_ptr(elem);
        int ok = elem_ptr && types_equal(expected, elem_ptr);
        free(elem);
        free(elem_ptr);
        return ok;
    }

    return 0;
}

static char *resolve_alias_type(SemanticAnalyzer *a, const char *type) {
    if (!type) return NULL;
    if (type_is_ptr(type)) {
        char *base = type_deref(type);
        char *resolved_base = resolve_alias_type(a, base);
        char *result = type_make_ptr(resolved_base ? resolved_base : base);
        free(base);
        free(resolved_base);
        return result;
    }
    if (strcmp(type, "self") == 0 && a->current_struct_name)
        return strdup(a->current_struct_name);
    Symbol *sym = scope_lookup(a->scope, type);
    if (sym && sym->type_name && strcmp(sym->type_name, type) != 0)
        return strdup(sym->type_name);
    return strdup(type);
}

/* ── Forward declarations ──────────────────────────────────── */

static char *analyze_expr(SemanticAnalyzer *a, Expr *expr);
static void analyze_stmt(SemanticAnalyzer *a, Stmt *stmt);
static void analyze_stmt_list(SemanticAnalyzer *a, StmtList *list);
static void analyze_decl(SemanticAnalyzer *a, Decl *decl);

static StructField *current_struct_field(SemanticAnalyzer *a, const char *name) {
    if (!a || !a->current_struct_name || !name) return NULL;
    StructInfo *si = registry_find_struct(a->types, a->current_struct_name);
    if (!si) return NULL;
    for (StructFieldList *fl = si->fields; fl; fl = fl->next) {
        if (strcmp(fl->field.name, name) == 0)
            return &fl->field;
    }
    return NULL;
}

static char *struct_field_type_name(StructField *field) {
    if (!field) return NULL;
    if (field->array_size > 0)
        return type_make_array(field->type_name, field->array_size);
    return field->type_name ? strdup(field->type_name) : NULL;
}

static int expr_list_count(ExprList *list) {
    int count = 0;
    for (ExprList *el = list; el; el = el->next) count++;
    return count;
}

static void analyze_array_initializer(SemanticAnalyzer *a, Expr *init,
                                      const char *elem_type, int array_size,
                                      SourceLocation loc, const char *name) {
    if (!init || array_size <= 0) return;
    if (init->type != EXPR_ARRAY_LIT) {
        sem_error(a, loc, "array '%s' initializer must be an array literal", name);
        return;
    }

    int count = expr_list_count(init->data.array_lit.elements);
    if (count > array_size) {
        sem_error(a, loc, "array '%s' initializer has %d elements, size is %d",
                  name, count, array_size);
    }

    int idx = 0;
    for (ExprList *el = init->data.array_lit.elements; el; el = el->next, idx++) {
        char *et = analyze_expr(a, el->expr);
        if (idx < array_size && et && elem_type && !types_compatible(elem_type, et)) {
            sem_error(a, el->expr ? el->expr->loc : loc,
                "array '%s' element %d: expected '%s', got '%s'",
                name, idx, elem_type, et);
        }
        free(et);
    }
}

static void analyze_struct_initializer(SemanticAnalyzer *a, Expr *init,
                                       const char *struct_type,
                                       SourceLocation loc, const char *name) {
    if (!init || !struct_type) return;
    if (init->type != EXPR_ARRAY_LIT) return;

    StructInfo *si = registry_find_struct(a->types, struct_type);
    if (!si) return;

    int field_count = 0;
    for (StructFieldList *fl = si->fields; fl; fl = fl->next) field_count++;
    int init_count = expr_list_count(init->data.array_lit.elements);
    if (init_count > field_count) {
        sem_error(a, loc, "struct '%s' initializer for '%s' has %d fields, struct has %d",
                  struct_type, name, init_count, field_count);
    }

    ExprList *el = init->data.array_lit.elements;
    StructFieldList *fl = si->fields;
    int idx = 0;
    while (el && fl) {
        char *et = analyze_expr(a, el->expr);
        char *ft = struct_field_type_name(&fl->field);
        if (et && ft && !types_compatible(ft, et)) {
            sem_error(a, el->expr ? el->expr->loc : loc,
                "struct '%s' initializer field %d '%s': expected '%s', got '%s'",
                struct_type, idx, fl->field.name, ft, et);
        }
        free(et);
        free(ft);
        el = el->next;
        fl = fl->next;
        idx++;
    }
}

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

    case EXPR_BOOL_LIT:
        return strdup("bool");

    case EXPR_NULL:
        return strdup("void");

    case EXPR_SIZEOF:
        if (!type_name_known(a, expr->data.sizeof_expr.type_name)) {
            sem_error(a, expr->loc, "sizeof unknown type or value '%s'",
                      expr->data.sizeof_expr.type_name);
            return NULL;
        }
        return strdup("i64");

    case EXPR_IDENTIFIER: {
        const char *name = expr->data.identifier.name;
        Symbol *sym = scope_lookup(a->scope, name);
        if (!sym) {
            StructField *field = current_struct_field(a, name);
            if (field)
                return struct_field_type_name(field);
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

            if (lhs->type == EXPR_INDEX) {
                char *lt = analyze_expr(a, lhs);
                if (!lt) { free(rt); return NULL; }
                if (!types_compatible(lt, rt)) {
                    sem_error(a, expr->loc, "type mismatch in index assignment: '%s' = '%s'", lt, rt);
                    free(lt); free(rt); return NULL;
                }
                { char *result = strdup(lt); free(lt); free(rt); return result; }
            }

            sem_error(a, expr->loc, "assignment target is not assignable");
            free(rt); return NULL;
        }

        if (is_assignment_op(op) && op != Equals) {
            Expr *lhs = expr->data.binary.left;
            char *lt = analyze_expr(a, lhs);
            char *rt = analyze_expr(a, expr->data.binary.right);
            if (!lt || !rt) { free(lt); free(rt); return NULL; }
            if (!types_compatible(lt, rt)) {
                sem_error(a, expr->loc, "type mismatch in compound assignment: '%s' and '%s'", lt, rt);
                free(lt); free(rt); return NULL;
            }
            if (!is_numeric_type(lt)) {
                sem_error(a, expr->loc, "compound assignment requires numeric types, got '%s'", lt);
                free(lt); free(rt); return NULL;
            }
            { char *result = strdup(lt); free(lt); free(rt); return result; }
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

        case EqualsEquals: case BangEquals:
            if (!types_compatible(lt, rt)) {
                sem_error(a, expr->loc, "comparison between incompatible types '%s' and '%s'", lt, rt);
                free(lt); free(rt); return NULL;
            }
            { char *result = strdup("i32"); free(lt); free(rt); return result; }

        case Less: case LessEquals: case Greater: case GreaterEquals:
            if (!is_numeric_type(lt) || !is_numeric_type(rt)) {
                sem_error(a, expr->loc, "relational operator requires numeric types");
                free(lt); free(rt); return NULL;
            }
            { char *result = strdup("i32"); free(lt); free(rt); return result; }

        case AmpersandAmpersand: case PipePipe:
            if (!is_condition_type(lt) || !is_condition_type(rt)) {
                sem_error(a, expr->loc, "logical operator requires bool or numeric types");
                free(lt); free(rt); return NULL;
            }
            { char *result = strdup("i32"); free(lt); free(rt); return result; }

        case Ampersand: case Caret: case Pipe:
        case LeftShift: case RightShift:
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
        case Tilde:
            if (!is_numeric_type(ot)) {
                sem_error(a, expr->loc, "bitwise NOT applied to non-numeric type '%s'", ot);
                free(ot); return NULL;
            }
            return ot;
        default:
            return ot;
        }
    }

    case EXPR_CALL: {
        Expr *callee = expr->data.call.callee;
        if (callee->type == EXPR_MEMBER) {
            char *objt = analyze_expr(a, callee->data.member.object);
            if (!objt) return NULL;

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

            StructInfo *si = registry_find_struct(a->types, base);
            if (!si) {
                sem_error(a, expr->loc, "cannot call method on non-struct type '%s'", base);
                return NULL;
            }

            Decl *method = NULL;
            for (DeclList *ml = si->methods; ml; ml = ml->next) {
                if (ml->decl && ml->decl->type == DECL_FUNC &&
                    strcmp(ml->decl->data.func.name, callee->data.member.field) == 0) {
                    method = ml->decl;
                    break;
                }
            }
            if (!method) {
                sem_error(a, expr->loc, "no method '%s' in struct '%s'",
                          callee->data.member.field, base);
                return NULL;
            }

            char full_name[256];
            snprintf(full_name, sizeof(full_name), "%s.%s", base, callee->data.member.field);
            Symbol *msym = scope_lookup(a->scope, full_name);

            FuncParamList *params = method->data.func.params;
            int has_explicit_receiver = params && params->param.name &&
                strcmp(params->param.name, "self") == 0;
            int param_count = 0;
            for (FuncParamList *pl = has_explicit_receiver ? params->next : params;
                 pl; pl = pl->next)
                param_count++;

            int argc = 0;
            for (ExprList *al = expr->data.call.args; al; al = al->next) argc++;

            if (argc != param_count && !method->data.func.is_variadic) {
                sem_error(a, expr->loc,
                    "method '%s' expects %d arguments, got %d",
                    full_name, param_count, argc);
            } else {
                ExprList *al = expr->data.call.args;
                FuncParamList *pl = has_explicit_receiver ? params->next : params;
                int idx = 0;
                while (al && pl) {
                    char *at = analyze_expr(a, al->expr);
                    char *expected = resolve_alias_type(a, pl->param.type_name);
                    if (at && expected && !types_compatible(expected, at)) {
                        sem_error(a, expr->loc,
                            "argument %d of '%s': expected '%s', got '%s'",
                            idx + 1, full_name, expected, at);
                    }
                    free(expected);
                    free(at);
                    al = al->next;
                    pl = pl->next;
                    idx++;
                }
            }

            if (msym && msym->type_name)
                return strdup(msym->type_name);
            return method->data.func.return_type ? strdup(method->data.func.return_type) : NULL;
        }

        if (callee->type != EXPR_IDENTIFIER) {
            sem_error(a, expr->loc, "invalid call target");
            return NULL;
        }
        const char *fname = callee->data.identifier.name;
        Symbol *fsym = scope_lookup(a->scope, fname);
        int implicit_method_call = 0;
        Decl *implicit_method = NULL;
        char method_name[256];
        if (!fsym) {
            if (a->current_struct_name) {
                snprintf(method_name, sizeof(method_name), "%s.%s",
                         a->current_struct_name, fname);
                fsym = scope_lookup(a->scope, method_name);
                StructInfo *si = registry_find_struct(a->types, a->current_struct_name);
                if (si) {
                    for (DeclList *ml = si->methods; ml; ml = ml->next) {
                        if (ml->decl && ml->decl->type == DECL_FUNC &&
                            strcmp(ml->decl->data.func.name, fname) == 0) {
                            implicit_method = ml->decl;
                            break;
                        }
                    }
                }
                implicit_method_call = fsym && implicit_method;
            }
            if (!fsym) {
                sem_error(a, expr->loc, "undeclared function '%s'", fname);
                return NULL;
            }
        }
        if (!fsym->is_func) {
            sem_error(a, expr->loc, "'%s' is not a function", fname);
            return NULL;
        }

        int argc = 0;
        for (ExprList *al = expr->data.call.args; al; al = al->next) argc++;

        FuncParamList *params = fsym->params;
        if (implicit_method_call && params && params->param.name &&
            strcmp(params->param.name, "self") == 0)
            params = params->next;

        int param_count = 0;
        for (FuncParamList *pl = params; pl; pl = pl->next) param_count++;

        if (argc != param_count && !fsym->is_variadic) {
            sem_error(a, expr->loc,
                "function '%s' expects %d arguments, got %d",
                implicit_method_call ? method_name : fname, param_count, argc);
        } else {
            ExprList *al = expr->data.call.args;
            FuncParamList *pl = params;
            int idx = 0;
            while (al && pl) {
                char *at = analyze_expr(a, al->expr);
                char *expected = resolve_alias_type(a, pl->param.type_name);
                if (at && expected && !types_compatible(expected, at)) {
                    sem_error(a, expr->loc,
                        "argument %d of '%s': expected '%s', got '%s'",
                        idx + 1, implicit_method_call ? method_name : fname, expected, at);
                }
                free(expected);
                free(at);
                al = al->next;
                pl = pl->next;
                idx++;
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
                return struct_field_type_name(&fl->field);
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
            StructField *field = current_struct_field(a, expr->data.assign.name);
            if (field) {
                char *vt = analyze_expr(a, expr->data.assign.value);
                if (!vt) return NULL;
                char *field_type = struct_field_type_name(field);
                if (!types_compatible(field_type, vt)) {
                    sem_error(a, expr->loc, "type mismatch in assignment to field '%s': '%s' = '%s'",
                              expr->data.assign.name, field_type, vt);
                    free(field_type); free(vt); return NULL;
                }
                char *result = field_type ? strdup(field_type) : NULL;
                free(field_type);
                free(vt);
                return result;
            }
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
        if (type_is_array(at)) {
            char *result = type_array_elem(at); free(at); return result;
        }
        if (type_is_ptr(at)) {
            char *result = type_deref(at); free(at); return result;
        }
        free(at);
        return NULL;
    }
    case EXPR_NEW: {
        for (ExprList *dl = expr->data.new_expr.dims; dl; dl = dl->next) {
            char *dt = analyze_expr(a, dl->expr);
            free(dt);
        }
        char *base = strdup(expr->data.new_expr.type_name);
        char *result = malloc(strlen(base) + 2);
        sprintf(result, "%s*", base);
        free(base);
        return result;
    }
    case EXPR_DELETE: {
        char *et = analyze_expr(a, expr->data.delete_expr.operand);
        free(et);
        return NULL;
    }

    case EXPR_ARRAY_LIT:
        for (ExprList *el = expr->data.array_lit.elements; el; el = el->next) {
            char *et = analyze_expr(a, el->expr);
            free(et);
        }
        return strdup("array");

    case EXPR_CONDITIONAL: {
        char *ct = analyze_expr(a, expr->data.conditional.condition);
        if (ct && !is_condition_type(ct)) {
            sem_error(a, expr->data.conditional.condition->loc,
                      "conditional expression condition must be bool or numeric, got '%s'", ct);
        }
        char *tt = analyze_expr(a, expr->data.conditional.then_expr);
        char *et = analyze_expr(a, expr->data.conditional.else_expr);
        free(ct);
        if (!tt || !et) { free(tt); free(et); return NULL; }
        if (!types_compatible(tt, et) && !types_compatible(et, tt)) {
            sem_error(a, expr->loc,
                      "conditional branches have incompatible types '%s' and '%s'", tt, et);
            free(tt); free(et); return NULL;
        }
        char *result = strdup(tt);
        free(tt);
        free(et);
        return result;
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
        char *resolved_ty = resolve_alias_type(a, ty);
        const char *effective_ty = resolved_ty ? resolved_ty : ty;

        if (scope_lookup_current(a->scope, name)) {
            sem_error(a, stmt->loc, "redeclaration of '%s'", name);
            free(resolved_ty);
            break;
        }

        char *decl_ty = NULL;
        if (stmt->data.var_decl.array_size > 0) {
            decl_ty = type_make_array(effective_ty, stmt->data.var_decl.array_size);
            if (stmt->data.var_decl.init)
                analyze_array_initializer(a, stmt->data.var_decl.init, effective_ty,
                                          stmt->data.var_decl.array_size, stmt->loc, name);
        }
        const char *stored_ty = decl_ty ? decl_ty : effective_ty;

        if (stmt->data.var_decl.init && stmt->data.var_decl.array_size == 0 &&
            registry_find_struct(a->types, effective_ty) &&
            stmt->data.var_decl.init->type == EXPR_ARRAY_LIT) {
            analyze_struct_initializer(a, stmt->data.var_decl.init, effective_ty,
                                       stmt->loc, name);
        } else if (stmt->data.var_decl.init && stmt->data.var_decl.array_size == 0) {
            char *it = analyze_expr(a, stmt->data.var_decl.init);
            if (it && effective_ty && !types_compatible(effective_ty, it)) {
                sem_error(a, stmt->loc,
                    "type mismatch in initialization of '%s': expected '%s', got '%s'",
                    name, effective_ty, it);
            }
            free(it);
        }

        scope_insert(a->scope, name, stored_ty, stmt->data.var_decl.is_ptr, 0, 0, NULL, stmt->loc);
        free(decl_ty);
        free(resolved_ty);
        break;
    }

    case STMT_IF:
        if (stmt->data.if_stmt.condition) {
            char *ct = analyze_expr(a, stmt->data.if_stmt.condition);
            if (ct && !is_condition_type(ct)) {
                sem_error(a, stmt->loc, "if condition must be bool or numeric, got '%s'", ct);
            }
            free(ct);
        }
        analyze_block(a, stmt->data.if_stmt.then_block);
        for (ElifClause *ec = stmt->data.if_stmt.elifs; ec; ec = ec->next) {
            if (ec->condition) {
                char *ct = analyze_expr(a, ec->condition);
                if (ct && !is_condition_type(ct)) {
                    sem_error(a, ec->condition->loc, "elif condition must be bool or numeric, got '%s'", ct);
                }
                free(ct);
            }
            analyze_block(a, ec->body);
        }
        analyze_block(a, stmt->data.if_stmt.else_block);
        break;

    case STMT_SWITCH: {
        char *st = analyze_expr(a, stmt->data.switch_stmt.expr);
        if (st && !is_numeric_type(st) && !types_equal(st, "char")) {
            sem_error(a, stmt->loc, "switch expression must be numeric or char, got '%s'", st);
        }

        a->switch_depth++;
        for (SwitchCase *sc = stmt->data.switch_stmt.cases; sc; sc = sc->next) {
            char *ct = analyze_expr(a, sc->value);
            if (st && ct && !types_compatible(st, ct)) {
                sem_error(a, sc->value->loc,
                          "case value type '%s' is not compatible with switch type '%s'",
                          ct, st);
            }
            free(ct);
            analyze_stmt_list(a, sc->stmts);
        }
        analyze_stmt_list(a, stmt->data.switch_stmt.default_stmts);
        a->switch_depth--;
        free(st);
        break;
    }

    case STMT_WHILE:
        if (stmt->data.while_stmt.condition) {
            char *ct = analyze_expr(a, stmt->data.while_stmt.condition);
            if (ct && !is_condition_type(ct)) {
                sem_error(a, stmt->loc, "while condition must be bool or numeric, got '%s'", ct);
            }
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
            if (ct && !is_condition_type(ct)) {
                sem_error(a, stmt->data.for_stmt.condition->loc,
                          "for condition must be bool or numeric, got '%s'", ct);
            }
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
            if (ct && !is_condition_type(ct)) {
                sem_error(a, stmt->data.do_while_stmt.condition->loc,
                          "do while condition must be bool or numeric, got '%s'", ct);
            }
            free(ct);
        }
        break;

    case STMT_BREAK:
        if (a->loop_depth == 0 && a->switch_depth == 0) {
            sem_error(a, stmt->loc, "break outside of loop or switch");
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
    char scoped_name[256];
    const char *symbol_name = f->name;
    if (a->current_struct_name) {
        snprintf(scoped_name, sizeof(scoped_name), "%s.%s", a->current_struct_name, f->name);
        symbol_name = scoped_name;
    }

    Symbol *existing = scope_lookup_current(a->scope, symbol_name);
    if (existing && !existing->is_func) {
        sem_error(a, decl->loc, "redeclaration of function '%s'", symbol_name);
        return;
    }

    char *resolved_return = resolve_alias_type(a, f->return_type);
    if (!existing) {
        scope_insert(a->scope, symbol_name, resolved_return ? resolved_return : f->return_type,
                     0, 1, f->is_variadic, f->params, decl->loc);
    }

    if (f->body) {
        SemScope *func_scope = scope_new(a->scope);
        SemScope *prev_scope = a->scope;
        a->scope = func_scope;

        char *prev_func_name = a->current_func_name;
        char *prev_func_return = a->current_func_return;
        a->current_func_name = strdup(f->name);
        a->current_func_return = resolved_return ? strdup(resolved_return) :
            (f->return_type ? strdup(f->return_type) : NULL);

        int has_explicit_receiver = is_receiver_param(a, f->params);
        if (a->current_struct_name && !has_explicit_receiver) {
            char *self_ty = type_make_ptr(a->current_struct_name);
            scope_insert(a->scope, "self", self_ty, 1, 0, 0, NULL, decl->loc);
            free(self_ty);
            Symbol *self_sym = scope_lookup_current(a->scope, "self");
            if (self_sym) self_sym->used = 1;
        }

        for (FuncParamList *pl = f->params; pl; pl = pl->next) {
            if (scope_lookup_current(a->scope, pl->param.name)) {
                sem_error(a, decl->loc, "redeclaration of parameter '%s'",
                          pl->param.name);
            } else {
                char *resolved_param = resolve_alias_type(a, pl->param.type_name);
                scope_insert(a->scope, pl->param.name,
                             resolved_param ? resolved_param : pl->param.type_name,
                             pl->param.is_ptr, 0, 0, NULL, decl->loc);
                free(resolved_param);
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
    free(resolved_return);
}

static int is_global_const_expr(Expr *expr) {
    if (!expr) return 1;
    switch (expr->type) {
    case EXPR_INT_LIT:
    case EXPR_FLOAT_LIT:
    case EXPR_CHAR_LIT:
    case EXPR_STRING_LIT:
    case EXPR_BOOL_LIT:
    case EXPR_NULL:
    case EXPR_IDENTIFIER:
    case EXPR_SIZEOF:
        return 1;
    case EXPR_UNARY:
        return expr->data.unary.op == Minus &&
               is_global_const_expr(expr->data.unary.operand);
    case EXPR_ARRAY_LIT:
        for (ExprList *el = expr->data.array_lit.elements; el; el = el->next) {
            if (!is_global_const_expr(el->expr))
                return 0;
        }
        return 1;
    default:
        return 0;
    }
}

static void declare_global_var_decl(SemanticAnalyzer *a, Decl *decl) {
    GlobalVarDecl *g = &decl->data.global_var;
    if (scope_lookup_current(a->scope, g->name)) {
        sem_error(a, decl->loc, "redeclaration of global variable '%s'", g->name);
        return;
    }

    char *resolved_type = resolve_alias_type(a, g->type_name);
    const char *effective = resolved_type ? resolved_type : g->type_name;
    char *decl_ty = g->array_size > 0 ? type_make_array(effective, g->array_size) : NULL;
    scope_insert(a->scope, g->name, decl_ty ? decl_ty : effective,
                 g->is_ptr, 0, 0, NULL, decl->loc);
    free(decl_ty);
    free(resolved_type);
}

static void analyze_global_var_decl(SemanticAnalyzer *a, Decl *decl) {
    GlobalVarDecl *g = &decl->data.global_var;
    if (!is_global_const_expr(g->init)) {
        sem_error(a, decl->loc, "global variable '%s' requires a constant initializer", g->name);
        return;
    }

    if (g->array_size > 0 && g->init) {
        char *expected = resolve_alias_type(a, g->type_name);
        const char *effective = expected ? expected : g->type_name;
        analyze_array_initializer(a, g->init, effective, g->array_size, decl->loc, g->name);
        free(expected);
        return;
    }

    if (g->init) {
        char *it = analyze_expr(a, g->init);
        char *expected = resolve_alias_type(a, g->type_name);
        const char *effective = expected ? expected : g->type_name;
        if (it && effective && !types_compatible(effective, it)) {
            sem_error(a, decl->loc,
                "type mismatch in initialization of global '%s': expected '%s', got '%s'",
                g->name, effective, it);
        }
        free(expected);
        free(it);
    }
}

static void declare_func_decl(SemanticAnalyzer *a, Decl *decl) {
    FuncDecl *f = &decl->data.func;

    if (scope_lookup_current(a->scope, f->name)) {
        sem_error(a, decl->loc, "redeclaration of function '%s'", f->name);
        return;
    }

    char *resolved_return = resolve_alias_type(a, f->return_type);
    scope_insert(a->scope, f->name, resolved_return ? resolved_return : f->return_type,
                 0, 1, f->is_variadic, f->params, decl->loc);
    free(resolved_return);
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

    scope_insert(a->scope, s->name, s->name, 0, 0, 0, NULL, decl->loc);

    registry_add_struct(a->types, s->name, s->fields,
                        s->generic_params, s->methods);

    char *prev_struct_name = a->current_struct_name;
    a->current_struct_name = strdup(s->name);

    for (DeclList *ml = s->methods; ml; ml = ml->next) {
        if (ml->decl->type == DECL_FUNC) {
            FuncDecl *m = &ml->decl->data.func;
            char full_name[256];
            snprintf(full_name, sizeof(full_name), "%s.%s", s->name, m->name);
            scope_insert(a->scope, full_name, m->return_type, 0, 1, 0,
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

    scope_insert(a->scope, iface->name, iface->name, 0, 0, 0, NULL, decl->loc);

    registry_add_interface(a->types, iface->name, iface->methods,
                           iface->generic_params);
}

static void analyze_type_alias_decl(SemanticAnalyzer *a, Decl *decl) {
    TypeAliasDecl *alias = &decl->data.type_alias;
    if (scope_lookup_current(a->scope, alias->name)) {
        sem_error(a, decl->loc, "redeclaration of type alias '%s'", alias->name);
        return;
    }
    char *target = resolve_alias_type(a, alias->target_type);
    scope_insert(a->scope, alias->name, target ? target : alias->target_type,
                 0, 0, 0, NULL, decl->loc);
    free(target);
}

static void analyze_enum_decl(SemanticAnalyzer *a, Decl *decl) {
    EnumDecl *en = &decl->data.enum_decl;
    if (scope_lookup_current(a->scope, en->name)) {
        sem_error(a, decl->loc, "redeclaration of enum '%s'", en->name);
        return;
    }
    scope_insert(a->scope, en->name, "i32", 0, 0, 0, NULL, decl->loc);
    for (EnumVariantList *v = en->variants; v; v = v->next) {
        if (scope_lookup_current(a->scope, v->name)) {
            sem_error(a, decl->loc, "redeclaration of enum variant '%s'", v->name);
            continue;
        }
        scope_insert(a->scope, v->name, "i32", 0, 0, 0, NULL, decl->loc);
    }
}

static void analyze_decl(SemanticAnalyzer *a, Decl *decl) {
    if (!decl) return;

    switch (decl->type) {
    case DECL_FUNC:
        analyze_func_decl(a, decl);
        break;
    case DECL_GLOBAL_VAR:
        analyze_global_var_decl(a, decl);
        break;
    case DECL_STRUCT:
        analyze_struct_decl(a, decl);
        break;
    case DECL_TYPE_ALIAS:
        analyze_type_alias_decl(a, decl);
        break;
    case DECL_ENUM:
        analyze_enum_decl(a, decl);
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
        if (!dl->decl) continue;
        if (dl->decl->type == DECL_TYPE_ALIAS ||
            dl->decl->type == DECL_ENUM)
            analyze_decl(a, dl->decl);
    }

    for (DeclList *dl = program->decls; dl; dl = dl->next) {
        if (dl->decl && dl->decl->type == DECL_FUNC)
            declare_func_decl(a, dl->decl);
    }

    for (DeclList *dl = program->decls; dl; dl = dl->next) {
        if (dl->decl && dl->decl->type == DECL_GLOBAL_VAR)
            declare_global_var_decl(a, dl->decl);
    }

    for (DeclList *dl = program->decls; dl; dl = dl->next) {
        if (dl->decl && (dl->decl->type == DECL_TYPE_ALIAS ||
                         dl->decl->type == DECL_ENUM))
            continue;
        analyze_decl(a, dl->decl);
    }

    return !a->had_error;
}
