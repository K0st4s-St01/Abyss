#ifndef IMPORT_RESOLVER_H
#define IMPORT_RESOLVER_H

#include "ast/ast.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *name;
    char *type_name;
    int is_ptr;
} IRParam;

typedef struct {
    char *name;
    char *return_type;
    IRParam *params;
    int param_count;
    int is_extern;
    int is_variadic;
} IRFunc;

typedef struct {
    char *name;
    char *version;
    IRFunc *funcs;
    int func_count;
} IRMetadata;

bool ir_find_library(const char *package_name, char **out_path);
bool ir_read_library_metadata(const char *path, IRMetadata *out_meta);
bool ir_inject_import(const char *package_name, Program *program, Decl *after_decl, char **out_lib_path);
bool ir_extract_library_object(const char *lib_path, const char *dest_path);
void ir_metadata_free(IRMetadata *meta);

#endif
