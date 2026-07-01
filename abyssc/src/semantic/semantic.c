#include "../../include/semantic/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static bool is_assignment_op(TokenType op) {
    return op == Equals || op == PlusEquals || op == MinusEquals ||
           op == StarEquals || op == SlashEquals || op == AmpersandEquals ||
           op == PercentEquals || op == PipeEquals || op == CaretEquals ||
           op == LeftShiftEquals || op == RightShiftEquals;
}

static bool is_assignable_expr(Expr *expr) {
    if (!expr) return false;
    return expr->type == EXPR_IDENTIFIER || expr->type == EXPR_MEMBER ||
           expr->type == EXPR_DEREF || expr->type == EXPR_INDEX;
}

/* ── Hash table / symbol table ──────────────────────────────── */

#define BUCKETS 64

typedef struct Symbol {
    char *name;
    char *type_name;
    int is_ptr;
    int is_func;
    int is_variadic;
    int is_const;
    int has_const_int;
    long long const_int_value;
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
    sym->is_const = 0;
    sym->has_const_int = 0;
    sym->const_int_value = 0;
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

static InterfaceInfo *registry_find_interface(TypeRegistry *tr, const char *name) {
    for (int i = 0; i < tr->iface_count; i++) {
        if (strcmp(tr->interfaces[i]->name, name) == 0)
            return tr->interfaces[i];
    }
    return NULL;
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

static int is_integer_type(const char *name) {
    return name && (is_signed_type(name) || is_unsigned_type(name) ||
                    strcmp(name, "char") == 0 || strcmp(name, "bool") == 0);
}

static long long parse_int_literal_value_sem(const char *text) {
    if (!text) return 0;
    int base = 10;
    size_t i = 0;
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        base = 16;
        i = 2;
    } else if (text[0] == '0' && (text[1] == 'b' || text[1] == 'B')) {
        base = 2;
        i = 2;
    } else if (text[0] == '0' && (text[1] == 'o' || text[1] == 'O')) {
        base = 8;
        i = 2;
    }

    long long value = 0;
    for (; text[i]; i++) {
        if (text[i] == '_') continue;
        int digit;
        if (text[i] >= '0' && text[i] <= '9') digit = text[i] - '0';
        else if (text[i] >= 'a' && text[i] <= 'f') digit = 10 + text[i] - 'a';
        else if (text[i] >= 'A' && text[i] <= 'F') digit = 10 + text[i] - 'A';
        else break;
        if (digit >= base) break;
        value = value * base + digit;
    }
    return value;
}

static double parse_float_literal_value_sem(const char *text) {
    if (!text) return 0.0;
    size_t len = strlen(text);
    char *clean = malloc(len + 1);
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (text[i] != '_')
            clean[j++] = text[i];
    }
    clean[j] = '\0';
    double value = strtod(clean, NULL);
    free(clean);
    return value;
}

static unsigned char parse_char_literal_value_sem(const char *text) {
    if (!text || text[0] == '\0') return 0;
    const char *p = text;
    if (p[0] == '\'') p++;
    if (p[0] != '\\') return (unsigned char)p[0];
    switch (p[1]) {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    case '0': return '\0';
    case '\\': return '\\';
    case '\'': return '\'';
    case '"': return '"';
    default: return (unsigned char)p[1];
    }
}

static bool is_bitwise_assignment_op(TokenType op) {
    return op == AmpersandEquals || op == PipeEquals || op == CaretEquals ||
           op == LeftShiftEquals || op == RightShiftEquals;
}

static int call_arg_count_invalid(int argc, int fixed_count, int is_variadic) {
    return is_variadic ? argc < fixed_count : argc != fixed_count;
}

static char *resolve_alias_type(SemanticAnalyzer *a, const char *type);

static int is_condition_type(const char *name) {
    if (!name) return 0;
    size_t len = strlen(name);
    return strcmp(name, "bool") == 0 || is_numeric_type(name) ||
           strcmp(name, "str") == 0 ||
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

static int type_array_size(const char *type) {
    if (!type_is_array(type)) return 0;
    const char *lb = strrchr(type, '[');
    return lb ? atoi(lb + 1) : 0;
}

static int type_name_known(SemanticAnalyzer *a, const char *type) {
    if (!type) return 0;
    if (type_is_array(type)) {
        char *elem = type_array_elem(type);
        int known = type_name_known(a, elem);
        free(elem);
        return known;
    }
    if (type_is_ptr(type)) {
        char *base = type_deref(type);
        int known = type_name_known(a, base);
        free(base);
        return known;
    }
    if (strcmp(type, "void") == 0 || strcmp(type, "char") == 0 ||
        strcmp(type, "str") == 0 || strcmp(type, "bool") == 0 ||
        is_numeric_type(type))
        return 1;
    if (strcmp(type, "self") == 0 && a->current_struct_name)
        return 1;
    if (registry_find_struct(a->types, type))
        return 1;
    if (registry_find_interface(a->types, type))
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

static int type_uses_generics(const char *type) {
    return type && (strchr(type, '<') || strchr(type, '>'));
}

static int reject_generic_params(SemanticAnalyzer *a, SourceLocation loc,
                                 GenericParamList *params, const char *subject,
                                 const char *name) {
    if (!params) return 0;
    sem_error(a, loc, "%s '%s' uses generics, which are not supported yet",
              subject ? subject : "declaration",
              name ? name : "");
    return 1;
}

static int type_is_pointer_like(const char *type) {
    return type && (type_is_ptr(type) || strcmp(type, "str") == 0);
}

static int type_is_numeric_like(const char *type) {
    return type && (is_numeric_type(type) || strcmp(type, "char") == 0 ||
                    strcmp(type, "bool") == 0);
}

static int type_is_aggregate_value(SemanticAnalyzer *a, const char *type) {
    return type && (type_is_array(type) ||
                    registry_find_struct(a->types, type) ||
                    registry_find_interface(a->types, type));
}

static int validate_declared_type(SemanticAnalyzer *a, SourceLocation loc,
                                  const char *type, const char *subject,
                                  int allow_void) {
    if (type_uses_generics(type)) {
        sem_error(a, loc, "generic type '%s' in %s is not supported yet",
                  type ? type : "<null>",
                  subject ? subject : "declaration");
        return 0;
    }
    if (!type_name_known(a, type)) {
        sem_error(a, loc, "unknown type '%s' in %s", type ? type : "<null>",
                  subject ? subject : "declaration");
        return 0;
    }
    if (!allow_void && type && strcmp(type, "void") == 0) {
        sem_error(a, loc, "void type is not allowed in %s",
                  subject ? subject : "declaration");
        return 0;
    }
    return 1;
}

static void validate_param_types(SemanticAnalyzer *a, SourceLocation loc,
                                 FuncParamList *params, const char *owner) {
    for (FuncParamList *pl = params; pl; pl = pl->next) {
        if (!pl->param.name || pl->param.name[0] == '\0') {
            sem_error(a, loc, "missing parameter name in %s",
                      owner ? owner : "function parameter");
        } else {
            for (FuncParamList *next = pl->next; next; next = next->next) {
                if (next->param.name && strcmp(pl->param.name, next->param.name) == 0) {
                    sem_error(a, loc, "duplicate parameter '%s' in %s",
                              pl->param.name,
                              owner ? owner : "function parameter");
                    break;
                }
            }
        }
        char *resolved = resolve_alias_type(a, pl->param.type_name);
        validate_declared_type(a, loc, resolved ? resolved : pl->param.type_name,
                               owner, 0);
        free(resolved);
    }
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
    if (strcmp(expected, "str") == 0 && strcmp(actual, "void") == 0)
        return 1;
    if (strcmp(actual, "str") == 0 && strcmp(expected, "void") == 0)
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

static int param_lists_compatible(FuncParamList *expected, FuncParamList *actual) {
    while (expected && actual) {
        if (!types_compatible(expected->param.type_name, actual->param.type_name))
            return 0;
        expected = expected->next;
        actual = actual->next;
    }
    return expected == NULL && actual == NULL;
}

static FuncParamList *skip_receiver_param(FuncParamList *params) {
    if (params && params->param.name && strcmp(params->param.name, "self") == 0)
        return params->next;
    return params;
}

static int struct_satisfies_interface(SemanticAnalyzer *a,
                                      const char *struct_type,
                                      const char *interface_type) {
    StructInfo *si = registry_find_struct(a->types, struct_type);
    InterfaceInfo *ii = registry_find_interface(a->types, interface_type);
    if (!si || !ii) return 0;

    for (InterfaceMethodList *im = ii->methods; im; im = im->next) {
        Decl *method = NULL;
        for (DeclList *ml = si->methods; ml; ml = ml->next) {
            if (ml->decl && ml->decl->type == DECL_FUNC &&
                !ml->decl->data.func.is_static &&
                strcmp(ml->decl->data.func.name, im->method.name) == 0) {
                method = ml->decl;
                break;
            }
        }
        if (!method) return 0;
        FuncDecl *mf = &method->data.func;
        if (!types_compatible(im->method.return_type, mf->return_type))
            return 0;
        if (im->method.is_variadic != mf->is_variadic)
            return 0;
        if (!param_lists_compatible(im->method.params, skip_receiver_param(mf->params)))
            return 0;
    }
    return 1;
}

static int types_compatible_in_context(SemanticAnalyzer *a,
                                       const char *expected,
                                       const char *actual) {
    if (types_compatible(expected, actual)) return 1;
    if (expected && actual && type_is_ptr(expected) && type_is_ptr(actual)) {
        char *expected_base = type_deref(expected);
        char *actual_base = type_deref(actual);
        int ok = expected_base && actual_base &&
            registry_find_interface(a->types, expected_base) &&
            struct_satisfies_interface(a, actual_base, expected_base);
        free(expected_base);
        free(actual_base);
        return ok;
    }
    return 0;
}

static char *resolve_alias_type(SemanticAnalyzer *a, const char *type) {
    if (!type) return NULL;
    if (type_is_array(type)) {
        char *elem = type_array_elem(type);
        int size = type_array_size(type);
        char *resolved_elem = resolve_alias_type(a, elem);
        char *result = type_make_array(resolved_elem ? resolved_elem : elem, size);
        free(elem);
        free(resolved_elem);
        return result;
    }
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
static int eval_const_int_expr_sem(SemanticAnalyzer *a, Expr *expr, long long *out);
static int stmt_guarantees_return(Stmt *stmt);
static int stmt_guarantees_exit(Stmt *stmt);

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

static int validate_assignable_target(SemanticAnalyzer *a, Expr *expr, const char *context) {
    if (!is_assignable_expr(expr)) {
        sem_error(a, expr->loc, "%s is not assignable", context);
        return 0;
    }

    if (expr->type == EXPR_IDENTIFIER) {
        const char *name = expr->data.identifier.name;
        Symbol *sym = scope_lookup(a->scope, name);
        if (sym && sym->is_func) {
            sem_error(a, expr->loc, "cannot assign to function '%s'", name);
            return 0;
        }
        return 1;
    }

    if (expr->type == EXPR_MEMBER) {
        char *objt = analyze_expr(a, expr->data.member.object);
        if (!objt) return 0;

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

        StructInfo *si = base[0] ? registry_find_struct(a->types, base) : NULL;
        if (!si) {
            sem_error(a, expr->loc, "%s is not a struct field", context);
            return 0;
        }

        const char *field = expr->data.member.field;
        for (StructFieldList *fl = si->fields; fl; fl = fl->next) {
            if (strcmp(fl->field.name, field) == 0)
                return 1;
        }

        for (DeclList *ml = si->methods; ml; ml = ml->next) {
            if (ml->decl->type == DECL_FUNC &&
                strcmp(ml->decl->data.func.name, field) == 0) {
                sem_error(a, expr->loc, "cannot assign to method '%s' of struct '%s'",
                          field, base);
                return 0;
            }
        }

        sem_error(a, expr->loc, "no assignable field '%s' in struct '%s'", field, base);
        return 0;
    }

    return 1;
}

static char *qualified_member_name_if_package(SemanticAnalyzer *a, Expr *object,
                                              const char *field) {
    if (!object || object->type != EXPR_IDENTIFIER || !field)
        return NULL;
    const char *package_name = object->data.identifier.name;
    if (scope_lookup(a->scope, package_name) || current_struct_field(a, package_name))
        return NULL;

    size_t pkg_len = strlen(package_name);
    size_t field_len = strlen(field);
    char *result = malloc(pkg_len + field_len + 2);
    memcpy(result, package_name, pkg_len);
    result[pkg_len] = '.';
    memcpy(result + pkg_len + 1, field, field_len);
    result[pkg_len + field_len + 1] = '\0';
    return result;
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

static void analyze_initializer_for_type(SemanticAnalyzer *a, Expr *init,
                                         const char *expected_type,
                                         SourceLocation loc, const char *name);

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
        if (idx < array_size)
            analyze_initializer_for_type(a, el->expr, elem_type,
                                         el->expr ? el->expr->loc : loc, name);
        else
            { char *unused = analyze_expr(a, el->expr); free(unused); }
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
        char *ft = struct_field_type_name(&fl->field);
        analyze_initializer_for_type(a, el->expr, ft,
                                     el->expr ? el->expr->loc : loc,
                                     fl->field.name ? fl->field.name : name);
        free(ft);
        el = el->next;
        fl = fl->next;
        idx++;
    }
}

static void analyze_initializer_for_type(SemanticAnalyzer *a, Expr *init,
                                         const char *expected_type,
                                         SourceLocation loc, const char *name) {
    if (!init || !expected_type) return;

    char *resolved_expected = resolve_alias_type(a, expected_type);
    const char *effective_expected = resolved_expected ? resolved_expected : expected_type;

    if (type_is_array(effective_expected)) {
        char *elem = type_array_elem(effective_expected);
        int size = type_array_size(effective_expected);
        analyze_array_initializer(a, init, elem, size, loc, name);
        free(elem);
        free(resolved_expected);
        return;
    }

    if (registry_find_struct(a->types, effective_expected) &&
        init->type == EXPR_ARRAY_LIT) {
        analyze_struct_initializer(a, init, effective_expected, loc, name);
        free(resolved_expected);
        return;
    }

    char *actual = analyze_expr(a, init);
    if (actual && !types_compatible_in_context(a, effective_expected, actual)) {
        sem_error(a, loc, "initializer for '%s': expected '%s', got '%s'",
                  name ? name : "value", effective_expected, actual);
    }
    free(actual);
    free(resolved_expected);
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
        if (expr->data.sizeof_expr.type_name &&
            strcmp(expr->data.sizeof_expr.type_name, "void") == 0) {
            sem_error(a, expr->loc, "sizeof void is not supported");
            return NULL;
        }
        {
            Symbol *sym = scope_lookup(a->scope, expr->data.sizeof_expr.type_name);
            if (sym && sym->is_func) {
                sem_error(a, expr->loc, "sizeof function '%s' is not supported",
                          expr->data.sizeof_expr.type_name);
                return NULL;
            }
        }
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
            Expr *rhs = expr->data.binary.right;
            if (!validate_assignable_target(a, lhs, "assignment target"))
                return NULL;
            char *rt = (rhs && rhs->type == EXPR_ARRAY_LIT) ? NULL : analyze_expr(a, rhs);
            if (!rt && (!rhs || rhs->type != EXPR_ARRAY_LIT)) return NULL;

            if (lhs->type == EXPR_IDENTIFIER) {
                const char *lname = lhs->data.identifier.name;
                Symbol *lsym = scope_lookup(a->scope, lname);
                if (!lsym) {
                    sem_error(a, expr->loc, "undeclared variable '%s' in assignment", lname);
                    free(rt); return NULL;
                }
                lsym->used = 1;
                if (!rt) {
                    analyze_initializer_for_type(a, rhs, lsym->type_name, expr->loc, lname);
                    return lsym->type_name ? strdup(lsym->type_name) : NULL;
                }
                if (lsym->type_name && !types_compatible_in_context(a, lsym->type_name, rt)) {
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
                if (!rt) {
                    analyze_initializer_for_type(a, rhs, lt, expr->loc, "field");
                    return lt;
                }
                if (!types_compatible_in_context(a, lt, rt)) {
                    sem_error(a, expr->loc, "type mismatch in field assignment: '%s' = '%s'", lt, rt);
                    free(lt); free(rt); return NULL;
                }
                { char *result = strdup(lt); free(lt); free(rt); return result; }
            }

            if (lhs->type == EXPR_DEREF) {
                char *lt = analyze_expr(a, lhs);
                if (!lt) { free(rt); return NULL; }
                if (!rt) {
                    analyze_initializer_for_type(a, rhs, lt, expr->loc, "dereference");
                    return lt;
                }
                if (!types_compatible_in_context(a, lt, rt)) {
                    sem_error(a, expr->loc, "type mismatch in deref assignment: '%s' = '%s'", lt, rt);
                    free(lt); free(rt); return NULL;
                }
                { char *result = strdup(lt); free(lt); free(rt); return result; }
            }

            if (lhs->type == EXPR_INDEX) {
                char *lt = analyze_expr(a, lhs);
                if (!lt) { free(rt); return NULL; }
                if (!rt) {
                    analyze_initializer_for_type(a, rhs, lt, expr->loc, "index");
                    return lt;
                }
                if (!types_compatible_in_context(a, lt, rt)) {
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
            if (!validate_assignable_target(a, lhs, "compound assignment target"))
                return NULL;
            char *lt = analyze_expr(a, lhs);
            char *rt = analyze_expr(a, expr->data.binary.right);
            if (!lt || !rt) { free(lt); free(rt); return NULL; }
            if (!types_compatible_in_context(a, lt, rt)) {
                sem_error(a, expr->loc, "type mismatch in compound assignment: '%s' and '%s'", lt, rt);
                free(lt); free(rt); return NULL;
            }
            if (is_bitwise_assignment_op(op) &&
                (!is_integer_type(lt) || !is_integer_type(rt))) {
                sem_error(a, expr->loc,
                          "bitwise compound assignment requires integer types");
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
            if (!types_compatible_in_context(a, lt, rt)) {
                sem_error(a, expr->loc, "type mismatch: '%s' and '%s'", lt, rt);
                free(lt); free(rt); return NULL;
            }
            { char *result = strdup(lt); free(lt); free(rt); return result; }

        case EqualsEquals: case BangEquals:
            if (!types_compatible_in_context(a, lt, rt)) {
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
            if (!is_integer_type(lt) || !is_integer_type(rt)) {
                sem_error(a, expr->loc, "bitwise operator requires integer types");
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
        case Bang:
            if (!is_condition_type(ot)) {
                sem_error(a, expr->loc, "logical NOT applied to non-condition type '%s'", ot);
                free(ot); return NULL;
            }
            if (strcmp(ot, "bool") == 0)
                return ot;
            free(ot);
            return strdup("i32");
        case Tilde:
            if (!is_integer_type(ot)) {
                sem_error(a, expr->loc, "bitwise NOT applied to non-integer type '%s'", ot);
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
            char *qualified_name = qualified_member_name_if_package(
                a, callee->data.member.object, callee->data.member.field);
            Symbol *qsym = qualified_name ? scope_lookup(a->scope, qualified_name) : NULL;
            if (qsym) {
                qsym->used = 1;
                if (!qsym->is_func) {
                    sem_error(a, expr->loc, "'%s' is not a function", qualified_name);
                    free(qualified_name);
                    return NULL;
                }

                int argc = 0;
                for (ExprList *al = expr->data.call.args; al; al = al->next) argc++;

                int param_count = 0;
                for (FuncParamList *pl = qsym->params; pl; pl = pl->next) param_count++;

                if (call_arg_count_invalid(argc, param_count, qsym->is_variadic)) {
                    sem_error(a, expr->loc,
                        qsym->is_variadic
                            ? "function '%s' expects at least %d arguments, got %d"
                            : "function '%s' expects %d arguments, got %d",
                        qualified_name, param_count, argc);
                } else {
                    ExprList *al = expr->data.call.args;
                    FuncParamList *pl = qsym->params;
                    int idx = 0;
                    while (al && pl) {
                        char *expected = resolve_alias_type(a, pl->param.type_name);
                        if (al->expr && al->expr->type == EXPR_ARRAY_LIT && expected) {
                            analyze_initializer_for_type(a, al->expr, expected,
                                                         al->expr->loc, qualified_name);
                        } else {
                            char *at = analyze_expr(a, al->expr);
                            if (at && expected && !types_compatible_in_context(a, expected, at)) {
                                sem_error(a, expr->loc,
                                    "argument %d of '%s': expected '%s', got '%s'",
                                    idx + 1, qualified_name, expected, at);
                            }
                            free(at);
                        }
                        free(expected);
                        al = al->next;
                        pl = pl->next;
                        idx++;
                    }
                }

                char *result = qsym->type_name ? strdup(qsym->type_name) : NULL;
                free(qualified_name);
                return result;
            }
            free(qualified_name);

            if (callee->data.member.object &&
                callee->data.member.object->type == EXPR_IDENTIFIER) {
                const char *type_name = callee->data.member.object->data.identifier.name;
                StructInfo *type_struct = registry_find_struct(a->types, type_name);
                if (type_struct) {
                    Decl *method = NULL;
                    for (DeclList *ml = type_struct->methods; ml; ml = ml->next) {
                        if (ml->decl && ml->decl->type == DECL_FUNC &&
                            strcmp(ml->decl->data.func.name, callee->data.member.field) == 0) {
                            method = ml->decl;
                            break;
                        }
                    }
                    if (!method) {
                        sem_error(a, expr->loc, "no method '%s' in struct '%s'",
                                  callee->data.member.field, type_name);
                        return NULL;
                    }
                    if (!method->data.func.is_static) {
                        sem_error(a, expr->loc,
                                  "cannot call instance method '%s.%s' on type '%s'",
                                  type_name, callee->data.member.field, type_name);
                        return NULL;
                    }

                    char full_name[256];
                    snprintf(full_name, sizeof(full_name), "%s.%s",
                             type_name, callee->data.member.field);
                    Symbol *msym = scope_lookup(a->scope, full_name);
                    if (msym) msym->used = 1;

                    int argc = 0;
                    for (ExprList *al = expr->data.call.args; al; al = al->next) argc++;
                    int param_count = 0;
                    for (FuncParamList *pl = method->data.func.params; pl; pl = pl->next)
                        param_count++;

                    if (call_arg_count_invalid(argc, param_count,
                                               method->data.func.is_variadic)) {
                        sem_error(a, expr->loc,
                            method->data.func.is_variadic
                                ? "method '%s' expects at least %d arguments, got %d"
                                : "method '%s' expects %d arguments, got %d",
                            full_name, param_count, argc);
                    } else {
                        ExprList *al = expr->data.call.args;
                        FuncParamList *pl = method->data.func.params;
                        int idx = 0;
                        while (al && pl) {
                            char *expected = resolve_alias_type(a, pl->param.type_name);
                            char *at = analyze_expr(a, al->expr);
                            if (at && expected && !types_compatible_in_context(a, expected, at)) {
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

                    return method->data.func.return_type
                        ? strdup(method->data.func.return_type) : NULL;
                }
            }

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
                InterfaceInfo *ii = registry_find_interface(a->types, base);
                if (!ii) {
                    sem_error(a, expr->loc, "cannot call method on non-struct type '%s'", base);
                    return NULL;
                }

                InterfaceMethod *method = NULL;
                for (InterfaceMethodList *ml = ii->methods; ml; ml = ml->next) {
                    if (ml->method.name &&
                        strcmp(ml->method.name, callee->data.member.field) == 0) {
                        method = &ml->method;
                        break;
                    }
                }
                if (!method) {
                    sem_error(a, expr->loc, "no method '%s' in interface '%s'",
                              callee->data.member.field, base);
                    return NULL;
                }

                int argc = 0;
                for (ExprList *al = expr->data.call.args; al; al = al->next) argc++;
                int param_count = 0;
                for (FuncParamList *pl = method->params; pl; pl = pl->next) param_count++;

                if (call_arg_count_invalid(argc, param_count, method->is_variadic)) {
                    sem_error(a, expr->loc,
                        method->is_variadic
                            ? "method '%s.%s' expects at least %d arguments, got %d"
                            : "method '%s.%s' expects %d arguments, got %d",
                        base, callee->data.member.field, param_count, argc);
                } else {
                    ExprList *al = expr->data.call.args;
                    FuncParamList *pl = method->params;
                    int idx = 0;
                    while (al && pl) {
                        char *expected = resolve_alias_type(a, pl->param.type_name);
                        char *at = analyze_expr(a, al->expr);
                        if (at && expected && !types_compatible_in_context(a, expected, at)) {
                            sem_error(a, expr->loc,
                                "argument %d of '%s.%s': expected '%s', got '%s'",
                                idx + 1, base, callee->data.member.field, expected, at);
                        }
                        free(expected);
                        free(at);
                        al = al->next;
                        pl = pl->next;
                        idx++;
                    }
                }
                return method->return_type ? strdup(method->return_type) : NULL;
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
            if (method->data.func.is_static) {
                sem_error(a, expr->loc,
                          "cannot call static method '%s.%s' on an instance",
                          base, callee->data.member.field);
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

            if (call_arg_count_invalid(argc, param_count,
                                       method->data.func.is_variadic)) {
                sem_error(a, expr->loc,
                    method->data.func.is_variadic
                        ? "method '%s' expects at least %d arguments, got %d"
                        : "method '%s' expects %d arguments, got %d",
                    full_name, param_count, argc);
            } else {
                ExprList *al = expr->data.call.args;
                FuncParamList *pl = has_explicit_receiver ? params->next : params;
                int idx = 0;
                while (al && pl) {
                    char *expected = resolve_alias_type(a, pl->param.type_name);
                    if (al->expr && al->expr->type == EXPR_ARRAY_LIT && expected) {
                        analyze_initializer_for_type(a, al->expr, expected,
                                                     al->expr->loc, full_name);
                    } else {
                        char *at = analyze_expr(a, al->expr);
                        if (at && expected && !types_compatible_in_context(a, expected, at)) {
                            sem_error(a, expr->loc,
                                "argument %d of '%s': expected '%s', got '%s'",
                                idx + 1, full_name, expected, at);
                        }
                        free(at);
                    }
                    free(expected);
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

        if (call_arg_count_invalid(argc, param_count, fsym->is_variadic)) {
            sem_error(a, expr->loc,
                fsym->is_variadic
                    ? "function '%s' expects at least %d arguments, got %d"
                    : "function '%s' expects %d arguments, got %d",
                implicit_method_call ? method_name : fname, param_count, argc);
        } else {
            ExprList *al = expr->data.call.args;
            FuncParamList *pl = params;
            int idx = 0;
            while (al && pl) {
                char *expected = resolve_alias_type(a, pl->param.type_name);
                if (al->expr && al->expr->type == EXPR_ARRAY_LIT && expected) {
                    analyze_initializer_for_type(a, al->expr, expected,
                                                 al->expr->loc,
                                                 implicit_method_call ? method_name : fname);
                } else {
                    char *at = analyze_expr(a, al->expr);
                    if (at && expected && !types_compatible_in_context(a, expected, at)) {
                        sem_error(a, expr->loc,
                            "argument %d of '%s': expected '%s', got '%s'",
                            idx + 1, implicit_method_call ? method_name : fname, expected, at);
                    }
                    free(at);
                }
                free(expected);
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
        char *qualified_name = qualified_member_name_if_package(
            a, expr->data.member.object, expr->data.member.field);
        Symbol *qsym = qualified_name ? scope_lookup(a->scope, qualified_name) : NULL;
        if (qsym) {
            qsym->used = 1;
            char *result = qsym->type_name ? strdup(qsym->type_name) : NULL;
            free(qualified_name);
            return result;
        }
        free(qualified_name);

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
            if (strcmp(fl->field.name, field) == 0) {
                char *field_type = struct_field_type_name(&fl->field);
                char *resolved = resolve_alias_type(a, field_type);
                free(field_type);
                return resolved;
            }
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
        if (!validate_assignable_target(a, expr->data.addrof.operand,
                                        "address-of operand"))
            return NULL;
        char *ot = analyze_expr(a, expr->data.addrof.operand);
        if (!ot) return NULL;
        char *result = type_make_ptr(ot);
        free(ot);
        return result;
    }

    case EXPR_CAST: {
        char *target = resolve_alias_type(a, expr->data.cast.type_name);
        const char *effective_target = target ? target : expr->data.cast.type_name;
        if (!type_name_known(a, effective_target)) {
            sem_error(a, expr->loc, "cast to unknown type '%s'",
                      expr->data.cast.type_name ? expr->data.cast.type_name : "");
            free(target);
            return NULL;
        }
        if (effective_target && strcmp(effective_target, "void") == 0) {
            sem_error(a, expr->loc, "cast to void is not supported");
            free(target);
            return NULL;
        }
        char *ot = analyze_expr(a, expr->data.cast.operand);
        if (!ot) {
            free(target);
            return NULL;
        }
        char *resolved_operand = resolve_alias_type(a, ot);
        const char *effective_operand = resolved_operand ? resolved_operand : ot;
        if (type_is_aggregate_value(a, effective_target) ||
            type_is_aggregate_value(a, effective_operand)) {
            sem_error(a, expr->loc,
                      "cannot cast aggregate type '%s' to '%s'",
                      effective_operand ? effective_operand : "",
                      effective_target ? effective_target : "");
            free(resolved_operand);
            free(target);
            free(ot);
            return NULL;
        }
        int ok = types_equal(effective_target, effective_operand) ||
                 (type_is_numeric_like(effective_target) &&
                  type_is_numeric_like(effective_operand)) ||
                 (type_is_pointer_like(effective_target) &&
                  type_is_pointer_like(effective_operand)) ||
                 (type_is_pointer_like(effective_target) &&
                  strcmp(effective_operand, "void") == 0) ||
                 (type_is_pointer_like(effective_target) &&
                  is_integer_type(effective_operand)) ||
                 (is_integer_type(effective_target) &&
                  type_is_pointer_like(effective_operand));
        if (!ok) {
            sem_error(a, expr->loc, "cannot cast from '%s' to '%s'",
                      effective_operand ? effective_operand : "",
                      effective_target ? effective_target : "");
            free(resolved_operand);
            free(target);
            free(ot);
            return NULL;
        }
        char *result = effective_target ? strdup(effective_target) : NULL;
        free(resolved_operand);
        free(target);
        free(ot);
        return result;
    }

    case EXPR_ASSIGN: {
        Symbol *sym = scope_lookup(a->scope, expr->data.assign.name);
        if (!sym) {
            StructField *field = current_struct_field(a, expr->data.assign.name);
            if (field) {
                char *field_type = struct_field_type_name(field);
                if (expr->data.assign.value &&
                    expr->data.assign.value->type == EXPR_ARRAY_LIT) {
                    analyze_initializer_for_type(a, expr->data.assign.value,
                                                 field_type, expr->loc,
                                                 expr->data.assign.name);
                    char *result = field_type ? strdup(field_type) : NULL;
                    free(field_type);
                    return result;
                }
                char *vt = analyze_expr(a, expr->data.assign.value);
                if (!vt) { free(field_type); return NULL; }
                if (!types_compatible_in_context(a, field_type, vt)) {
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
        if (sym->is_func) {
            sem_error(a, expr->loc, "cannot assign to function '%s'",
                      expr->data.assign.name);
            return NULL;
        }
        sym->used = 1;
        if (expr->data.assign.value &&
            expr->data.assign.value->type == EXPR_ARRAY_LIT) {
            analyze_initializer_for_type(a, expr->data.assign.value,
                                         sym->type_name, expr->loc,
                                         expr->data.assign.name);
            return sym->type_name ? strdup(sym->type_name) : NULL;
        }
        char *vt = analyze_expr(a, expr->data.assign.value);
        if (!vt) return NULL;
        if (sym->type_name && !types_compatible_in_context(a, sym->type_name, vt)) {
            sem_error(a, expr->loc, "type mismatch in assignment to '%s': '%s' = '%s'",
                      expr->data.assign.name, sym->type_name, vt);
            free(vt); return NULL;
        }
        { char *result = sym->type_name ? strdup(sym->type_name) : NULL; free(vt); return result; }
    }

    case EXPR_INDEX: {
        char *at = analyze_expr(a, expr->data.index.array);
        char *it = analyze_expr(a, expr->data.index.index);
        if (it && !is_integer_type(it)) {
            sem_error(a, expr->data.index.index ? expr->data.index.index->loc : expr->loc,
                      "index must be integer, got '%s'", it);
            free(at);
            free(it);
            return NULL;
        }
        free(it);
        if (!at) return NULL;
        if (type_is_array(at)) {
            char *result = type_array_elem(at); free(at); return result;
        }
        if (type_is_ptr(at)) {
            char *result = type_deref(at); free(at); return result;
        }
        if (strcmp(at, "str") == 0) {
            free(at);
            return strdup("char");
        }
        sem_error(a, expr->loc, "indexing non-array/non-pointer type '%s'", at);
        free(at);
        return NULL;
    }
    case EXPR_NEW: {
        if (!type_name_known(a, expr->data.new_expr.type_name)) {
            sem_error(a, expr->loc, "new of unknown type '%s'",
                      expr->data.new_expr.type_name ? expr->data.new_expr.type_name : "");
            return NULL;
        }
        for (ExprList *dl = expr->data.new_expr.dims; dl; dl = dl->next) {
            char *dt = analyze_expr(a, dl->expr);
            if (dt && !is_integer_type(dt)) {
                sem_error(a, dl->expr ? dl->expr->loc : expr->loc,
                          "new array dimension must be integer, got '%s'", dt);
                free(dt);
                return NULL;
            }
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
        if (et && !type_is_ptr(et)) {
            sem_error(a, expr->loc, "delete requires pointer type, got '%s'", et);
        }
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
        if (!types_compatible_in_context(a, tt, et) && !types_compatible_in_context(a, et, tt)) {
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
            if (a->current_func_return &&
                strcmp(a->current_func_return, "void") == 0) {
                sem_error(a, stmt->loc,
                    "void function '%s' must not return a value",
                    a->current_func_name ? a->current_func_name : "?");
            } else if (stmt->data.return_stmt.value->type == EXPR_ARRAY_LIT &&
                a->current_func_return &&
                registry_find_struct(a->types, a->current_func_return)) {
                analyze_initializer_for_type(a, stmt->data.return_stmt.value,
                                             a->current_func_return,
                                             stmt->loc, "return value");
            } else {
                char *vt = analyze_expr(a, stmt->data.return_stmt.value);
                if (vt && a->current_func_return) {
                    if (!types_compatible_in_context(a, a->current_func_return, vt)) {
                        sem_error(a, stmt->loc,
                            "return type mismatch: function returns '%s', got '%s'",
                            a->current_func_return, vt);
                    }
                }
                free(vt);
            }
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

        if (!validate_declared_type(a, stmt->loc, effective_ty, "local variable", 0)) {
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
            if (it && effective_ty && !types_compatible_in_context(a, effective_ty, it)) {
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
        long long *case_values = NULL;
        int case_value_count = 0;
        int case_value_cap = 0;
        for (SwitchCase *sc = stmt->data.switch_stmt.cases; sc; sc = sc->next) {
            char *ct = analyze_expr(a, sc->value);
            if (st && ct && !types_compatible_in_context(a, st, ct)) {
                sem_error(a, sc->value->loc,
                          "case value type '%s' is not compatible with switch type '%s'",
                          ct, st);
            }
            long long case_value = 0;
            if (eval_const_int_expr_sem(a, sc->value, &case_value)) {
                for (int i = 0; i < case_value_count; i++) {
                    if (case_values[i] == case_value) {
                        sem_error(a, sc->value->loc,
                                  "duplicate switch case value '%lld'",
                                  case_value);
                        break;
                    }
                }
                if (case_value_count == case_value_cap) {
                    case_value_cap = case_value_cap ? case_value_cap * 2 : 8;
                    case_values = realloc(case_values,
                                          sizeof(long long) * case_value_cap);
                }
                case_values[case_value_count++] = case_value;
            }
            free(ct);
            analyze_stmt_list(a, sc->stmts);
        }
        free(case_values);
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
    int unreachable = 0;
    for (StmtList *sl = list; sl; sl = sl->next) {
        if (unreachable) {
            sem_error(a, sl->stmt ? sl->stmt->loc : (SourceLocation){0},
                      "unreachable statement");
        }
        analyze_stmt(a, sl->stmt);
        if (sl->stmt && stmt_guarantees_exit(sl->stmt))
            unreachable = 1;
    }
}

/* ── Declaration analysis ──────────────────────────────────── */

static int stmt_list_guarantees_return(StmtList *list);
static int stmt_list_guarantees_exit(StmtList *list);

static int stmt_guarantees_exit(Stmt *stmt) {
    if (!stmt) return 0;

    switch (stmt->type) {
    case STMT_RETURN:
    case STMT_BREAK:
    case STMT_CONTINUE:
        return 1;

    case STMT_BLOCK:
        return stmt_list_guarantees_exit(stmt->data.block.stmts);

    case STMT_IF:
        if (!stmt->data.if_stmt.else_block ||
            !stmt_guarantees_exit(stmt->data.if_stmt.then_block))
            return 0;
        for (ElifClause *ec = stmt->data.if_stmt.elifs; ec; ec = ec->next) {
            if (!stmt_guarantees_exit(ec->body))
                return 0;
        }
        return stmt_guarantees_exit(stmt->data.if_stmt.else_block);

    case STMT_SWITCH:
        return stmt_guarantees_return(stmt);

    default:
        return 0;
    }
}

static int stmt_list_guarantees_exit(StmtList *list) {
    for (StmtList *sl = list; sl; sl = sl->next) {
        if (stmt_guarantees_exit(sl->stmt))
            return 1;
    }
    return 0;
}

static int stmt_guarantees_return(Stmt *stmt) {
    if (!stmt) return 0;

    switch (stmt->type) {
    case STMT_RETURN:
        return 1;

    case STMT_BLOCK:
        return stmt_list_guarantees_return(stmt->data.block.stmts);

    case STMT_IF:
        if (!stmt->data.if_stmt.else_block ||
            !stmt_guarantees_return(stmt->data.if_stmt.then_block))
            return 0;
        for (ElifClause *ec = stmt->data.if_stmt.elifs; ec; ec = ec->next) {
            if (!stmt_guarantees_return(ec->body))
                return 0;
        }
        return stmt_guarantees_return(stmt->data.if_stmt.else_block);

    case STMT_SWITCH:
        if (!stmt->data.switch_stmt.default_stmts)
            return 0;
        for (SwitchCase *sc = stmt->data.switch_stmt.cases; sc; sc = sc->next) {
            if (!stmt_list_guarantees_return(sc->stmts))
                return 0;
        }
        return stmt_list_guarantees_return(stmt->data.switch_stmt.default_stmts);

    default:
        return 0;
    }
}

static int stmt_list_guarantees_return(StmtList *list) {
    for (StmtList *sl = list; sl; sl = sl->next) {
        if (stmt_guarantees_return(sl->stmt))
            return 1;
    }
    return 0;
}

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
    validate_declared_type(a, decl->loc, resolved_return ? resolved_return : f->return_type,
                           "function return type", 1);
    if (a->current_struct_name)
        validate_param_types(a, decl->loc, f->params, "function parameter");
    if (f->is_variadic && !f->params) {
        sem_error(a, decl->loc,
                  "variadic function '%s' requires at least one fixed parameter",
                  f->name);
    }
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
        if (a->current_struct_name && !f->is_static && !has_explicit_receiver) {
            char *self_ty = type_make_ptr(a->current_struct_name);
            scope_insert(a->scope, "self", self_ty, 1, 0, 0, NULL, decl->loc);
            free(self_ty);
            Symbol *self_sym = scope_lookup_current(a->scope, "self");
            if (self_sym) self_sym->used = 1;
        }

        for (FuncParamList *pl = f->params; pl; pl = pl->next) {
            if (!pl->param.name || pl->param.name[0] == '\0')
                continue;
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
        if (a->current_func_return &&
            strcmp(a->current_func_return, "void") != 0 &&
            !stmt_list_guarantees_return(f->body->data.block.stmts)) {
            sem_error(a, decl->loc,
                      "function '%s' may exit without returning a value of type '%s'",
                      f->name, a->current_func_return);
        }

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

static int eval_const_int_expr_sem(SemanticAnalyzer *a, Expr *expr, long long *out) {
    if (!expr || !out) return 0;
    switch (expr->type) {
    case EXPR_INT_LIT:
        *out = parse_int_literal_value_sem(expr->data.int_lit.value);
        return 1;
    case EXPR_CHAR_LIT:
        *out = parse_char_literal_value_sem(expr->data.char_lit.value);
        return 1;
    case EXPR_BOOL_LIT:
        *out = expr->data.bool_lit.value ? 1 : 0;
        return 1;
    case EXPR_NULL:
        *out = 0;
        return 1;
    case EXPR_IDENTIFIER: {
        Symbol *sym = scope_lookup(a->scope, expr->data.identifier.name);
        if (sym && sym->has_const_int) {
            sym->used = 1;
            *out = sym->const_int_value;
            return 1;
        }
        return 0;
    }
    case EXPR_UNARY: {
        long long val;
        if (!eval_const_int_expr_sem(a, expr->data.unary.operand, &val))
            return 0;
        if (expr->data.unary.op == Minus) {
            *out = -val;
            return 1;
        }
        if (expr->data.unary.op == Tilde) {
            *out = ~val;
            return 1;
        }
        return 0;
    }
    case EXPR_BINARY: {
        long long left, right;
        if (!eval_const_int_expr_sem(a, expr->data.binary.left, &left) ||
            !eval_const_int_expr_sem(a, expr->data.binary.right, &right))
            return 0;
        switch (expr->data.binary.op) {
        case Plus:       *out = left + right; return 1;
        case Minus:      *out = left - right; return 1;
        case Star:       *out = left * right; return 1;
        case Slash:      if (right == 0) return 0; *out = left / right; return 1;
        case Percent:    if (right == 0) return 0; *out = left % right; return 1;
        case Ampersand:  *out = left & right; return 1;
        case Pipe:       *out = left | right; return 1;
        case Caret:      *out = left ^ right; return 1;
        case LeftShift:  *out = left << right; return 1;
        case RightShift: *out = left >> right; return 1;
        default:         return 0;
        }
    }
    default:
        return 0;
    }
}

static int eval_const_float_expr_sem(SemanticAnalyzer *a, Expr *expr, double *out) {
    if (!expr || !out) return 0;
    switch (expr->type) {
    case EXPR_FLOAT_LIT:
        *out = parse_float_literal_value_sem(expr->data.float_lit.value);
        return 1;
    case EXPR_INT_LIT:
        *out = (double)parse_int_literal_value_sem(expr->data.int_lit.value);
        return 1;
    case EXPR_CHAR_LIT:
        *out = (double)parse_char_literal_value_sem(expr->data.char_lit.value);
        return 1;
    case EXPR_BOOL_LIT:
        *out = expr->data.bool_lit.value ? 1.0 : 0.0;
        return 1;
    case EXPR_IDENTIFIER: {
        long long val;
        if (eval_const_int_expr_sem(a, expr, &val)) {
            *out = (double)val;
            return 1;
        }
        return 0;
    }
    case EXPR_UNARY: {
        double val;
        if (expr->data.unary.op != Minus ||
            !eval_const_float_expr_sem(a, expr->data.unary.operand, &val))
            return 0;
        *out = -val;
        return 1;
    }
    case EXPR_BINARY: {
        double left, right;
        if (!eval_const_float_expr_sem(a, expr->data.binary.left, &left) ||
            !eval_const_float_expr_sem(a, expr->data.binary.right, &right))
            return 0;
        switch (expr->data.binary.op) {
        case Plus:  *out = left + right; return 1;
        case Minus: *out = left - right; return 1;
        case Star:  *out = left * right; return 1;
        case Slash: if (right == 0.0) return 0; *out = left / right; return 1;
        default:    return 0;
        }
    }
    default:
        return 0;
    }
}

static int validate_global_const_expr(SemanticAnalyzer *a, Expr *expr) {
    if (!expr) return 1;
    switch (expr->type) {
    case EXPR_INT_LIT:
    case EXPR_FLOAT_LIT:
    case EXPR_CHAR_LIT:
    case EXPR_STRING_LIT:
    case EXPR_BOOL_LIT:
    case EXPR_NULL:
    case EXPR_SIZEOF:
        return 1;
    case EXPR_IDENTIFIER: {
        Symbol *sym = scope_lookup(a->scope, expr->data.identifier.name);
        if (sym && sym->is_const) {
            sym->used = 1;
            return 1;
        }
        return 0;
    }
    case EXPR_UNARY:
        return (expr->data.unary.op == Minus || expr->data.unary.op == Tilde) &&
               validate_global_const_expr(a, expr->data.unary.operand);
    case EXPR_BINARY: {
        switch (expr->data.binary.op) {
        case Plus: case Minus: case Star: case Slash: case Percent:
        case Ampersand: case Pipe: case Caret:
        case LeftShift: case RightShift:
            if (!validate_global_const_expr(a, expr->data.binary.left) ||
                !validate_global_const_expr(a, expr->data.binary.right))
                return 0;
            if (expr->data.binary.op == Slash || expr->data.binary.op == Percent) {
                long long int_divisor;
                double float_divisor;
                if (eval_const_int_expr_sem(a, expr->data.binary.right, &int_divisor) &&
                    int_divisor == 0) {
                    sem_error(a, expr->loc, "constant division by zero");
                    return 0;
                }
                if (expr->data.binary.op == Slash &&
                    eval_const_float_expr_sem(a, expr->data.binary.right, &float_divisor) &&
                    float_divisor == 0.0) {
                    sem_error(a, expr->loc, "constant division by zero");
                    return 0;
                }
            }
            return 1;
        default:
            return 0;
        }
    }
    case EXPR_ARRAY_LIT:
        for (ExprList *el = expr->data.array_lit.elements; el; el = el->next) {
            if (!validate_global_const_expr(a, el->expr))
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
    if (!validate_declared_type(a, decl->loc, effective, "global variable", 0)) {
        free(resolved_type);
        return;
    }
    char *decl_ty = g->array_size > 0 ? type_make_array(effective, g->array_size) : NULL;
    scope_insert(a->scope, g->name, decl_ty ? decl_ty : effective,
                 g->is_ptr, 0, 0, NULL, decl->loc);
    free(decl_ty);
    free(resolved_type);
}

static void analyze_global_var_decl(SemanticAnalyzer *a, Decl *decl) {
    GlobalVarDecl *g = &decl->data.global_var;
    int had_error_before_const_check = a->had_error;
    if (!validate_global_const_expr(a, g->init)) {
        if (a->had_error == had_error_before_const_check) {
            sem_error(a, decl->loc,
                      "global variable '%s' requires a constant initializer",
                      g->name);
        }
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
        char *expected = resolve_alias_type(a, g->type_name);
        const char *effective = expected ? expected : g->type_name;
        if (registry_find_struct(a->types, effective) &&
            g->init->type == EXPR_ARRAY_LIT) {
            analyze_struct_initializer(a, g->init, effective, decl->loc, g->name);
            free(expected);
            return;
        }
        free(expected);
    }

    if (g->init) {
        char *it = analyze_expr(a, g->init);
        char *expected = resolve_alias_type(a, g->type_name);
        const char *effective = expected ? expected : g->type_name;
        if (it && effective && !types_compatible_in_context(a, effective, it)) {
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

    reject_generic_params(a, decl->loc, f->generic_params, "function", f->name);

    if (scope_lookup_current(a->scope, f->name)) {
        sem_error(a, decl->loc, "redeclaration of function '%s'", f->name);
        return;
    }

    char *resolved_return = resolve_alias_type(a, f->return_type);
    validate_declared_type(a, decl->loc, resolved_return ? resolved_return : f->return_type,
                           "function return type", 1);
    validate_param_types(a, decl->loc, f->params, "function parameter");
    scope_insert(a->scope, f->name, resolved_return ? resolved_return : f->return_type,
                 0, 1, f->is_variadic, f->params, decl->loc);
    free(resolved_return);
}

static void declare_struct_type_decl(SemanticAnalyzer *a, Decl *decl) {
    StructDecl *s = &decl->data.struct_decl;
    reject_generic_params(a, decl->loc, s->generic_params, "struct", s->name);

    if (scope_lookup_current(a->scope, s->name)) {
        sem_error(a, decl->loc, "redeclaration of struct '%s'", s->name);
        return;
    }

    scope_insert(a->scope, s->name, s->name, 0, 0, 0, NULL, decl->loc);
    registry_add_struct(a->types, s->name, s->fields,
                        s->generic_params, s->methods);
}

static void declare_interface_type_decl(SemanticAnalyzer *a, Decl *decl) {
    InterfaceDecl *iface = &decl->data.interface;
    reject_generic_params(a, decl->loc, iface->generic_params, "interface", iface->name);

    if (scope_lookup_current(a->scope, iface->name)) {
        sem_error(a, decl->loc, "redeclaration of interface '%s'", iface->name);
        return;
    }

    scope_insert(a->scope, iface->name, iface->name, 0, 0, 0, NULL, decl->loc);
    registry_add_interface(a->types, iface->name, iface->methods,
                           iface->generic_params);
}

static void analyze_struct_decl(SemanticAnalyzer *a, Decl *decl) {
    StructDecl *s = &decl->data.struct_decl;
    StructInfo *registered = registry_find_struct(a->types, s->name);
    if (!registered || registered->fields != s->fields)
        return;

    for (StructFieldList *fl = s->fields; fl; fl = fl->next) {
        char *resolved_field = resolve_alias_type(a, fl->field.type_name);
        validate_declared_type(a, decl->loc,
                               resolved_field ? resolved_field : fl->field.type_name,
                               "struct field", 0);
        free(resolved_field);

        for (StructFieldList *fl2 = fl->next; fl2; fl2 = fl2->next) {
            if (strcmp(fl->field.name, fl2->field.name) == 0) {
                sem_error(a, decl->loc, "duplicate field '%s' in struct '%s'",
                          fl->field.name, s->name);
            }
        }

        for (DeclList *ml = s->methods; ml; ml = ml->next) {
            if (ml->decl && ml->decl->type == DECL_FUNC &&
                strcmp(fl->field.name, ml->decl->data.func.name) == 0) {
                sem_error(a, ml->decl->loc,
                          "method '%s' conflicts with field of struct '%s'",
                          fl->field.name, s->name);
            }
        }
    }

    char *prev_struct_name = a->current_struct_name;
    a->current_struct_name = strdup(s->name);

    for (DeclList *ml = s->methods; ml; ml = ml->next) {
        if (ml->decl->type == DECL_FUNC) {
            FuncDecl *m = &ml->decl->data.func;
            int duplicate_method = 0;
            for (DeclList *prev = s->methods; prev && prev != ml; prev = prev->next) {
                if (prev->decl && prev->decl->type == DECL_FUNC &&
                    strcmp(prev->decl->data.func.name, m->name) == 0) {
                    duplicate_method = 1;
                    break;
                }
            }
            if (duplicate_method) {
                sem_error(a, ml->decl->loc, "duplicate method '%s' in struct '%s'",
                          m->name, s->name);
                continue;
            }
            char full_name[256];
            snprintf(full_name, sizeof(full_name), "%s.%s", s->name, m->name);
            if (!scope_insert(a->scope, full_name, m->return_type, 0, 1, 0,
                              m->params, ml->decl->loc)) {
                sem_error(a, ml->decl->loc, "redeclaration of method '%s'",
                          full_name);
            }
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
    for (InterfaceMethodList *ml = iface->methods; ml; ml = ml->next) {
        InterfaceMethod *m = &ml->method;
        for (InterfaceMethodList *next = ml->next; next; next = next->next) {
            if (next->method.name && m->name &&
                strcmp(m->name, next->method.name) == 0) {
                sem_error(a, decl->loc, "duplicate method '%s' in interface '%s'",
                          m->name, iface->name);
                break;
            }
        }

        char *resolved_return = resolve_alias_type(a, m->return_type);
        validate_declared_type(a, decl->loc,
                               resolved_return ? resolved_return : m->return_type,
                               "interface method return type", 1);
        free(resolved_return);
        validate_param_types(a, decl->loc, m->params, "interface method parameter");
        if (m->is_variadic && !m->params) {
            sem_error(a, decl->loc,
                      "variadic interface method '%s' requires at least one fixed parameter",
                      m->name);
        }
    }
}

static void analyze_type_alias_decl(SemanticAnalyzer *a, Decl *decl) {
    TypeAliasDecl *alias = &decl->data.type_alias;
    if (scope_lookup_current(a->scope, alias->name)) {
        sem_error(a, decl->loc, "redeclaration of type alias '%s'", alias->name);
        return;
    }
    char *target = resolve_alias_type(a, alias->target_type);
    validate_declared_type(a, decl->loc, target ? target : alias->target_type,
                           "type alias", 1);
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
        Symbol *variant = scope_lookup_current(a->scope, v->name);
        if (variant) {
            variant->is_const = 1;
            variant->has_const_int = 1;
            variant->const_int_value = v->value;
        }
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
        if (dl->decl->type == DECL_STRUCT)
            declare_struct_type_decl(a, dl->decl);
        else if (dl->decl->type == DECL_INTERFACE)
            declare_interface_type_decl(a, dl->decl);
    }

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
