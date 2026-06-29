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
                    out_meta->funcs[i].is_variadic = ir_json_get_int(&item, "is_variadic") != 0;

                    IRJP pi = item;
                    IRJP params_arr = ir_jp_find(&pi, "params");
                    ir_jp_skip(&params_arr);
                    if (params_arr.p < params_arr.end && *params_arr.p == '[') {
                        out_meta->funcs[i].param_count = ir_jp_array_count(params_arr);
                        if (out_meta->funcs[i].param_count > 0) {
                            out_meta->funcs[i].params = calloc(out_meta->funcs[i].param_count, sizeof(IRParam));
                            for (int j = 0; j < out_meta->funcs[i].param_count; j++) {
                                IRJP pic = ir_jp_array_item(params_arr, j);
                                out_meta->funcs[i].params[j].name = ir_json_get_string(&pic, "name");
                                out_meta->funcs[i].params[j].type_name = ir_json_get_string(&pic, "type");
                                out_meta->funcs[i].params[j].is_ptr = ir_json_get_int(&pic, "is_ptr");
                            }
                        }
                    }
                }
            }
        }
    }

    free(json);
    return true;
}

/* ── Inject import declarations into program ────────────────── */

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

    for (int i = 0; i < meta.func_count; i++) {
        IRFunc *pf = &meta.funcs[i];

        FuncParamList *params = NULL;
        for (int j = 0; j < pf->param_count; j++) {
            FuncParam fp = func_param_new(
                strdup(pf->params[j].type_name),
                strdup(pf->params[j].name),
                pf->params[j].is_ptr);
            func_param_list_append(&params, fp);
        }

        Decl *decl = decl_new_func(
            strdup(pf->return_type),
            strdup(pf->name),
            params, NULL,
            NULL, NULL, pf->is_extern, pf->is_variadic, loc);
        program_insert_decl_after(program, after_decl, decl);
        after_decl = decl;
    }

    if (out_lib_path) *out_lib_path = lib;
    else free(lib);

    ir_metadata_free(&meta);
    return true;
}

void ir_metadata_free(IRMetadata *meta) {
    free(meta->name);
    free(meta->version);
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
