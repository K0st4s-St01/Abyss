#include "../../include/dwellerpkg/dwellerpkg.h"
#include "../../../abyssc/include/lexer/lexer.h"
#include "../../../abyssc/include/lexer/vec_token.h"
#include "../../../abyssc/include/parser/parser.h"
#include "../../../abyssc/include/ast/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

/* ── Magic ──────────────────────────────────────────────────── */

static const char MAGIC[] = "DWELLERPKG\0";
#define MAGIC_LEN 11
#define HEADER_META_LEN_OFFSET MAGIC_LEN
#define HEADER_SIZE (MAGIC_LEN + 4)

/* ── Helpers ────────────────────────────────────────────────── */

static bool ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
        return true;
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

static bool ensure_path(const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            ensure_dir(tmp);
            *p = '/';
        }
    }
    return ensure_dir(path);
}

static char *read_file_raw(const char *path, size_t *out_size) {
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

/* ── Simple JSON builder (no dependencies) ──────────────────── */

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} JsonBuf;

static void json_init(JsonBuf *b) {
    b->cap = 4096;
    b->data = malloc(b->cap);
    b->len = 0;
    b->data[0] = '\0';
}

static void json_raw(JsonBuf *b, const char *s, size_t n) {
    while (b->len + n + 1 > b->cap) {
        b->cap *= 2;
        b->data = realloc(b->data, b->cap);
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void json_str(JsonBuf *b, const char *s) {
    json_raw(b, "\"", 1);
    for (const char *p = s; *p; p++) {
        if (*p == '"' || *p == '\\') {
            char esc[2] = {'\\', *p};
            json_raw(b, esc, 2);
        } else {
            json_raw(b, p, 1);
        }
    }
    json_raw(b, "\"", 1);
}

static void json_int(JsonBuf *b, int v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", v);
    json_raw(b, buf, strlen(buf));
}

static void json_kv_str(JsonBuf *b, const char *k, const char *v) {
    json_str(b, k);
    json_raw(b, ":", 1);
    json_str(b, v);
}

static void json_kv_int(JsonBuf *b, const char *k, int v) {
    json_str(b, k);
    json_raw(b, ":", 1);
    json_int(b, v);
}

/* ── JSON parser (minimal, recursive descent) ───────────────── */

typedef struct {
    const char *p;
    const char *end;
} JP;

static void jp_skip(JP *jp) {
    while (jp->p < jp->end && (*jp->p == ' ' || *jp->p == '\t' ||
           *jp->p == '\n' || *jp->p == '\r'))
        jp->p++;
}

static char *jp_parse_str(JP *jp) {
    jp_skip(jp);
    if (jp->p >= jp->end || *jp->p != '"') return NULL;
    jp->p++;
    size_t cap = 64, len = 0;
    char *s = malloc(cap);
    while (jp->p < jp->end && *jp->p != '"') {
        char c = *jp->p++;
        if (c == '\\' && jp->p < jp->end) {
            c = *jp->p++;
        }
        if (len + 1 >= cap) { cap *= 2; s = realloc(s, cap); }
        s[len++] = c;
    }
    if (jp->p < jp->end) jp->p++;
    s[len] = '\0';
    return s;
}

static int jp_parse_int(JP *jp) {
    jp_skip(jp);
    int neg = 0;
    if (jp->p < jp->end && *jp->p == '-') { neg = 1; jp->p++; }
    int val = 0;
    while (jp->p < jp->end && *jp->p >= '0' && *jp->p <= '9')
        val = val * 10 + (*jp->p++ - '0');
    return neg ? -val : val;
}

static void jp_skip_value(JP *jp) {
    jp_skip(jp);
    if (jp->p >= jp->end) return;
    if (*jp->p == '"') { jp_parse_str(jp); return; }
    if (*jp->p == '{' || *jp->p == '[') {
        char open = *jp->p++;
        char close = (open == '{') ? '}' : ']';
        int depth = 1;
        int in_str = 0;
        while (jp->p < jp->end && depth > 0) {
            if (*jp->p == '"' && !in_str) { in_str = 1; }
            else if (*jp->p == '"' && in_str) { in_str = 0; }
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

/* ── Metadata serialization ─────────────────────────────────── */

char *pkg_metadata_to_json(const PkgMetadata *meta) {
    JsonBuf b;
    json_init(&b);

    json_raw(&b, "{", 1);
    json_kv_str(&b, "name", meta->name);
    json_raw(&b, ",", 1);
    json_kv_str(&b, "version", meta->version);

    json_raw(&b, ",\"type_aliases\":[", 17);
    for (int i = 0; i < meta->alias_count; i++) {
        if (i > 0) json_raw(&b, ",", 1);
        json_raw(&b, "{", 1);
        json_kv_str(&b, "name", meta->aliases[i].name);
        json_raw(&b, ",", 1);
        json_kv_str(&b, "target_type", meta->aliases[i].target_type);
        json_raw(&b, "}", 1);
    }
    json_raw(&b, "]", 1);

    json_raw(&b, ",\"enums\":[", 10);
    for (int i = 0; i < meta->enum_count; i++) {
        if (i > 0) json_raw(&b, ",", 1);
        PkgEnum *e = &meta->enums[i];
        json_raw(&b, "{", 1);
        json_kv_str(&b, "name", e->name);
        json_raw(&b, ",\"variants\":[", 13);
        for (int j = 0; j < e->variant_count; j++) {
            if (j > 0) json_raw(&b, ",", 1);
            json_raw(&b, "{", 1);
            json_kv_str(&b, "name", e->variants[j].name);
            json_raw(&b, ",", 1);
            json_kv_int(&b, "value", e->variants[j].value);
            json_raw(&b, "}", 1);
        }
        json_raw(&b, "]}", 2);
    }
    json_raw(&b, "]", 1);

    json_raw(&b, ",\"interfaces\":[", 15);
    for (int i = 0; i < meta->interface_count; i++) {
        if (i > 0) json_raw(&b, ",", 1);
        PkgInterface *iface = &meta->interfaces[i];
        json_raw(&b, "{", 1);
        json_kv_str(&b, "name", iface->name);
        json_raw(&b, ",\"methods\":[", 12);
        for (int j = 0; j < iface->method_count; j++) {
            if (j > 0) json_raw(&b, ",", 1);
            PkgInterfaceMethod *m = &iface->methods[j];
            json_raw(&b, "{", 1);
            json_kv_str(&b, "name", m->name);
            json_raw(&b, ",", 1);
            json_kv_str(&b, "return_type", m->return_type);
            json_raw(&b, ",", 1);
            json_kv_int(&b, "is_variadic", m->is_variadic ? 1 : 0);
            json_raw(&b, ",\"params\":[", 11);
            for (int k = 0; k < m->param_count; k++) {
                if (k > 0) json_raw(&b, ",", 1);
                json_raw(&b, "{", 1);
                json_kv_str(&b, "name", m->params[k].name);
                json_raw(&b, ",", 1);
                json_kv_str(&b, "type", m->params[k].type_name);
                json_raw(&b, ",", 1);
                json_kv_int(&b, "is_ptr", m->params[k].is_ptr);
                json_raw(&b, "}", 1);
            }
            json_raw(&b, "]}", 2);
        }
        json_raw(&b, "]}", 2);
    }
    json_raw(&b, "]", 1);

    /* structs */
    json_raw(&b, ",\"structs\":[", 12);
    for (int i = 0; i < meta->struct_count; i++) {
        if (i > 0) json_raw(&b, ",", 1);
        PkgStruct *s = &meta->structs[i];
        json_raw(&b, "{", 1);
        json_kv_str(&b, "name", s->name);

        json_raw(&b, ",\"fields\":[", 11);
        for (int j = 0; j < s->field_count; j++) {
            if (j > 0) json_raw(&b, ",", 1);
            json_raw(&b, "{", 1);
            json_kv_str(&b, "name", s->fields[j].name);
            json_raw(&b, ",", 1);
            json_kv_str(&b, "type", s->fields[j].type_name);
            json_raw(&b, ",", 1);
            json_kv_int(&b, "is_ptr", s->fields[j].is_ptr);
            json_raw(&b, ",", 1);
            json_kv_int(&b, "array_size", s->fields[j].array_size);
            json_raw(&b, "}", 1);
        }
        json_raw(&b, "]", 1);

        if (s->generic_param_count > 0) {
            json_raw(&b, ",\"generic_params\":[", 19);
            for (int j = 0; j < s->generic_param_count; j++) {
                if (j > 0) json_raw(&b, ",", 1);
                json_raw(&b, "{", 1);
                json_kv_str(&b, "name", s->generic_params[j].name);
                json_raw(&b, "}", 1);
            }
            json_raw(&b, "]", 1);
        }

        json_raw(&b, ",\"methods\":[", 12);
        for (int j = 0; j < s->method_count; j++) {
            if (j > 0) json_raw(&b, ",", 1);
            PkgFunc *m = &s->methods[j];
            json_raw(&b, "{", 1);
            json_kv_str(&b, "name", m->name);
            json_raw(&b, ",", 1);
            json_kv_str(&b, "return_type", m->return_type);
            json_raw(&b, ",", 1);
            json_kv_int(&b, "is_extern", m->is_extern ? 1 : 0);
            json_raw(&b, ",", 1);
            json_kv_int(&b, "is_static", m->is_static ? 1 : 0);
            json_raw(&b, ",", 1);
            json_kv_int(&b, "is_variadic", m->is_variadic ? 1 : 0);

            json_raw(&b, ",\"params\":[", 11);
            for (int k = 0; k < m->param_count; k++) {
                if (k > 0) json_raw(&b, ",", 1);
                json_raw(&b, "{", 1);
                json_kv_str(&b, "name", m->params[k].name);
                json_raw(&b, ",", 1);
                json_kv_str(&b, "type", m->params[k].type_name);
                json_raw(&b, ",", 1);
                json_kv_int(&b, "is_ptr", m->params[k].is_ptr);
                json_raw(&b, "}", 1);
            }
            json_raw(&b, "]", 1);

            if (m->generic_param_count > 0) {
                json_raw(&b, ",\"generic_params\":[", 19);
                for (int k = 0; k < m->generic_param_count; k++) {
                    if (k > 0) json_raw(&b, ",", 1);
                    json_raw(&b, "{", 1);
                    json_kv_str(&b, "name", m->generic_params[k].name);
                    json_raw(&b, "}", 1);
                }
                json_raw(&b, "]", 1);
            }

            json_raw(&b, "}", 1);
        }
        json_raw(&b, "]", 1);

        json_raw(&b, "}", 1);
    }
    json_raw(&b, "]", 1);

    json_raw(&b, ",\"globals\":[", 12);
    for (int i = 0; i < meta->global_count; i++) {
        if (i > 0) json_raw(&b, ",", 1);
        PkgGlobal *g = &meta->globals[i];
        json_raw(&b, "{", 1);
        json_kv_str(&b, "name", g->name);
        json_raw(&b, ",", 1);
        json_kv_str(&b, "type", g->type_name);
        json_raw(&b, ",", 1);
        json_kv_int(&b, "is_ptr", g->is_ptr);
        json_raw(&b, ",", 1);
        json_kv_int(&b, "is_extern", g->is_extern ? 1 : 0);
        json_raw(&b, ",", 1);
        json_kv_int(&b, "array_size", g->array_size);
        json_raw(&b, "}", 1);
    }
    json_raw(&b, "]", 1);

    /* functions */
    json_raw(&b, ",\"functions\":[", 14);
    for (int i = 0; i < meta->func_count; i++) {
        if (i > 0) json_raw(&b, ",", 1);
        PkgFunc *f = &meta->funcs[i];
        json_raw(&b, "{", 1);
        json_kv_str(&b, "name", f->name);
        json_raw(&b, ",", 1);
        json_kv_str(&b, "return_type", f->return_type);
        json_raw(&b, ",", 1);
        json_kv_int(&b, "is_extern", f->is_extern ? 1 : 0);
        json_raw(&b, ",", 1);
        json_kv_int(&b, "is_static", f->is_static ? 1 : 0);
        json_raw(&b, ",", 1);
        json_kv_int(&b, "is_variadic", f->is_variadic ? 1 : 0);

        json_raw(&b, ",\"params\":[", 11);
        for (int j = 0; j < f->param_count; j++) {
            if (j > 0) json_raw(&b, ",", 1);
            json_raw(&b, "{", 1);
            json_kv_str(&b, "name", f->params[j].name);
            json_raw(&b, ",", 1);
            json_kv_str(&b, "type", f->params[j].type_name);
            json_raw(&b, ",", 1);
            json_kv_int(&b, "is_ptr", f->params[j].is_ptr);
            json_raw(&b, "}", 1);
        }
        json_raw(&b, "]", 1);

        if (f->generic_param_count > 0) {
            json_raw(&b, ",\"generic_params\":[", 19);
            for (int j = 0; j < f->generic_param_count; j++) {
                if (j > 0) json_raw(&b, ",", 1);
                json_raw(&b, "{", 1);
                json_kv_str(&b, "name", f->generic_params[j].name);
                json_raw(&b, "}", 1);
            }
            json_raw(&b, "]", 1);
        }

        json_raw(&b, "}", 1);
    }
    json_raw(&b, "]", 1);

    json_raw(&b, "}", 1);
    return b.data;
}

/* ── Metadata deserialization ───────────────────────────────── */

static char *json_get_string(JP *jp, const char *key) {
    /* find "key": */
    size_t klen = strlen(key);
    while (jp->p < jp->end) {
        jp_skip(jp);
        if (*jp->p == '{' || *jp->p == '[') { jp->p++; continue; }
        if (*jp->p == '}' || *jp->p == ']' || *jp->p == ',') { jp->p++; continue; }
        if (*jp->p != '"') { jp_skip_value(jp); continue; }
        const char *start = ++jp->p;
        while (jp->p < jp->end && *jp->p != '"') jp->p++;
        if (jp->p - start == (long)klen && memcmp(start, key, klen) == 0) {
            jp->p++;
            jp_skip(jp);
            if (jp->p < jp->end && *jp->p == ':') jp->p++;
            jp_skip(jp);
            return jp_parse_str(jp);
        }
        jp->p++;
        jp_skip(jp);
        if (jp->p < jp->end && *jp->p == ':') { jp->p++; jp_skip_value(jp); }
    }
    return NULL;
}

static int json_get_int(JP *jp, const char *key) {
    size_t klen = strlen(key);
    while (jp->p < jp->end) {
        jp_skip(jp);
        if (*jp->p == '{' || *jp->p == '[') { jp->p++; continue; }
        if (*jp->p == '}' || *jp->p == ']' || *jp->p == ',') { jp->p++; continue; }
        if (*jp->p != '"') { jp_skip_value(jp); continue; }
        const char *start = ++jp->p;
        while (jp->p < jp->end && *jp->p != '"') jp->p++;
        if (jp->p - start == (long)klen && memcmp(start, key, klen) == 0) {
            jp->p++;
            jp_skip(jp);
            if (jp->p < jp->end && *jp->p == ':') jp->p++;
            jp_skip(jp);
            return jp_parse_int(jp);
        }
        jp->p++;
        jp_skip(jp);
        if (jp->p < jp->end && *jp->p == ':') { jp->p++; jp_skip_value(jp); }
    }
    return 0;
}

/* Find the Nth occurrence of key in a JSON array context. */
static JP jp_find_array_item(JP base, const char *key, int index) {
    JP scan = base;
    int count = 0;
    while (scan.p < scan.end) {
        jp_skip(&scan);
        if (scan.p >= scan.end) break;
        if (*scan.p == ',') { scan.p++; continue; }
        if (*scan.p != '{') break;
        if (count == index) return scan;
        /* skip this object */
        int depth = 1;
        scan.p++;
        while (scan.p < scan.end && depth > 0) {
            if (*scan.p == '{') depth++;
            else if (*scan.p == '}') depth--;
            scan.p++;
        }
        count++;
    }
    return base;
}

static bool jp_find_array(JP *jp, const char *key) {
    size_t klen = strlen(key);
    while (jp->p < jp->end) {
        jp_skip(jp);
        if (*jp->p == '{' || *jp->p == '[') { jp->p++; continue; }
        if (*jp->p == '}' || *jp->p == ']' || *jp->p == ',') { jp->p++; continue; }
        if (*jp->p != '"') { jp_skip_value(jp); continue; }
        const char *start = ++jp->p;
        while (jp->p < jp->end && *jp->p != '"') jp->p++;
        if (jp->p - start == (long)klen && memcmp(start, key, klen) == 0) {
            jp->p++;
            jp_skip(jp);
            if (jp->p < jp->end && *jp->p == ':') jp->p++;
            jp_skip(jp);
            if (jp->p < jp->end && *jp->p == '[') {
                jp->p++;
                return true;
            }
            return false;
        }
        jp->p++;
        jp_skip(jp);
        if (jp->p < jp->end && *jp->p == ':') { jp->p++; jp_skip_value(jp); }
    }
    return false;
}

static int jp_array_count(JP arr) {
    int count = 0;
    jp_skip(&arr);
    if (arr.p >= arr.end || *arr.p != ']') {
        count = 1;
        int depth = 0;
        while (arr.p < arr.end) {
            if (*arr.p == '{' || *arr.p == '[') depth++;
            else if (*arr.p == '}' || *arr.p == ']') {
                if (depth == 0) break;
                depth--;
            }
            else if (*arr.p == ',' && depth == 0) count++;
            arr.p++;
        }
    }
    return count;
}

static PkgParam *pkg_parse_params(JP item, int *out_count) {
    *out_count = 0;
    JP pi = item;
    if (!jp_find_array(&pi, "params"))
        return NULL;

    *out_count = jp_array_count(pi);
    if (*out_count <= 0)
        return NULL;

    PkgParam *params = calloc(*out_count, sizeof(PkgParam));
    for (int j = 0; j < *out_count; j++) {
        JP pic = jp_find_array_item(pi, "params", j);
        params[j].name = json_get_string(&pic, "name");
        params[j].type_name = json_get_string(&pic, "type");
        params[j].is_ptr = json_get_int(&pic, "is_ptr");
    }
    return params;
}

bool pkg_metadata_from_json(const char *json, size_t len, PkgMetadata *out) {
    memset(out, 0, sizeof(*out));
    JP jp = {json, json + len};

    out->name = json_get_string(&jp, "name");
    out->version = json_get_string(&jp, "version");
    if (!out->name) out->name = strdup("unknown");
    if (!out->version) out->version = strdup("0.0.0");

    JP ajp = jp;
    if (jp_find_array(&ajp, "type_aliases")) {
        out->alias_count = jp_array_count(ajp);
        if (out->alias_count > 0) {
            out->aliases = calloc(out->alias_count, sizeof(PkgTypeAlias));
            for (int i = 0; i < out->alias_count; i++) {
                JP item = jp_find_array_item(ajp, "type_aliases", i);
                out->aliases[i].name = json_get_string(&item, "name");
                out->aliases[i].target_type = json_get_string(&item, "target_type");
            }
        }
    }

    JP ejp = jp;
    if (jp_find_array(&ejp, "enums")) {
        out->enum_count = jp_array_count(ejp);
        if (out->enum_count > 0) {
            out->enums = calloc(out->enum_count, sizeof(PkgEnum));
            for (int i = 0; i < out->enum_count; i++) {
                JP item = jp_find_array_item(ejp, "enums", i);
                out->enums[i].name = json_get_string(&item, "name");

                JP vi = item;
                if (jp_find_array(&vi, "variants")) {
                    out->enums[i].variant_count = jp_array_count(vi);
                    if (out->enums[i].variant_count > 0) {
                        out->enums[i].variants = calloc(out->enums[i].variant_count, sizeof(PkgEnumVariant));
                        for (int j = 0; j < out->enums[i].variant_count; j++) {
                            JP vic = jp_find_array_item(vi, "variants", j);
                            out->enums[i].variants[j].name = json_get_string(&vic, "name");
                            out->enums[i].variants[j].value = json_get_int(&vic, "value");
                        }
                    }
                }
            }
        }
    }

    JP ijp = jp;
    if (jp_find_array(&ijp, "interfaces")) {
        out->interface_count = jp_array_count(ijp);
        if (out->interface_count > 0) {
            out->interfaces = calloc(out->interface_count, sizeof(PkgInterface));
            for (int i = 0; i < out->interface_count; i++) {
                JP item = jp_find_array_item(ijp, "interfaces", i);
                out->interfaces[i].name = json_get_string(&item, "name");

                JP mi = item;
                if (jp_find_array(&mi, "methods")) {
                    out->interfaces[i].method_count = jp_array_count(mi);
                    if (out->interfaces[i].method_count > 0) {
                        out->interfaces[i].methods = calloc(out->interfaces[i].method_count, sizeof(PkgInterfaceMethod));
                        for (int j = 0; j < out->interfaces[i].method_count; j++) {
                            JP mic = jp_find_array_item(mi, "methods", j);
                            out->interfaces[i].methods[j].name = json_get_string(&mic, "name");
                            out->interfaces[i].methods[j].return_type = json_get_string(&mic, "return_type");
                            out->interfaces[i].methods[j].is_variadic =
                                json_get_int(&mic, "is_variadic") != 0;
                            out->interfaces[i].methods[j].params =
                                pkg_parse_params(mic, &out->interfaces[i].methods[j].param_count);
                        }
                    }
                }
            }
        }
    }

    /* structs */
    JP sjp = jp;
    if (jp_find_array(&sjp, "structs")) {
        out->struct_count = jp_array_count(sjp);
        if (out->struct_count > 0) {
            out->structs = calloc(out->struct_count, sizeof(PkgStruct));
            for (int i = 0; i < out->struct_count; i++) {
                JP item = jp_find_array_item(sjp, "structs", i);
                out->structs[i].name = json_get_string(&item, "name");

                JP fi;
                fi = item;
                if (jp_find_array(&fi, "fields")) {
                    out->structs[i].field_count = jp_array_count(fi);
                    if (out->structs[i].field_count > 0) {
                        out->structs[i].fields = calloc(out->structs[i].field_count, sizeof(PkgField));
                        for (int j = 0; j < out->structs[i].field_count; j++) {
                            JP fic = jp_find_array_item(fi, "fields", j);
                            out->structs[i].fields[j].name = json_get_string(&fic, "name");
                            out->structs[i].fields[j].type_name = json_get_string(&fic, "type");
                            out->structs[i].fields[j].is_ptr = json_get_int(&fic, "is_ptr");
                            out->structs[i].fields[j].array_size = json_get_int(&fic, "array_size");
                        }
                    }
                }

                fi = item;
                if (jp_find_array(&fi, "generic_params")) {
                    out->structs[i].generic_param_count = jp_array_count(fi);
                    if (out->structs[i].generic_param_count > 0) {
                        out->structs[i].generic_params = calloc(out->structs[i].generic_param_count, sizeof(PkgGenericParam));
                        for (int j = 0; j < out->structs[i].generic_param_count; j++) {
                            JP gpc = jp_find_array_item(fi, "generic_params", j);
                            out->structs[i].generic_params[j].name = json_get_string(&gpc, "name");
                        }
                    }
                }

                fi = item;
                if (jp_find_array(&fi, "methods")) {
                    out->structs[i].method_count = jp_array_count(fi);
                    if (out->structs[i].method_count > 0) {
                        out->structs[i].methods = calloc(out->structs[i].method_count, sizeof(PkgFunc));
                        for (int j = 0; j < out->structs[i].method_count; j++) {
                            JP mic = jp_find_array_item(fi, "methods", j);
                            out->structs[i].methods[j].name = json_get_string(&mic, "name");
                            out->structs[i].methods[j].return_type = json_get_string(&mic, "return_type");
                            out->structs[i].methods[j].is_extern = json_get_int(&mic, "is_extern") != 0;
                            out->structs[i].methods[j].is_static = json_get_int(&mic, "is_static") != 0;
                            out->structs[i].methods[j].is_variadic = json_get_int(&mic, "is_variadic") != 0;
                            out->structs[i].methods[j].params =
                                pkg_parse_params(mic, &out->structs[i].methods[j].param_count);

                            JP pi = mic;
                            if (jp_find_array(&pi, "generic_params")) {
                                out->structs[i].methods[j].generic_param_count = jp_array_count(pi);
                                if (out->structs[i].methods[j].generic_param_count > 0) {
                                    out->structs[i].methods[j].generic_params =
                                        calloc(out->structs[i].methods[j].generic_param_count, sizeof(PkgGenericParam));
                                    for (int k = 0; k < out->structs[i].methods[j].generic_param_count; k++) {
                                        JP gpc = jp_find_array_item(pi, "generic_params", k);
                                        out->structs[i].methods[j].generic_params[k].name =
                                            json_get_string(&gpc, "name");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    JP gjp = jp;
    if (jp_find_array(&gjp, "globals")) {
        out->global_count = jp_array_count(gjp);
        if (out->global_count > 0) {
            out->globals = calloc(out->global_count, sizeof(PkgGlobal));
            for (int i = 0; i < out->global_count; i++) {
                JP item = jp_find_array_item(gjp, "globals", i);
                out->globals[i].name = json_get_string(&item, "name");
                out->globals[i].type_name = json_get_string(&item, "type");
                out->globals[i].is_ptr = json_get_int(&item, "is_ptr");
                out->globals[i].is_extern = json_get_int(&item, "is_extern") != 0;
                out->globals[i].array_size = json_get_int(&item, "array_size");
            }
        }
    }

    /* functions */
    JP fjp = jp;
    if (jp_find_array(&fjp, "functions")) {
        out->func_count = jp_array_count(fjp);
        if (out->func_count > 0) {
            out->funcs = calloc(out->func_count, sizeof(PkgFunc));
            for (int i = 0; i < out->func_count; i++) {
                JP item = jp_find_array_item(fjp, "functions", i);
                out->funcs[i].name = json_get_string(&item, "name");
                out->funcs[i].return_type = json_get_string(&item, "return_type");
                out->funcs[i].is_extern = json_get_int(&item, "is_extern") != 0;
                out->funcs[i].is_static = json_get_int(&item, "is_static") != 0;
                out->funcs[i].is_variadic = json_get_int(&item, "is_variadic") != 0;

                out->funcs[i].params =
                    pkg_parse_params(item, &out->funcs[i].param_count);

                JP pi = item;
                if (jp_find_array(&pi, "generic_params")) {
                    out->funcs[i].generic_param_count = jp_array_count(pi);
                    if (out->funcs[i].generic_param_count > 0) {
                        out->funcs[i].generic_params = calloc(out->funcs[i].generic_param_count, sizeof(PkgGenericParam));
                        for (int j = 0; j < out->funcs[i].generic_param_count; j++) {
                            JP gpc = jp_find_array_item(pi, "generic_params", j);
                            out->funcs[i].generic_params[j].name = json_get_string(&gpc, "name");
                        }
                    }
                }
            }
        }
    }

    return true;
}

static bool make_temp_obj_dir(char *out, size_t out_size) {
    const char *tmp = getenv("TMPDIR");
    if (!tmp || !*tmp) tmp = "/tmp";
    snprintf(out, out_size, "%s/abyssc_pkg_XXXXXX", tmp);
    return mkdtemp(out) != NULL;
}

void pkg_metadata_free(PkgMetadata *meta) {
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
        for (int j = 0; j < meta->structs[i].generic_param_count; j++)
            free(meta->structs[i].generic_params[j].name);
        free(meta->structs[i].generic_params);
        for (int j = 0; j < meta->structs[i].method_count; j++) {
            free(meta->structs[i].methods[j].name);
            free(meta->structs[i].methods[j].return_type);
            for (int k = 0; k < meta->structs[i].methods[j].param_count; k++) {
                free(meta->structs[i].methods[j].params[k].name);
                free(meta->structs[i].methods[j].params[k].type_name);
            }
            free(meta->structs[i].methods[j].params);
            for (int k = 0; k < meta->structs[i].methods[j].generic_param_count; k++)
                free(meta->structs[i].methods[j].generic_params[k].name);
            free(meta->structs[i].methods[j].generic_params);
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
        for (int j = 0; j < meta->funcs[i].generic_param_count; j++)
            free(meta->funcs[i].generic_params[j].name);
        free(meta->funcs[i].generic_params);
    }
    free(meta->funcs);
    memset(meta, 0, sizeof(*meta));
}

/* ── Library read/write ─────────────────────────────────────── */

bool pkg_write_library(const char *path, const PkgMetadata *meta,
                       const void *lib_data, size_t lib_size) {
    char *json = pkg_metadata_to_json(meta);
    uint32_t json_len = (uint32_t)strlen(json);

    FILE *f = fopen(path, "wb");
    if (!f) { free(json); return false; }

    fwrite(MAGIC, 1, MAGIC_LEN, f);
    fwrite(&json_len, 4, 1, f);
    fwrite(json, 1, json_len, f);
    if (lib_data && lib_size > 0)
        fwrite(lib_data, 1, lib_size, f);
    fclose(f);
    free(json);
    return true;
}

bool pkg_read_library(const char *path, char **json_out, size_t *json_size,
                      void **lib_out, size_t *lib_size) {
    size_t file_size;
    char *data = read_file_raw(path, &file_size);
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

    size_t lib_off = HEADER_SIZE + meta_len;
    *lib_size = file_size - lib_off;
    if (*lib_size > 0) {
        *lib_out = malloc(*lib_size);
        memcpy(*lib_out, data + lib_off, *lib_size);
    } else {
        *lib_out = NULL;
    }

    free(data);
    return true;
}

/* ── Package directory ──────────────────────────────────────── */

const char *pkg_dir(void) {
    static char buf[512];
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(buf, sizeof(buf), "%s/.dwellerpkg/packages", home);
    return buf;
}

static bool ensure_pkg_dir(const char *name) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", pkg_dir(), name);
    return ensure_path(path);
}

/* ── Build ──────────────────────────────────────────────────── */

static void pkg_copy_params(FuncParamList *src, PkgParam **out_params, int *out_count) {
    int count = 0;
    for (FuncParamList *pl = src; pl; pl = pl->next) count++;
    *out_count = count;
    if (count <= 0) return;

    *out_params = calloc(count, sizeof(PkgParam));
    int i = 0;
    for (FuncParamList *pl = src; pl; pl = pl->next, i++) {
        (*out_params)[i].name = strdup(pl->param.name);
        (*out_params)[i].type_name = strdup(pl->param.type_name);
        (*out_params)[i].is_ptr = pl->param.is_ptr;
    }
}

static void pkg_copy_generic_params(GenericParamList *src,
                                    PkgGenericParam **out_params,
                                    int *out_count) {
    int count = 0;
    for (GenericParamList *gl = src; gl; gl = gl->next) count++;
    *out_count = count;
    if (count <= 0) return;

    *out_params = calloc(count, sizeof(PkgGenericParam));
    int i = 0;
    for (GenericParamList *gl = src; gl; gl = gl->next, i++)
        (*out_params)[i].name = strdup(gl->param.name);
}

static void pkg_metadata_add_alias(PkgMetadata *meta, TypeAliasDecl *alias) {
    meta->alias_count++;
    meta->aliases = realloc(meta->aliases, sizeof(PkgTypeAlias) * meta->alias_count);
    PkgTypeAlias *pa = &meta->aliases[meta->alias_count - 1];
    memset(pa, 0, sizeof(*pa));
    pa->name = strdup(alias->name);
    pa->target_type = strdup(alias->target_type);
}

static void pkg_metadata_add_enum(PkgMetadata *meta, EnumDecl *en) {
    meta->enum_count++;
    meta->enums = realloc(meta->enums, sizeof(PkgEnum) * meta->enum_count);
    PkgEnum *pe = &meta->enums[meta->enum_count - 1];
    memset(pe, 0, sizeof(*pe));
    pe->name = strdup(en->name);

    for (EnumVariantList *v = en->variants; v; v = v->next) pe->variant_count++;
    if (pe->variant_count > 0) {
        pe->variants = calloc(pe->variant_count, sizeof(PkgEnumVariant));
        int i = 0;
        for (EnumVariantList *v = en->variants; v; v = v->next, i++) {
            pe->variants[i].name = strdup(v->name);
            pe->variants[i].value = v->value;
        }
    }
}

static void pkg_metadata_add_interface(PkgMetadata *meta, InterfaceDecl *iface) {
    meta->interface_count++;
    meta->interfaces = realloc(meta->interfaces, sizeof(PkgInterface) * meta->interface_count);
    PkgInterface *pi = &meta->interfaces[meta->interface_count - 1];
    memset(pi, 0, sizeof(*pi));
    pi->name = strdup(iface->name);

    for (InterfaceMethodList *ml = iface->methods; ml; ml = ml->next) pi->method_count++;
    if (pi->method_count > 0) {
        pi->methods = calloc(pi->method_count, sizeof(PkgInterfaceMethod));
        int i = 0;
        for (InterfaceMethodList *ml = iface->methods; ml; ml = ml->next, i++) {
            pi->methods[i].name = strdup(ml->method.name);
            pi->methods[i].return_type = strdup(ml->method.return_type);
            pi->methods[i].is_variadic = ml->method.is_variadic;
            pkg_copy_params(ml->method.params, &pi->methods[i].params,
                            &pi->methods[i].param_count);
        }
    }
}

static void pkg_metadata_add_struct(PkgMetadata *meta, StructDecl *sd,
                                    bool include_generics) {
    meta->struct_count++;
    meta->structs = realloc(meta->structs, sizeof(PkgStruct) * meta->struct_count);
    PkgStruct *ps = &meta->structs[meta->struct_count - 1];
    memset(ps, 0, sizeof(*ps));
    ps->name = strdup(sd->name);

    for (StructFieldList *fl = sd->fields; fl; fl = fl->next) ps->field_count++;
    if (ps->field_count > 0) {
        ps->fields = calloc(ps->field_count, sizeof(PkgField));
        int i = 0;
        for (StructFieldList *fl = sd->fields; fl; fl = fl->next, i++) {
            ps->fields[i].name = strdup(fl->field.name);
            ps->fields[i].type_name = strdup(fl->field.type_name);
            ps->fields[i].is_ptr = fl->field.is_ptr;
            ps->fields[i].array_size = fl->field.array_size;
        }
    }

    if (include_generics)
        pkg_copy_generic_params(sd->generic_params, &ps->generic_params,
                                &ps->generic_param_count);

    for (DeclList *ml = sd->methods; ml; ml = ml->next) {
        if (!ml->decl || ml->decl->type != DECL_FUNC)
            continue;
        FuncDecl *method = &ml->decl->data.func;
        ps->method_count++;
        ps->methods = realloc(ps->methods, sizeof(PkgFunc) * ps->method_count);
        PkgFunc *pm = &ps->methods[ps->method_count - 1];
        memset(pm, 0, sizeof(*pm));
        pm->name = strdup(method->name);
        pm->return_type = strdup(method->return_type);
        pm->is_extern = (method->body == NULL);
        pm->is_static = method->is_static;
        pm->is_variadic = method->is_variadic;
        pkg_copy_params(method->params, &pm->params, &pm->param_count);
        if (include_generics)
            pkg_copy_generic_params(method->generic_params, &pm->generic_params,
                                    &pm->generic_param_count);
    }
}

static void pkg_metadata_add_global(PkgMetadata *meta, GlobalVarDecl *g) {
    if (g->is_static) return;
    meta->global_count++;
    meta->globals = realloc(meta->globals, sizeof(PkgGlobal) * meta->global_count);
    PkgGlobal *pg = &meta->globals[meta->global_count - 1];
    memset(pg, 0, sizeof(*pg));
    pg->name = strdup(g->name);
    pg->type_name = strdup(g->type_name);
    pg->is_ptr = g->is_ptr;
    pg->is_extern = g->is_extern;
    pg->array_size = g->array_size;
}

static void pkg_metadata_add_func(PkgMetadata *meta, FuncDecl *fd,
                                  bool include_generics) {
    if (fd->is_static) return;
    meta->func_count++;
    meta->funcs = realloc(meta->funcs, sizeof(PkgFunc) * meta->func_count);
    PkgFunc *pf = &meta->funcs[meta->func_count - 1];
    memset(pf, 0, sizeof(*pf));
    pf->name = strdup(fd->name);
    pf->return_type = strdup(fd->return_type);
    pf->is_extern = (fd->body == NULL);
    pf->is_static = fd->is_static;
    pf->is_variadic = fd->is_variadic;
    pkg_copy_params(fd->params, &pf->params, &pf->param_count);
    if (include_generics)
        pkg_copy_generic_params(fd->generic_params, &pf->generic_params,
                                &pf->generic_param_count);
}

static void pkg_metadata_add_decl(PkgMetadata *meta, Decl *d, bool include_generics) {
    if (!d) return;
    switch (d->type) {
    case DECL_TYPE_ALIAS:
        pkg_metadata_add_alias(meta, &d->data.type_alias);
        break;
    case DECL_ENUM:
        pkg_metadata_add_enum(meta, &d->data.enum_decl);
        break;
    case DECL_INTERFACE:
        pkg_metadata_add_interface(meta, &d->data.interface);
        break;
    case DECL_STRUCT:
        pkg_metadata_add_struct(meta, &d->data.struct_decl, include_generics);
        break;
    case DECL_GLOBAL_VAR:
        pkg_metadata_add_global(meta, &d->data.global_var);
        break;
    case DECL_FUNC:
        pkg_metadata_add_func(meta, &d->data.func, include_generics);
        break;
    default:
        break;
    }
}

bool pkg_build_static(const char **source_files, int source_count,
                      const char *output_path, const char *package_name,
                      const char *version) {
    char obj_dir[512];
    if (!make_temp_obj_dir(obj_dir, sizeof(obj_dir))) {
        fprintf(stderr, "dwellerpkg: cannot create temporary object directory\n");
        return false;
    }

    char *obj_files[256];
    int obj_count = 0;
    bool ok = true;

    for (int i = 0; i < source_count && ok; i++) {
        char obj_path[1024];
        char *base = strdup(source_files[i]);
        char *dot = strrchr(base, '.');
        if (dot) *dot = '\0';
        char *slash = strrchr(base, '/');
        const char *fname = slash ? slash + 1 : base;
        snprintf(obj_path, sizeof(obj_path), "%s/%s.o", obj_dir, fname);

        char cmd[2048];
        snprintf(cmd, sizeof(cmd),
            "abyssc --package-prefix \"%s\" -c -o \"%s\" \"%s\"",
            package_name, obj_path, source_files[i]);
        if (system(cmd) != 0) {
            fprintf(stderr, "dwellerpkg: failed to compile %s\n", source_files[i]);
            ok = false;
        } else {
            obj_files[obj_count++] = strdup(obj_path);
        }
        free(base);
    }

    if (ok && obj_count > 0) {
        /* Create the .a archive first */
        remove(output_path);
        char ar_cmd[4096];
        int n = snprintf(ar_cmd, sizeof(ar_cmd), "ar rcs \"%s\"", output_path);
        for (int i = 0; i < obj_count; i++)
            n += snprintf(ar_cmd + n, sizeof(ar_cmd) - n, " \"%s\"", obj_files[i]);

        if (system(ar_cmd) != 0) {
            fprintf(stderr, "dwellerpkg: ar failed\n");
            ok = false;
        }
    }

    /* Build metadata from source files */
    if (ok) {
        PkgMetadata meta = {0};
        meta.name = strdup(package_name);
        meta.version = strdup(version ? version : "0.0.0");

        /* Read each source file to extract declarations */
        for (int i = 0; i < source_count; i++) {
            char *source = read_file_raw(source_files[i], &(size_t){0});
            if (!source) continue;

            Lexer *lexer = lexer_new(source_files[i], source);
            Tokens *tokens = tokens_vec_new();
            while (lexer_has_next(lexer))
                tokens_vec_append(tokens, lexer_next_token(lexer));

            Parser *parser = parser_new(tokens);
            Program *prog = parse_program(parser, source_files[i]);

            for (DeclList *dl = prog->decls; dl; dl = dl->next)
                pkg_metadata_add_decl(&meta, dl->decl, true);

            program_free(prog);
            parser_free(parser);
            tokens_vec_free(tokens);
            free(source);
        }

        /* Read back the .a file and prepend metadata */
        size_t ar_size;
        char *ar_data = read_file_raw(output_path, &ar_size);
        if (ar_data) {
            pkg_write_library(output_path, &meta, ar_data, ar_size);
            free(ar_data);
        } else {
            pkg_write_library(output_path, &meta, NULL, 0);
        }

        pkg_metadata_free(&meta);
    }

    for (int i = 0; i < obj_count; i++) {
        remove(obj_files[i]);
        free(obj_files[i]);
    }
    rmdir(obj_dir);

    return ok;
}

bool pkg_build_shared(const char **source_files, int source_count,
                      const char *output_path, const char *package_name,
                      const char *version) {
    char obj_dir[512];
    if (!make_temp_obj_dir(obj_dir, sizeof(obj_dir))) {
        fprintf(stderr, "dwellerpkg: cannot create temporary object directory\n");
        return false;
    }

    char *obj_files[256];
    int obj_count = 0;
    bool ok = true;

    for (int i = 0; i < source_count && ok; i++) {
        char obj_path[1024];
        char *base = strdup(source_files[i]);
        char *dot = strrchr(base, '.');
        if (dot) *dot = '\0';
        char *slash = strrchr(base, '/');
        const char *fname = slash ? slash + 1 : base;
        snprintf(obj_path, sizeof(obj_path), "%s/%s.o", obj_dir, fname);

        char cmd[2048];
        snprintf(cmd, sizeof(cmd),
            "abyssc --package-prefix \"%s\" -c -o \"%s\" \"%s\"",
            package_name, obj_path, source_files[i]);
        if (system(cmd) != 0) {
            fprintf(stderr, "dwellerpkg: failed to compile %s\n", source_files[i]);
            ok = false;
        } else {
            obj_files[obj_count++] = strdup(obj_path);
        }
        free(base);
    }

    if (ok && obj_count > 0) {
        char link_cmd[4096];
        int n = snprintf(link_cmd, sizeof(link_cmd),
            "clang -shared -o \"%s\"", output_path);
        for (int i = 0; i < obj_count; i++)
            n += snprintf(link_cmd + n, sizeof(link_cmd) - n, " \"%s\"", obj_files[i]);

        if (system(link_cmd) != 0) {
            fprintf(stderr, "dwellerpkg: clang -shared failed\n");
            ok = false;
        }
    }

    /* Build metadata same as static */
    if (ok) {
        PkgMetadata meta = {0};
        meta.name = strdup(package_name);
        meta.version = strdup(version ? version : "0.0.0");

        for (int i = 0; i < source_count; i++) {
            char *source = read_file_raw(source_files[i], &(size_t){0});
            if (!source) continue;

            Lexer *lexer = lexer_new(source_files[i], source);
            Tokens *tokens = tokens_vec_new();
            while (lexer_has_next(lexer))
                tokens_vec_append(tokens, lexer_next_token(lexer));

            Parser *parser = parser_new(tokens);
            Program *prog = parse_program(parser, source_files[i]);

            for (DeclList *dl = prog->decls; dl; dl = dl->next)
                pkg_metadata_add_decl(&meta, dl->decl, false);

            program_free(prog);
            parser_free(parser);
            tokens_vec_free(tokens);
            free(source);
        }

        size_t so_size;
        char *so_data = read_file_raw(output_path, &so_size);
        if (so_data) {
            pkg_write_library(output_path, &meta, so_data, so_size);
            free(so_data);
        } else {
            pkg_write_library(output_path, &meta, NULL, 0);
        }

        pkg_metadata_free(&meta);
    }

    for (int i = 0; i < obj_count; i++) {
        remove(obj_files[i]);
        free(obj_files[i]);
    }
    rmdir(obj_dir);

    return ok;
}

/* ── Executable build ────────────────────────────────────────── */

bool pkg_build_executable(const char **source_files, int source_count,
                          const char *output_path,
                          const char **link_libs, int link_lib_count) {
    char obj_dir[512];
    if (!make_temp_obj_dir(obj_dir, sizeof(obj_dir))) {
        fprintf(stderr, "dwellerpkg: cannot create temporary object directory\n");
        return false;
    }

    char *obj_files[256];
    int obj_count = 0;
    bool ok = true;

    for (int i = 0; i < source_count && ok; i++) {
        char obj_path[1024];
        char *base = strdup(source_files[i]);
        char *dot = strrchr(base, '.');
        if (dot) *dot = '\0';
        char *slash = strrchr(base, '/');
        const char *fname = slash ? slash + 1 : base;
        snprintf(obj_path, sizeof(obj_path), "%s/%s.o", obj_dir, fname);

        char cmd[2048];
        snprintf(cmd, sizeof(cmd),
            "abyssc -c -o \"%s\" \"%s\"", obj_path, source_files[i]);
        if (system(cmd) != 0) {
            fprintf(stderr, "dwellerpkg: failed to compile %s\n", source_files[i]);
            ok = false;
        } else {
            obj_files[obj_count++] = strdup(obj_path);
        }
        free(base);
    }

    if (ok && obj_count > 0) {
        /* Resolve library archives from installed packages.
           Extract raw library data from metadata-wrapped files. */
        char *lib_paths[256];
        int lib_count = 0;

        for (int i = 0; i < link_lib_count; i++) {
            char *pkg_path = pkg_find_library(link_libs[i]);
            if (!pkg_path) {
                fprintf(stderr, "dwellerpkg: library '%s' not found\n", link_libs[i]);
                ok = false;
                continue;
            }

            /* Extract raw library content from the dwellerpkg wrapper */
            char *json = NULL;
            size_t json_len = 0;
            void *lib_data = NULL;
            size_t lib_size = 0;
            if (pkg_read_library(pkg_path, &json, &json_len, &lib_data, &lib_size)) {
                /* Write raw .a to temp file */
                char raw_path[1024];
                snprintf(raw_path, sizeof(raw_path), "%s/%s_raw.a", obj_dir, link_libs[i]);
                FILE *rf = fopen(raw_path, "wb");
                if (rf && lib_data && lib_size > 0) {
                    fwrite(lib_data, 1, lib_size, rf);
                    fclose(rf);
                    lib_paths[lib_count++] = strdup(raw_path);
                } else {
                    fprintf(stderr, "dwellerpkg: failed to extract library '%s'\n", link_libs[i]);
                    ok = false;
                }
                free(json);
                free(lib_data);
            } else {
                fprintf(stderr, "dwellerpkg: cannot read library '%s'\n", link_libs[i]);
                ok = false;
            }
            free(pkg_path);
        }

        if (ok) {
            char link_cmd[8192];
            int n = snprintf(link_cmd, sizeof(link_cmd), "clang -o \"%s\"", output_path);
            for (int i = 0; i < obj_count; i++)
                n += snprintf(link_cmd + n, sizeof(link_cmd) - n, " \"%s\"", obj_files[i]);
            for (int i = 0; i < lib_count; i++)
                n += snprintf(link_cmd + n, sizeof(link_cmd) - n, " \"%s\"", lib_paths[i]);

            if (system(link_cmd) != 0) {
                fprintf(stderr, "dwellerpkg: link failed\n");
                ok = false;
            }
        }

        for (int i = 0; i < lib_count; i++)
            free(lib_paths[i]);
    }

    for (int i = 0; i < obj_count; i++) {
        remove(obj_files[i]);
        free(obj_files[i]);
    }
    rmdir(obj_dir);

    return ok;
}

/* ── Install ────────────────────────────────────────────────── */

bool pkg_install(const char *src_path, const char *package_name,
                 const char *version) {
    char dest_dir[1024];
    snprintf(dest_dir, sizeof(dest_dir), "%s/%s/%s", pkg_dir(), package_name, version);
    if (!ensure_path(dest_dir)) {
        fprintf(stderr, "dwellerpkg: cannot create %s\n", dest_dir);
        return false;
    }

    const char *basename = strrchr(src_path, '/');
    basename = basename ? basename + 1 : src_path;

    char dest_path[1024];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, basename);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", src_path, dest_path);
    if (system(cmd) != 0) {
        fprintf(stderr, "dwellerpkg: failed to copy %s\n", src_path);
        return false;
    }

    fprintf(stderr, "installed %s %s -> %s\n", package_name, version, dest_path);
    return true;
}

/* ── List ───────────────────────────────────────────────────── */

int pkg_list_installed(char ***names_out, char ***versions_out) {
    const char *base = pkg_dir();
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "find \"%s\" -mindepth 2 -maxdepth 2 -type d 2>/dev/null", base);

    FILE *p = popen(cmd, "r");
    if (!p) return 0;

    int cap = 16, count = 0;
    *names_out = malloc(sizeof(char*) * cap);
    *versions_out = malloc(sizeof(char*) * cap);

    char line[512];
    while (fgets(line, sizeof(line), p)) {
        line[strcspn(line, "\n")] = '\0';
        /* line is like /home/user/.dwellerpkg/packages/math/1.0.0 */
        char *pkg = strrchr(line, '/');
        if (!pkg) continue;
        *pkg = '\0';
        char *ver = pkg + 1;
        pkg = strrchr(line, '/');
        if (!pkg) continue;
        pkg++;

        if (count >= cap) {
            cap *= 2;
            *names_out = realloc(*names_out, sizeof(char*) * cap);
            *versions_out = realloc(*versions_out, sizeof(char*) * cap);
        }
        (*names_out)[count] = strdup(pkg);
        (*versions_out)[count] = strdup(ver);
        count++;
    }

    pclose(p);
    return count;
}

/* ── Find ───────────────────────────────────────────────────── */

char *pkg_find_library(const char *package_name) {
    /* Search for latest version with .abyss_a or .abyss_so */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "find \"%s/%s\" -name '*.abyss_a' -o -name '*.abyss_so' 2>/dev/null | sort -V | tail -1",
        pkg_dir(), package_name);

    FILE *p = popen(cmd, "r");
    if (!p) return NULL;

    char line[1024];
    char *result = NULL;
    if (fgets(line, sizeof(line), p)) {
        line[strcspn(line, "\n")] = '\0';
        result = strdup(line);
    }
    pclose(p);
    return result;
}

/* ── Link flags ─────────────────────────────────────────────── */

bool pkg_print_link_flags(const char *package_name, FILE *out) {
    char *lib = pkg_find_library(package_name);
    if (!lib) return false;

    char *dup = strdup(lib);
    char *dir = strdup(dup);
    char *slash = strrchr(dir, '/');
    if (slash) *slash = '\0';

    bool is_static = strstr(dup, ".abyss_a") != NULL;
    bool is_shared = strstr(dup, ".abyss_so") != NULL;

    if (is_static || is_shared) {
        void *lib_data = NULL;
        size_t lib_size = 0;
        char *json = NULL;
        size_t json_len = 0;

        if (pkg_read_library(lib, &json, &json_len, &lib_data, &lib_size)) {
            const char *ext = is_static ? ".raw.a" : ".raw.so";
            char raw_path[1024];
            snprintf(raw_path, sizeof(raw_path), "%s/%s", dir, ext);

            FILE *f = fopen(raw_path, "wb");
            if (f && lib_data && lib_size > 0) {
                fwrite(lib_data, 1, lib_size, f);
                fclose(f);
                fprintf(out, "\"%s\" ", raw_path);
            } else {
                fprintf(out, "\"%s\" ", lib);
            }
            free(lib_data);
            free(json);
        } else {
            fprintf(out, "\"%s\" ", lib);
        }
    } else {
        fprintf(out, "\"%s\" ", lib);
    }

    free(dir);
    free(dup);
    free(lib);
    return true;
}

/* ── Compiler integration: inject import declarations ───────── */

static FuncParamList *pkg_build_func_params(PkgParam *params, int count) {
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

bool pkg_inject_import(const char *package_name, Program *program) {
    char *lib = pkg_find_library(package_name);
    if (!lib) {
        fprintf(stderr, "dwellerpkg: package '%s' not found\n", package_name);
        return false;
    }

    char *json = NULL;
    size_t json_len = 0;
    void *lib_data = NULL;
    size_t lib_size = 0;

    if (!pkg_read_library(lib, &json, &json_len, &lib_data, &lib_size)) {
        fprintf(stderr, "dwellerpkg: cannot read metadata from '%s'\n", lib);
        free(lib);
        return false;
    }

    PkgMetadata meta;
    if (!pkg_metadata_from_json(json, json_len, &meta)) {
        fprintf(stderr, "dwellerpkg: cannot parse metadata from '%s'\n", lib);
        free(json); free(lib_data); free(lib);
        return false;
    }

    SourceLocation loc = {program->filename, 0, 0};

    for (int i = 0; i < meta.alias_count; i++) {
        PkgTypeAlias *pa = &meta.aliases[i];
        Decl *decl = decl_new_type_alias(
            pa->name ? pa->name : "",
            pa->target_type ? pa->target_type : "i32",
            loc);
        program_add_decl(program, decl);
    }

    for (int i = 0; i < meta.enum_count; i++) {
        PkgEnum *pe = &meta.enums[i];
        EnumVariantList *variants = NULL;
        for (int j = 0; j < pe->variant_count; j++) {
            EnumVariantList *variant = enum_variant_list_new(
                pe->variants[j].name ? pe->variants[j].name : "",
                pe->variants[j].value);
            enum_variant_list_append(&variants, variant);
        }

        Decl *decl = decl_new_enum(pe->name ? pe->name : "", variants, loc);
        program_add_decl(program, decl);
    }

    for (int i = 0; i < meta.interface_count; i++) {
        PkgInterface *pi = &meta.interfaces[i];
        InterfaceMethodList *methods = NULL;
        for (int j = 0; j < pi->method_count; j++) {
            PkgInterfaceMethod *pm = &pi->methods[j];
            InterfaceMethod method = interface_method_new(
                pm->return_type ? pm->return_type : "void",
                pm->name ? pm->name : "",
                pkg_build_func_params(pm->params, pm->param_count),
                pm->is_variadic);
            interface_method_list_append(&methods, method);
        }

        Decl *decl = decl_new_interface(pi->name ? pi->name : "", methods, NULL, loc);
        program_add_decl(program, decl);
    }

    /* Inject struct declarations */
    for (int i = 0; i < meta.struct_count; i++) {
        PkgStruct *ps = &meta.structs[i];

        StructFieldList *fields = NULL;
        for (int j = 0; j < ps->field_count; j++) {
            StructField sf = struct_field_new(
                strdup(ps->fields[j].type_name),
                strdup(ps->fields[j].name),
                ps->fields[j].is_ptr,
                ps->fields[j].array_size);
            struct_field_list_append(&fields, sf);
        }

        GenericParamList *gparams = NULL;
        for (int j = 0; j < ps->generic_param_count; j++)
            generic_param_list_append(&gparams, generic_param_new(strdup(ps->generic_params[j].name)));

        DeclList *methods = NULL;
        for (int j = 0; j < ps->method_count; j++) {
            PkgFunc *pm = &ps->methods[j];
            FuncParamList *params = pkg_build_func_params(pm->params, pm->param_count);

            GenericParamList *method_gparams = NULL;
            for (int k = 0; k < pm->generic_param_count; k++)
                generic_param_list_append(&method_gparams,
                                          generic_param_new(pm->generic_params[k].name ?
                                                            pm->generic_params[k].name : ""));

            Decl *method = decl_new_func(
                pm->return_type ? pm->return_type : "void",
                pm->name ? pm->name : "",
                params, method_gparams,
                NULL, NULL, pm->is_static, pm->is_extern, pm->is_variadic, loc);
            decl_list_append(&methods, method);
        }

        Decl *decl = decl_new_struct(
            strdup(ps->name), fields, gparams, methods, NULL, loc);
        program_add_decl(program, decl);
    }

    for (int i = 0; i < meta.global_count; i++) {
        PkgGlobal *pg = &meta.globals[i];
        Decl *decl = decl_new_global_var(
            pg->type_name ? pg->type_name : "i32",
            pg->name ? pg->name : "",
            NULL,
            pg->is_ptr,
            0,
            1,
            pg->array_size,
            loc);
        program_add_decl(program, decl);
    }

    /* Inject function declarations (extern, no body) */
    for (int i = 0; i < meta.func_count; i++) {
        PkgFunc *pf = &meta.funcs[i];

        FuncParamList *params = pkg_build_func_params(pf->params, pf->param_count);

        GenericParamList *gparams = NULL;
        for (int j = 0; j < pf->generic_param_count; j++)
            generic_param_list_append(&gparams, generic_param_new(strdup(pf->generic_params[j].name)));

        Decl *decl = decl_new_func(
            pf->return_type ? pf->return_type : "void",
            pf->name ? pf->name : "",
            params, gparams,
            NULL, NULL, pf->is_static, pf->is_extern, pf->is_variadic, loc);
        program_add_decl(program, decl);
    }

    pkg_metadata_free(&meta);
    free(json);
    free(lib_data);
    free(lib);
    return true;
}

bool pkg_collect_link_flags(const char **imported_packages, int count, FILE *out) {
    for (int i = 0; i < count; i++) {
        pkg_print_link_flags(imported_packages[i], out);
    }
    return true;
}
