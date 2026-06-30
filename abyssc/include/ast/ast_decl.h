#ifndef AST_DECL_H
#define AST_DECL_H

#include "ast_stmt.h"

typedef enum {
    DECL_FUNC,
    DECL_STRUCT,
    DECL_STRUCT_FIELD,
    DECL_GLOBAL_VAR,
    DECL_TYPE_ALIAS,
    DECL_ENUM,
    DECL_INTERFACE,
    DECL_INTERFACE_METHOD,
    DECL_IMPORT,
} DeclType;

typedef struct {
    char *name;
    ExprList *args;
} Decorator;

typedef struct DecoratorList {
    Decorator dec;
    struct DecoratorList *next;
} DecoratorList;

typedef struct {
    char *type_name;
    char *name;
    int is_ptr;
} FuncParam;

typedef struct FuncParamList {
    FuncParam param;
    struct FuncParamList *next;
} FuncParamList;

typedef struct {
    char *name;
} GenericParam;

typedef struct GenericParamList {
    GenericParam param;
    struct GenericParamList *next;
} GenericParamList;

typedef struct {
    char *return_type;
    char *name;
    FuncParamList *params;
    GenericParamList *generic_params;
    Stmt *body;
    DecoratorList *decorators;
    int is_static;
    int is_extern;
    int is_variadic;
} FuncDecl;

typedef struct {
    char *type_name;
    char *name;
    Expr *init;
    int is_ptr;
    int is_static;
    int is_extern;
    int array_size;
} GlobalVarDecl;

typedef struct {
    char *type_name;
    char *name;
    int is_ptr;
    int array_size;
} StructField;

typedef struct StructFieldList {
    StructField field;
    struct StructFieldList *next;
} StructFieldList;

typedef struct DeclList DeclList;

typedef struct {
    char *name;
    StructFieldList *fields;
    GenericParamList *generic_params;
    DeclList *methods;
    DecoratorList *decorators;
} StructDecl;

typedef struct {
    char *name;
    char *target_type;
} TypeAliasDecl;

typedef struct EnumVariantList {
    char *name;
    int value;
    struct EnumVariantList *next;
} EnumVariantList;

typedef struct {
    char *name;
    EnumVariantList *variants;
} EnumDecl;

typedef struct {
    char *return_type;
    char *name;
    FuncParamList *params;
} InterfaceMethod;

typedef struct InterfaceMethodList {
    InterfaceMethod method;
    struct InterfaceMethodList *next;
} InterfaceMethodList;

typedef struct {
    char *name;
    InterfaceMethodList *methods;
    GenericParamList *generic_params;
} InterfaceDecl;

typedef struct {
    char *module_name;
} ImportDecl;

typedef struct Decl Decl;

struct Decl {
    DeclType type;
    SourceLocation loc;
    union {
        FuncDecl func;
        GlobalVarDecl global_var;
        StructDecl struct_decl;
        TypeAliasDecl type_alias;
        EnumDecl enum_decl;
        InterfaceDecl interface;
        ImportDecl import;
    } data;
};

typedef struct DeclList {
    Decl *decl;
    struct DeclList *next;
} DeclList;

typedef struct {
    char *filename;
    DeclList *decls;
} Program;

typedef struct {
    char *type_name;
    char *name;
    int is_ptr;
} InterfaceMethodParam;

Decorator *decorator_new(char *name, ExprList *args);
void decorator_free(Decorator *dec);
DecoratorList *decorator_list_new(void);
void decorator_list_append(DecoratorList **list, Decorator *dec);
void decorator_list_free(DecoratorList *list);

FuncParam func_param_new(char *type_name, char *name, int is_ptr);
void func_param_free(FuncParam *param);
FuncParamList *func_param_list_new(void);
void func_param_list_append(FuncParamList **list, FuncParam param);
void func_param_list_free(FuncParamList *list);

StructField struct_field_new(char *type_name, char *name, int is_ptr, int array_size);
StructFieldList *struct_field_list_new(void);
void struct_field_list_append(StructFieldList **list, StructField field);
void struct_field_list_free(StructFieldList *list);

InterfaceMethod interface_method_new(char *return_type, char *name, FuncParamList *params);
InterfaceMethodList *interface_method_list_new(void);
void interface_method_list_append(InterfaceMethodList **list, InterfaceMethod method);
void interface_method_list_free(InterfaceMethodList *list);

GenericParam generic_param_new(char *name);
void generic_param_free(GenericParam *param);
GenericParamList *generic_param_list_new(void);
void generic_param_list_append(GenericParamList **list, GenericParam param);
void generic_param_list_free(GenericParamList *list);

Decl *decl_new_func(char *return_type, char *name, FuncParamList *params, GenericParamList *generic_params, Stmt *body, DecoratorList *decorators, int is_static, int is_extern, int is_variadic, SourceLocation loc);
Decl *decl_new_global_var(char *type_name, char *name, Expr *init, int is_ptr, int is_static, int is_extern, int array_size, SourceLocation loc);
Decl *decl_new_struct(char *name, StructFieldList *fields, GenericParamList *generic_params, DeclList *methods, DecoratorList *decorators, SourceLocation loc);
Decl *decl_new_type_alias(char *name, char *target_type, SourceLocation loc);
Decl *decl_new_enum(char *name, EnumVariantList *variants, SourceLocation loc);
Decl *decl_new_interface(char *name, InterfaceMethodList *methods, GenericParamList *generic_params, SourceLocation loc);
Decl *decl_new_import(char *module_name, SourceLocation loc);
void decl_free(Decl *decl);

EnumVariantList *enum_variant_list_new(char *name, int value);
void enum_variant_list_append(EnumVariantList **list, EnumVariantList *variant);
void enum_variant_list_free(EnumVariantList *list);

DeclList *decl_list_new(void);
void decl_list_append(DeclList **list, Decl *decl);
void decl_list_free(DeclList *list);

Program *program_new(char *filename);
void program_add_decl(Program *program, Decl *decl);
void program_insert_decl_after(Program *program, Decl *after, Decl *decl);
void program_free(Program *program);

#endif
