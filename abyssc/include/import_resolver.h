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
    char *target_type;
} IRTypeAlias;

typedef struct {
    char *name;
    int value;
} IREnumVariant;

typedef struct {
    char *name;
    IREnumVariant *variants;
    int variant_count;
} IREnum;

typedef struct {
    char *name;
    char *type_name;
    int is_ptr;
    int array_size;
} IRField;

typedef struct {
    char *name;
    char *return_type;
    IRParam *params;
    int param_count;
    int is_extern;
    int is_static;
    int is_variadic;
} IRFunc;

typedef struct {
    char *name;
    IRField *fields;
    int field_count;
    IRFunc *methods;
    int method_count;
} IRStruct;

typedef struct {
    char *return_type;
    char *name;
    IRParam *params;
    int param_count;
    int is_variadic;
} IRInterfaceMethod;

typedef struct {
    char *name;
    IRInterfaceMethod *methods;
    int method_count;
} IRInterface;

typedef struct {
    char *name;
    char *type_name;
    int is_ptr;
    int is_extern;
    int array_size;
} IRGlobal;

typedef struct {
    char *name;
    char *version;
    IRTypeAlias *aliases;
    int alias_count;
    IREnum *enums;
    int enum_count;
    IRInterface *interfaces;
    int interface_count;
    IRStruct *structs;
    int struct_count;
    IRGlobal *globals;
    int global_count;
    IRFunc *funcs;
    int func_count;
} IRMetadata;

bool ir_find_library(const char *package_name, char **out_path);
bool ir_read_library_metadata(const char *path, IRMetadata *out_meta);
bool ir_inject_import(const char *package_name, Program *program, Decl *after_decl, char **out_lib_path);
bool ir_extract_library_object(const char *lib_path, const char *dest_path);
void ir_metadata_free(IRMetadata *meta);

#endif
