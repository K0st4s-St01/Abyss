#include "import_resolver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ── Magic ──────────────────────────────────────────────────── */

static const char MAGIC[] = "DWELLERPKG\0";
#define MAGIC_LEN 11
#define HEADER_META_LEN_OFFSET MAGIC_LEN
#define HEADER_SIZE (MAGIC_LEN + 4)

/* ── File I/O ──────────────────────────────────────────────── */

static char *ir_read_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    char *buf = malloc(sz);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    fclose(f);
    *out_size = sz;
    return buf;
}

/* ── Minimal JSON parser ────────────────────────────────────── */

typedef struct { const char *p; const char *end; } IRJP;

static IRJP ir_jp_array_item(IRJP arr, int index);

static void ir_jp_skip(IRJP *jp) {
    while (jp->p < jp->end && (*jp->p == ' ' || *jp->p == '\t' ||
           *jp->p == '\n' || *jp->p == '\r'))
        jp->p++;
}

static char *ir_jp_parse_str(IRJP *jp) {
    ir_jp_skip(jp);
    if (jp->p >= jp->end || *jp->p != '"') return NULL;
    jp->p++;
    size_t cap = 64, len = 0;
    char *s = malloc(cap);
    while (jp->p < jp->end && *jp->p != '"') {
        char c = *jp->p++;
        if (c == '\\' && jp->p < jp->end) c = *jp->p++;
        if (len + 1 >= cap) { cap *= 2; s = realloc(s, cap); }
        s[len++] = c;
    }
    if (jp->p < jp->end) jp->p++;
    s[len] = '\0';
    return s;
}

static int ir_jp_parse_int(IRJP *jp) {
    ir_jp_skip(jp);
    int neg = 0;
    if (jp->p < jp->end && *jp->p == '-') { neg = 1; jp->p++; }
    int val = 0;
    while (jp->p < jp->end && *jp->p >= '0' && *jp->p <= '9')
        val = val * 10 + (*jp->p++ - '0');
    return neg ? -val : val;
}

static void ir_jp_skip_value(IRJP *jp) {
    ir_jp_skip(jp);
    if (jp->p >= jp->end) return;
    if (*jp->p == '"') { ir_jp_parse_str(jp); return; }
    if (*jp->p == '{' || *jp->p == '[') {
        char open = *jp->p++;
        char close = (open == '{') ? '}' : ']';
        int depth = 1, in_str = 0;
        while (jp->p < jp->end && depth > 0) {
            if (*jp->p == '"' && !in_str) in_str = 1;
            else if (*jp->p == '"' && in_str) in_str = 0;
            else if (!in_str) {
                if (*jp->p == open) depth++;
                else if (*jp->p == close) depth--;
            }
            jp->p++;
        }
        return;
    }
    while (jp->p < jp->end && *jp->p != ',' && *jp->p != '}' && *jp->p != ']')
        jp->p++;
}

static IRJP ir_jp_find(IRJP *jp, const char *key) {
    IRJP sub = *jp;
    ir_jp_skip(&sub);
    if (sub.p >= sub.end || *sub.p != '{') return *jp;
    sub.p++;

    while (sub.p < sub.end) {
        ir_jp_skip(&sub);
        if (sub.p >= sub.end || *sub.p == '}') break;
        char *k = ir_jp_parse_str(&sub);
        ir_jp_skip(&sub);
        if (sub.p < sub.end && *sub.p == ':') sub.p++;
        if (k && strcmp(k, key) == 0) {
            free(k);
            return sub;
        }
        free(k);
        ir_jp_skip_value(&sub);
        ir_jp_skip(&sub);
        if (sub.p < sub.end && *sub.p == ',') sub.p++;
    }
    return *jp;
}

static int ir_jp_array_count(IRJP arr) {
    ir_jp_skip(&arr);
    if (arr.p >= arr.end || *arr.p != '[') return 0;
    arr.p++;
    int count = 0;
    while (arr.p < arr.end) {
        ir_jp_skip(&arr);
        if (arr.p >= arr.end || *arr.p == ']') break;
        count++;
        ir_jp_skip_value(&arr);
        ir_jp_skip(&arr);
        if (arr.p < arr.end && *arr.p == ',') arr.p++;
    }
    return count;
}

static char *ir_json_get_string(IRJP *jp, const char *key) {
    IRJP val = ir_jp_find(jp, key);
    ir_jp_skip(&val);
    return ir_jp_parse_str(&val);
}

static int ir_json_get_int(IRJP *jp, const char *key) {
    IRJP val = ir_jp_find(jp, key);
    return ir_jp_parse_int(&val);
}

static IRParam *ir_parse_params(IRJP item, int *out_count) {
    *out_count = 0;
    IRJP params_arr = ir_jp_find(&item, "params");
    ir_jp_skip(&params_arr);
    if (params_arr.p >= params_arr.end || *params_arr.p != '[')
        return NULL;

    *out_count = ir_jp_array_count(params_arr);
    if (*out_count <= 0)
        return NULL;

    IRParam *params = calloc(*out_count, sizeof(IRParam));
    for (int j = 0; j < *out_count; j++) {
        IRJP pic = ir_jp_array_item(params_arr, j);
        params[j].name = ir_json_get_string(&pic, "name");
        params[j].type_name = ir_json_get_string(&pic, "type");
        params[j].is_ptr = ir_json_get_int(&pic, "is_ptr");
    }
    return params;
}

static IRJP ir_jp_array_item(IRJP arr, int index) {
    ir_jp_skip(&arr);
    if (arr.p >= arr.end || *arr.p != '[') return arr;
    arr.p++;
    IRJP item = arr;
    for (int i = 0; i <= index; i++) {
        ir_jp_skip(&item);
        if (item.p >= item.end || *item.p == ']') break;
        if (i < index) {
            ir_jp_skip_value(&item);
            ir_jp_skip(&item);
            if (item.p < item.end && *item.p == ',') item.p++;
        }
    }
    return item;
}

/* ── Package directory ──────────────────────────────────────── */

static const char *ir_pkg_dir(void) {
    static char buf[512];
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(buf, sizeof(buf), "%s/.dwellerpkg/packages", home);
    return buf;
}

/* ── Find library ───────────────────────────────────────────── */

bool ir_find_library(const char *package_name, char **out_path) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "find \"%s/%s\" -name '*.abyss_a' -o -name '*.abyss_so' 2>/dev/null | sort -V | tail -1",
        ir_pkg_dir(), package_name);

    FILE *p = popen(cmd, "r");
    if (!p) return false;

    char line[1024];
    *out_path = NULL;
    if (fgets(line, sizeof(line), p)) {
        line[strcspn(line, "\n")] = '\0';
        *out_path = strdup(line);
    }
    pclose(p);
    return *out_path != NULL;
}

/* ── Read library metadata ──────────────────────────────────── */

static bool ir_read_library(const char *path, char **json_out, size_t *json_size) {
    size_t file_size;
    char *data = ir_read_file(path, &file_size);
    if (!data) return false;

    if (file_size < HEADER_SIZE || memcmp(data, MAGIC, MAGIC_LEN) != 0) {
        free(data);
        return false;
    }

    uint32_t meta_len;
    memcpy(&meta_len, data + HEADER_META_LEN_OFFSET, 4);

    if (HEADER_SIZE + meta_len > file_size) {
        free(data);
        return false;
    }

    *json_out = malloc(meta_len + 1);
    memcpy(*json_out, data + HEADER_SIZE, meta_len);
    (*json_out)[meta_len] = '\0';
    *json_size = meta_len;

    free(data);
    return true;
}

bool ir_read_library_metadata(const char *path, IRMetadata *out_meta) {
    memset(out_meta, 0, sizeof(*out_meta));

    char *json = NULL;
    size_t json_len = 0;
    if (!ir_read_library(path, &json, &json_len)) return false;

    IRJP jp = {json, json + json_len};

    out_meta->name = ir_json_get_string(&jp, "name");
    out_meta->version = ir_json_get_string(&jp, "version");
    if (!out_meta->name) out_meta->name = strdup("unknown");
    if (!out_meta->version) out_meta->version = strdup("0.0.0");

    IRJP fjp = jp;
    ir_jp_skip(&fjp);
    if (fjp.p < fjp.end && *fjp.p == '{') {
        IRJP alias_arr = ir_jp_find(&fjp, "type_aliases");
        ir_jp_skip(&alias_arr);
        if (alias_arr.p < alias_arr.end && *alias_arr.p == '[') {
            out_meta->alias_count = ir_jp_array_count(alias_arr);
            if (out_meta->alias_count > 0) {
                out_meta->aliases = calloc(out_meta->alias_count, sizeof(IRTypeAlias));
                for (int i = 0; i < out_meta->alias_count; i++) {
                    IRJP item = ir_jp_array_item(alias_arr, i);
                    out_meta->aliases[i].name = ir_json_get_string(&item, "name");
                    out_meta->aliases[i].target_type = ir_json_get_string(&item, "target_type");
                }
            }
        }

        IRJP enum_arr = ir_jp_find(&fjp, "enums");
        ir_jp_skip(&enum_arr);
        if (enum_arr.p < enum_arr.end && *enum_arr.p == '[') {
            out_meta->enum_count = ir_jp_array_count(enum_arr);
            if (out_meta->enum_count > 0) {
                out_meta->enums = calloc(out_meta->enum_count, sizeof(IREnum));
                for (int i = 0; i < out_meta->enum_count; i++) {
                    IRJP item = ir_jp_array_item(enum_arr, i);
                    out_meta->enums[i].name = ir_json_get_string(&item, "name");

                    IRJP vi = item;
                    IRJP variants_arr = ir_jp_find(&vi, "variants");
                    ir_jp_skip(&variants_arr);
                    if (variants_arr.p < variants_arr.end && *variants_arr.p == '[') {
                        out_meta->enums[i].variant_count = ir_jp_array_count(variants_arr);
                        if (out_meta->enums[i].variant_count > 0) {
                            out_meta->enums[i].variants = calloc(out_meta->enums[i].variant_count, sizeof(IREnumVariant));
                            for (int j = 0; j < out_meta->enums[i].variant_count; j++) {
                                IRJP vic = ir_jp_array_item(variants_arr, j);
                                out_meta->enums[i].variants[j].name = ir_json_get_string(&vic, "name");
                                out_meta->enums[i].variants[j].value = ir_json_get_int(&vic, "value");
                            }
                        }
                    }
                }
            }
        }

        IRJP interface_arr = ir_jp_find(&fjp, "interfaces");
        ir_jp_skip(&interface_arr);
        if (interface_arr.p < interface_arr.end && *interface_arr.p == '[') {
            out_meta->interface_count = ir_jp_array_count(interface_arr);
            if (out_meta->interface_count > 0) {
                out_meta->interfaces = calloc(out_meta->interface_count, sizeof(IRInterface));
                for (int i = 0; i < out_meta->interface_count; i++) {
                    IRJP item = ir_jp_array_item(interface_arr, i);
                    out_meta->interfaces[i].name = ir_json_get_string(&item, "name");

                    IRJP mi = item;
                    IRJP methods_arr = ir_jp_find(&mi, "methods");
                    ir_jp_skip(&methods_arr);
                    if (methods_arr.p < methods_arr.end && *methods_arr.p == '[') {
                        out_meta->interfaces[i].method_count = ir_jp_array_count(methods_arr);
                        if (out_meta->interfaces[i].method_count > 0) {
                            out_meta->interfaces[i].methods = calloc(out_meta->interfaces[i].method_count, sizeof(IRInterfaceMethod));
                            for (int j = 0; j < out_meta->interfaces[i].method_count; j++) {
                                IRJP mic = ir_jp_array_item(methods_arr, j);
                                out_meta->interfaces[i].methods[j].name = ir_json_get_string(&mic, "name");
                                out_meta->interfaces[i].methods[j].return_type = ir_json_get_string(&mic, "return_type");
                                out_meta->interfaces[i].methods[j].is_variadic =
                                    ir_json_get_int(&mic, "is_variadic") != 0;
                                out_meta->interfaces[i].methods[j].params =
                                    ir_parse_params(mic, &out_meta->interfaces[i].methods[j].param_count);
                            }
                        }
                    }
                }
            }
        }

        IRJP struct_arr = ir_jp_find(&fjp, "structs");
        ir_jp_skip(&struct_arr);
        if (struct_arr.p < struct_arr.end && *struct_arr.p == '[') {
            out_meta->struct_count = ir_jp_array_count(struct_arr);
            if (out_meta->struct_count > 0) {
                out_meta->structs = calloc(out_meta->struct_count, sizeof(IRStruct));
                for (int i = 0; i < out_meta->struct_count; i++) {
                    IRJP item = ir_jp_array_item(struct_arr, i);
                    out_meta->structs[i].name = ir_json_get_string(&item, "name");

                    IRJP fi = item;
                    IRJP fields_arr = ir_jp_find(&fi, "fields");
                    ir_jp_skip(&fields_arr);
                    if (fields_arr.p < fields_arr.end && *fields_arr.p == '[') {
                        out_meta->structs[i].field_count = ir_jp_array_count(fields_arr);
                        if (out_meta->structs[i].field_count > 0) {
                            out_meta->structs[i].fields = calloc(out_meta->structs[i].field_count, sizeof(IRField));
                            for (int j = 0; j < out_meta->structs[i].field_count; j++) {
                                IRJP fic = ir_jp_array_item(fields_arr, j);
                                out_meta->structs[i].fields[j].name = ir_json_get_string(&fic, "name");
                                out_meta->structs[i].fields[j].type_name = ir_json_get_string(&fic, "type");
                                out_meta->structs[i].fields[j].is_ptr = ir_json_get_int(&fic, "is_ptr");
                                out_meta->structs[i].fields[j].array_size = ir_json_get_int(&fic, "array_size");
                            }
                        }
                    }

                    IRJP mi = item;
                    IRJP methods_arr = ir_jp_find(&mi, "methods");
                    ir_jp_skip(&methods_arr);
                    if (methods_arr.p < methods_arr.end && *methods_arr.p == '[') {
                        out_meta->structs[i].method_count = ir_jp_array_count(methods_arr);
                        if (out_meta->structs[i].method_count > 0) {
                            out_meta->structs[i].methods = calloc(out_meta->structs[i].method_count, sizeof(IRFunc));
                            for (int j = 0; j < out_meta->structs[i].method_count; j++) {
                                IRJP mic = ir_jp_array_item(methods_arr, j);
                                out_meta->structs[i].methods[j].name = ir_json_get_string(&mic, "name");
                                out_meta->structs[i].methods[j].return_type = ir_json_get_string(&mic, "return_type");
                                out_meta->structs[i].methods[j].is_extern =
                                    ir_json_get_int(&mic, "is_extern") != 0;
                                out_meta->structs[i].methods[j].is_static =
                                    ir_json_get_int(&mic, "is_static") != 0;
                                out_meta->structs[i].methods[j].is_variadic =
                                    ir_json_get_int(&mic, "is_variadic") != 0;
                                out_meta->structs[i].methods[j].params =
                                    ir_parse_params(mic, &out_meta->structs[i].methods[j].param_count);
                            }
                        }
                    }
                }
            }
        }

        IRJP global_arr = ir_jp_find(&fjp, "globals");
        ir_jp_skip(&global_arr);
        if (global_arr.p < global_arr.end && *global_arr.p == '[') {
            out_meta->global_count = ir_jp_array_count(global_arr);
            if (out_meta->global_count > 0) {
                out_meta->globals = calloc(out_meta->global_count, sizeof(IRGlobal));
                for (int i = 0; i < out_meta->global_count; i++) {
                    IRJP item = ir_jp_array_item(global_arr, i);
                    out_meta->globals[i].name = ir_json_get_string(&item, "name");
                    out_meta->globals[i].type_name = ir_json_get_string(&item, "type");
                    out_meta->globals[i].is_ptr = ir_json_get_int(&item, "is_ptr");
                    out_meta->globals[i].is_extern = ir_json_get_int(&item, "is_extern") != 0;
                    out_meta->globals[i].array_size = ir_json_get_int(&item, "array_size");
                }
            }
        }

        IRJP func_arr = ir_jp_find(&fjp, "functions");
        ir_jp_skip(&func_arr);
        if (func_arr.p < func_arr.end && *func_arr.p == '[') {
            out_meta->func_count = ir_jp_array_count(func_arr);
            if (out_meta->func_count > 0) {
                out_meta->funcs = calloc(out_meta->func_count, sizeof(IRFunc));
                for (int i = 0; i < out_meta->func_count; i++) {
                    IRJP item = ir_jp_array_item(func_arr, i);
                    out_meta->funcs[i].name = ir_json_get_string(&item, "name");
                    out_meta->funcs[i].return_type = ir_json_get_string(&item, "return_type");
                    out_meta->funcs[i].is_extern = ir_json_get_int(&item, "is_extern") != 0;
                    out_meta->funcs[i].is_static = ir_json_get_int(&item, "is_static") != 0;
                    out_meta->funcs[i].is_variadic = ir_json_get_int(&item, "is_variadic") != 0;
                    out_meta->funcs[i].params =
                        ir_parse_params(item, &out_meta->funcs[i].param_count);
                }
            }
        }
    }

    free(json);
    return true;
}

/* ── Inject import declarations into program ────────────────── */

static FuncParamList *ir_build_func_params(IRParam *params, int count) {
    FuncParamList *list = NULL;
    for (int i = 0; i < count; i++) {
        FuncParam fp = func_param_new(
            params[i].type_name ? params[i].type_name : "i32",
            params[i].name ? params[i].name : "",
            params[i].is_ptr);
        func_param_list_append(&list, fp);
    }
    return list;
}

static char *ir_qualified_name(const char *package_name, const char *name) {
    const char *pkg = package_name ? package_name : "";
    const char *sym = name ? name : "";
    size_t pkg_len = strlen(pkg);
    size_t sym_len = strlen(sym);
    char *result = malloc(pkg_len + sym_len + 2);
    memcpy(result, pkg, pkg_len);
    result[pkg_len] = '.';
    memcpy(result + pkg_len + 1, sym, sym_len);
    result[pkg_len + sym_len + 1] = '\0';
    return result;
}

static int ir_decl_symbol_matches(Decl *decl, const char *name) {
    if (!decl || !name) return 0;
    switch (decl->type) {
    case DECL_FUNC:
        return decl->data.func.name && strcmp(decl->data.func.name, name) == 0;
    case DECL_GLOBAL_VAR:
        return decl->data.global_var.name && strcmp(decl->data.global_var.name, name) == 0;
    case DECL_STRUCT:
        return decl->data.struct_decl.name && strcmp(decl->data.struct_decl.name, name) == 0;
    case DECL_TYPE_ALIAS:
        return decl->data.type_alias.name && strcmp(decl->data.type_alias.name, name) == 0;
    case DECL_ENUM:
        if (decl->data.enum_decl.name && strcmp(decl->data.enum_decl.name, name) == 0)
            return 1;
        for (EnumVariantList *v = decl->data.enum_decl.variants; v; v = v->next) {
            if (v->name && strcmp(v->name, name) == 0)
                return 1;
        }
        return 0;
    case DECL_INTERFACE:
        return decl->data.interface.name && strcmp(decl->data.interface.name, name) == 0;
    default:
        return 0;
    }
}

static int ir_program_symbol_exists(Program *program, const char *name) {
    for (DeclList *dl = program->decls; dl; dl = dl->next) {
        if (ir_decl_symbol_matches(dl->decl, name))
            return 1;
    }
    return 0;
}

static EnumVariantList *ir_build_enum_variants(IREnum *pe,
                                               const char *package_name,
                                               int qualified) {
    EnumVariantList *variants = NULL;
    for (int j = 0; j < pe->variant_count; j++) {
        char *name = qualified
            ? ir_qualified_name(package_name, pe->variants[j].name)
            : NULL;
        EnumVariantList *variant = enum_variant_list_new(
            qualified ? name : (pe->variants[j].name ? pe->variants[j].name : ""),
            pe->variants[j].value);
        free(name);
        enum_variant_list_append(&variants, variant);
    }
    return variants;
}

static InterfaceMethodList *ir_build_interface_methods(IRInterface *pi) {
    InterfaceMethodList *methods = NULL;
    for (int j = 0; j < pi->method_count; j++) {
        IRInterfaceMethod *pm = &pi->methods[j];
        InterfaceMethod method = interface_method_new(
            pm->return_type ? pm->return_type : "void",
            pm->name ? pm->name : "",
            ir_build_func_params(pm->params, pm->param_count),
            pm->is_variadic);
        interface_method_list_append(&methods, method);
    }
    return methods;
}

static StructFieldList *ir_build_struct_fields(IRStruct *ps) {
    StructFieldList *fields = NULL;
    for (int j = 0; j < ps->field_count; j++) {
        IRField *pf = &ps->fields[j];
        StructField field = struct_field_new(
            pf->type_name ? pf->type_name : "i32",
            pf->name ? pf->name : "",
            pf->is_ptr,
            pf->array_size);
        struct_field_list_append(&fields, field);
    }
    return fields;
}

static DeclList *ir_build_struct_methods(IRStruct *ps, SourceLocation loc) {
    DeclList *methods = NULL;
    for (int j = 0; j < ps->method_count; j++) {
        IRFunc *pm = &ps->methods[j];
        Decl *method = decl_new_func(
            pm->return_type ? pm->return_type : "void",
            pm->name ? pm->name : "",
            ir_build_func_params(pm->params, pm->param_count), NULL,
            NULL, NULL, pm->is_static, pm->is_extern, pm->is_variadic, loc);
        decl_list_append(&methods, method);
    }
    return methods;
}

static void ir_insert_decl(Program *program, Decl **after_decl, Decl *decl) {
    program_insert_decl_after(program, *after_decl, decl);
    *after_decl = decl;
}

bool ir_inject_import(const char *package_name, Program *program, Decl *after_decl, char **out_lib_path) {
    char *lib = NULL;
    if (!ir_find_library(package_name, &lib)) {
        fprintf(stderr, "import error: package '%s' not found\n", package_name);
        return false;
    }

    IRMetadata meta;
    if (!ir_read_library_metadata(lib, &meta)) {
        fprintf(stderr, "import error: cannot read metadata from '%s'\n", lib);
        free(lib);
        return false;
    }

    SourceLocation loc = {program->filename, 0, 0};

    for (int i = 0; i < meta.alias_count; i++) {
        IRTypeAlias *pa = &meta.aliases[i];
        char *qualified = ir_qualified_name(package_name, pa->name);
        Decl *qdecl = decl_new_type_alias(
            qualified,
            pa->target_type ? pa->target_type : "i32",
            loc);
        ir_insert_decl(program, &after_decl, qdecl);
        free(qualified);

        if (!ir_program_symbol_exists(program, pa->name)) {
            Decl *decl = decl_new_type_alias(
                pa->name ? pa->name : "",
                pa->target_type ? pa->target_type : "i32",
                loc);
            ir_insert_decl(program, &after_decl, decl);
        }
    }

    for (int i = 0; i < meta.enum_count; i++) {
        IREnum *pe = &meta.enums[i];
        char *qualified = ir_qualified_name(package_name, pe->name);
        Decl *qdecl = decl_new_enum(qualified,
                                    ir_build_enum_variants(pe, package_name, 1),
                                    loc);
        ir_insert_decl(program, &after_decl, qdecl);
        free(qualified);

        if (!ir_program_symbol_exists(program, pe->name)) {
            Decl *decl = decl_new_enum(pe->name ? pe->name : "",
                                       ir_build_enum_variants(pe, package_name, 0),
                                       loc);
            ir_insert_decl(program, &after_decl, decl);
        }
    }

    for (int i = 0; i < meta.interface_count; i++) {
        IRInterface *pi = &meta.interfaces[i];
        char *qualified = ir_qualified_name(package_name, pi->name);
        Decl *qdecl = decl_new_interface(qualified,
                                         ir_build_interface_methods(pi),
                                         NULL, loc);
        ir_insert_decl(program, &after_decl, qdecl);
        free(qualified);

        if (!ir_program_symbol_exists(program, pi->name)) {
            Decl *decl = decl_new_interface(pi->name ? pi->name : "",
                                            ir_build_interface_methods(pi),
                                            NULL, loc);
            ir_insert_decl(program, &after_decl, decl);
        }
    }

    for (int i = 0; i < meta.struct_count; i++) {
        IRStruct *ps = &meta.structs[i];
        char *qualified = ir_qualified_name(package_name, ps->name);
        Decl *qdecl = decl_new_struct(qualified,
                                      ir_build_struct_fields(ps),
                                      NULL,
                                      ir_build_struct_methods(ps, loc),
                                      NULL, loc);
        ir_insert_decl(program, &after_decl, qdecl);
        free(qualified);

        if (!ir_program_symbol_exists(program, ps->name)) {
            Decl *decl = decl_new_struct(
                ps->name ? ps->name : "",
                ir_build_struct_fields(ps), NULL,
                ir_build_struct_methods(ps, loc), NULL, loc);
            ir_insert_decl(program, &after_decl, decl);
        }
    }

    for (int i = 0; i < meta.global_count; i++) {
        IRGlobal *pg = &meta.globals[i];
        char *qualified = ir_qualified_name(package_name, pg->name);
        Decl *qdecl = decl_new_global_var(
            pg->type_name ? pg->type_name : "i32",
            qualified,
            NULL,
            pg->is_ptr,
            0,
            1,
            pg->array_size,
            loc);
        ir_insert_decl(program, &after_decl, qdecl);
        free(qualified);

        if (!ir_program_symbol_exists(program, pg->name)) {
            Decl *decl = decl_new_global_var(
                pg->type_name ? pg->type_name : "i32",
                pg->name ? pg->name : "",
                NULL,
                pg->is_ptr,
                0,
                1,
                pg->array_size,
                loc);
            ir_insert_decl(program, &after_decl, decl);
        }
    }

    for (int i = 0; i < meta.func_count; i++) {
        IRFunc *pf = &meta.funcs[i];
        char *qualified = ir_qualified_name(package_name, pf->name);
        Decl *qdecl = decl_new_func(
            pf->return_type ? pf->return_type : "void",
            qualified,
            ir_build_func_params(pf->params, pf->param_count), NULL,
            NULL, NULL, pf->is_static, pf->is_extern, pf->is_variadic, loc);
        ir_insert_decl(program, &after_decl, qdecl);
        free(qualified);

        if (!ir_program_symbol_exists(program, pf->name)) {
            Decl *decl = decl_new_func(
                pf->return_type ? pf->return_type : "void",
                pf->name ? pf->name : "",
                ir_build_func_params(pf->params, pf->param_count), NULL,
                NULL, NULL, pf->is_static, pf->is_extern, pf->is_variadic, loc);
            ir_insert_decl(program, &after_decl, decl);
        }
    }

    if (out_lib_path) *out_lib_path = lib;
    else free(lib);

    ir_metadata_free(&meta);
    return true;
}

void ir_metadata_free(IRMetadata *meta) {
    free(meta->name);
    free(meta->version);
    for (int i = 0; i < meta->alias_count; i++) {
        free(meta->aliases[i].name);
        free(meta->aliases[i].target_type);
    }
    free(meta->aliases);
    for (int i = 0; i < meta->enum_count; i++) {
        free(meta->enums[i].name);
        for (int j = 0; j < meta->enums[i].variant_count; j++)
            free(meta->enums[i].variants[j].name);
        free(meta->enums[i].variants);
    }
    free(meta->enums);
    for (int i = 0; i < meta->interface_count; i++) {
        free(meta->interfaces[i].name);
        for (int j = 0; j < meta->interfaces[i].method_count; j++) {
            free(meta->interfaces[i].methods[j].name);
            free(meta->interfaces[i].methods[j].return_type);
            for (int k = 0; k < meta->interfaces[i].methods[j].param_count; k++) {
                free(meta->interfaces[i].methods[j].params[k].name);
                free(meta->interfaces[i].methods[j].params[k].type_name);
            }
            free(meta->interfaces[i].methods[j].params);
        }
        free(meta->interfaces[i].methods);
    }
    free(meta->interfaces);
    for (int i = 0; i < meta->struct_count; i++) {
        free(meta->structs[i].name);
        for (int j = 0; j < meta->structs[i].field_count; j++) {
            free(meta->structs[i].fields[j].name);
            free(meta->structs[i].fields[j].type_name);
        }
        free(meta->structs[i].fields);
        for (int j = 0; j < meta->structs[i].method_count; j++) {
            free(meta->structs[i].methods[j].name);
            free(meta->structs[i].methods[j].return_type);
            for (int k = 0; k < meta->structs[i].methods[j].param_count; k++) {
                free(meta->structs[i].methods[j].params[k].name);
                free(meta->structs[i].methods[j].params[k].type_name);
            }
            free(meta->structs[i].methods[j].params);
        }
        free(meta->structs[i].methods);
    }
    free(meta->structs);
    for (int i = 0; i < meta->global_count; i++) {
        free(meta->globals[i].name);
        free(meta->globals[i].type_name);
    }
    free(meta->globals);
    for (int i = 0; i < meta->func_count; i++) {
        free(meta->funcs[i].name);
        free(meta->funcs[i].return_type);
        for (int j = 0; j < meta->funcs[i].param_count; j++) {
            free(meta->funcs[i].params[j].name);
            free(meta->funcs[i].params[j].type_name);
        }
        free(meta->funcs[i].params);
    }
    free(meta->funcs);
    memset(meta, 0, sizeof(*meta));
}

bool ir_extract_library_object(const char *lib_path, const char *dest_path) {
    size_t file_size;
    char *data = ir_read_file(lib_path, &file_size);
    if (!data) return false;

    if (file_size < HEADER_SIZE || memcmp(data, MAGIC, MAGIC_LEN) != 0) {
        free(data);
        return false;
    }

    uint32_t meta_len;
    memcpy(&meta_len, data + HEADER_META_LEN_OFFSET, 4);

    if (HEADER_SIZE + meta_len > file_size) {
        free(data);
        return false;
    }

    size_t lib_off = HEADER_SIZE + meta_len;
    size_t lib_size = file_size - lib_off;

    if (lib_size == 0) {
        free(data);
        return false;
    }

    FILE *f = fopen(dest_path, "wb");
    if (!f) { free(data); return false; }
    fwrite(data + lib_off, 1, lib_size, f);
    fclose(f);
    free(data);
    return true;
}
