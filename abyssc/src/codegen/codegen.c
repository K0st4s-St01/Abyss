#include "../../include/codegen/codegen.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Internal context ──────────────────────────────────────── */

typedef struct {
    char *name;
    LLVMTypeRef type;
} NamedType;

typedef struct {
    char *name;
    LLVMValueRef value;
    LLVMTypeRef type;
} ConstInfo;

typedef struct {
    char *struct_name;
    char **field_names;
    LLVMTypeRef *field_element_types;
    int field_count;
} StructFieldInfo;

typedef struct {
    char *name;
    LLVMTypeRef pointee_type;
    LLVMTypeRef element_type;
    char *source_type;
    int is_ptr;
    LLVMValueRef alloca;
} VarInfo;

typedef struct {
    LLVMValueRef val;
    LLVMTypeRef type;
    LLVMTypeRef pointee_type;
    const char *source_type;
    int is_ptr;
} TypedValue;

typedef struct {
    char *name;
    char *source_type;
} GlobalTypeInfo;

typedef struct {
    char *name;
    InterfaceMethodList *methods;
    LLVMTypeRef type;
} InterfaceTypeInfo;

typedef struct {
    char *name;
    FuncParamList *params;
    int hidden_self;
} FunctionInfo;

struct CodegenCtx {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;

    NamedType *named_types;
    int named_type_count;
    int named_type_cap;

    ConstInfo *consts;
    int const_count;
    int const_cap;

    StructFieldInfo *struct_fields;
    int struct_field_count;
    int struct_field_cap;

    VarInfo *var_infos;
    int var_info_count;
    int var_info_cap;

    GlobalTypeInfo *global_types;
    int global_type_count;
    int global_type_cap;

    InterfaceTypeInfo *interfaces;
    int interface_count;
    int interface_cap;

    FunctionInfo *functions;
    int function_count;
    int function_cap;

    char *package_prefix;
    LLVMValueRef current_func;
    LLVMTypeRef current_func_return_type;
    const char *current_func_return_source_type;
    int current_func_return_is_ptr;
    const char *current_struct_name;
    LLVMBasicBlockRef break_block;
    LLVMBasicBlockRef continue_block;
    int has_terminator;
    int loop_depth;
};

/* ── Helpers ───────────────────────────────────────────────── */

static TypedValue tv_null(void) {
    TypedValue r = {NULL, NULL, NULL, NULL, 0};
    return r;
}

static TypedValue tv_make(LLVMValueRef val, LLVMTypeRef type, int is_ptr) {
    TypedValue r = {val, type, NULL, NULL, is_ptr};
    return r;
}

static TypedValue tv_make_p(LLVMValueRef val, LLVMTypeRef type, LLVMTypeRef pointee, int is_ptr) {
    TypedValue r = {val, type, pointee, NULL, is_ptr};
    return r;
}

static TypedValue tv_make_t(LLVMValueRef val, LLVMTypeRef type, int is_ptr, const char *source_type) {
    TypedValue r = {val, type, NULL, source_type, is_ptr};
    return r;
}

static TypedValue tv_make_p_t(LLVMValueRef val, LLVMTypeRef type, LLVMTypeRef pointee,
                              int is_ptr, const char *source_type) {
    TypedValue r = {val, type, pointee, source_type, is_ptr};
    return r;
}

static LLVMValueRef ensure_i1(CodegenCtx *ctx, LLVMValueRef val) {
    LLVMTypeRef ty = LLVMTypeOf(val);
    LLVMTypeKind kind = LLVMGetTypeKind(ty);
    if (kind == LLVMIntegerTypeKind && LLVMGetIntTypeWidth(ty) != 1) {
        return LLVMBuildICmp(ctx->builder, LLVMIntNE,
            val, LLVMConstInt(ty, 0, 0), "cond");
    }
    if (kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(ctx->builder, LLVMRealONE,
            val, LLVMConstReal(ty, 0.0), "cond");
    if (kind == LLVMPointerTypeKind)
        return LLVMBuildICmp(ctx->builder, LLVMIntNE,
            val, LLVMConstNull(ty), "cond");
    return val;
}

static unsigned long long parse_uint_literal_value(const char *text) {
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

    unsigned long long value = 0;
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

static long long parse_int_literal_value(const char *text) {
    return (long long)parse_uint_literal_value(text);
}

static double parse_float_literal_value(const char *text) {
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

static unsigned char parse_char_literal_value(const char *text) {
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

static LLVMTypeRef llvm_int(CodegenCtx *ctx, int bits) {
    return LLVMIntTypeInContext(ctx->context, bits);
}

static LLVMTypeRef llvm_void(CodegenCtx *ctx) {
    return LLVMVoidTypeInContext(ctx->context);
}

static LLVMTypeRef llvm_i1(CodegenCtx *ctx)  { return LLVMInt1TypeInContext(ctx->context); }
static LLVMTypeRef llvm_i8(CodegenCtx *ctx)  { return LLVMInt8TypeInContext(ctx->context); }
static LLVMTypeRef llvm_i32(CodegenCtx *ctx) { return LLVMInt32TypeInContext(ctx->context); }
static LLVMTypeRef llvm_i64(CodegenCtx *ctx) { return LLVMInt64TypeInContext(ctx->context); }

static int codegen_has_package_prefix(CodegenCtx *ctx, const char *name) {
    if (!ctx || !ctx->package_prefix || !name)
        return 0;
    size_t prefix_len = strlen(ctx->package_prefix);
    return strncmp(name, ctx->package_prefix, prefix_len) == 0 &&
           name[prefix_len] == '.';
}

static char *codegen_package_symbol_name(CodegenCtx *ctx, const char *name) {
    if (!ctx || !ctx->package_prefix || !name || !*name ||
        strcmp(name, "main") == 0 || codegen_has_package_prefix(ctx, name))
        return name ? strdup(name) : NULL;

    size_t prefix_len = strlen(ctx->package_prefix);
    size_t name_len = strlen(name);
    char *result = malloc(prefix_len + name_len + 2);
    memcpy(result, ctx->package_prefix, prefix_len);
    result[prefix_len] = '.';
    memcpy(result + prefix_len + 1, name, name_len);
    result[prefix_len + name_len + 1] = '\0';
    return result;
}

static LLVMValueRef codegen_lookup_function(CodegenCtx *ctx, const char *name,
                                            const char **out_name) {
    if (out_name) *out_name = name;
    LLVMValueRef func = LLVMGetNamedFunction(ctx->module, name);
    if (func) return func;

    char *prefixed = codegen_package_symbol_name(ctx, name);
    if (!prefixed || strcmp(prefixed, name) == 0) {
        free(prefixed);
        return NULL;
    }

    func = LLVMGetNamedFunction(ctx->module, prefixed);
    if (func && out_name)
        *out_name = prefixed;
    else
        free(prefixed);
    return func;
}

static LLVMValueRef codegen_lookup_global(CodegenCtx *ctx, const char *name,
                                          const char **out_name) {
    if (out_name) *out_name = name;
    LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, name);
    if (global) return global;

    char *prefixed = codegen_package_symbol_name(ctx, name);
    if (!prefixed || strcmp(prefixed, name) == 0) {
        free(prefixed);
        return NULL;
    }

    global = LLVMGetNamedGlobal(ctx->module, prefixed);
    if (global && out_name)
        *out_name = prefixed;
    else
        free(prefixed);
    return global;
}

static void codegen_add_weak_alias(CodegenCtx *ctx, const char *alias_name,
                                   LLVMValueRef aliasee) {
    if (!ctx || !ctx->package_prefix || !alias_name || !aliasee ||
        strcmp(alias_name, "main") == 0 ||
        codegen_has_package_prefix(ctx, alias_name) ||
        LLVMGetNamedFunction(ctx->module, alias_name) ||
        LLVMGetNamedGlobal(ctx->module, alias_name) ||
        LLVMGetNamedGlobalAlias(ctx->module, alias_name, strlen(alias_name)))
        return;

    LLVMTypeRef value_ty = LLVMGlobalGetValueType(aliasee);
    LLVMValueRef alias = LLVMAddAlias2(ctx->module, value_ty, 0,
                                       aliasee, alias_name);
    LLVMSetLinkage(alias, LLVMWeakAnyLinkage);
}

static LLVMBasicBlockRef make_bb(LLVMContextRef ctx, LLVMValueRef func, const char *name) {
    LLVMBasicBlockRef bb = LLVMAppendBasicBlockInContext(ctx, func, name);
    return bb;
}

static int value_name_eq(LLVMValueRef v, const char *name) {
    const char *n = LLVMGetValueName(v);
    return n && strcmp(n, name) == 0;
}

/* ── Type mapping ──────────────────────────────────────────── */

static VarInfo *var_info_find(CodegenCtx *ctx, const char *name);
static InterfaceTypeInfo *find_interface_info(CodegenCtx *ctx, const char *name);
static InterfaceTypeInfo *find_interface_info_by_type(CodegenCtx *ctx, LLVMTypeRef type);

static LLVMTypeRef resolve_type(CodegenCtx *ctx, const char *name) {
    if (!name) return llvm_void(ctx);

    if (strcmp(name, "void") == 0) return llvm_void(ctx);
    if (strcmp(name, "self") == 0 && ctx->current_struct_name)
        name = ctx->current_struct_name;
    if (strcmp(name, "bool") == 0) return llvm_i1(ctx);
    if (strcmp(name, "i8")  == 0 || strcmp(name, "char") == 0) return llvm_i8(ctx);
    if (strcmp(name, "i16") == 0) return llvm_int(ctx, 16);
    if (strcmp(name, "i32") == 0) return llvm_i32(ctx);
    if (strcmp(name, "i64") == 0) return llvm_i64(ctx);
    if (strcmp(name, "u8")  == 0) return llvm_i8(ctx);
    if (strcmp(name, "u16") == 0) return llvm_int(ctx, 16);
    if (strcmp(name, "u32") == 0) return llvm_i32(ctx);
    if (strcmp(name, "u64") == 0) return llvm_i64(ctx);
    if (strcmp(name, "f32") == 0) return LLVMFloatTypeInContext(ctx->context);
    if (strcmp(name, "f64") == 0) return LLVMDoubleTypeInContext(ctx->context);
    if (strcmp(name, "str") == 0) return LLVMPointerType(llvm_i8(ctx), 0);

    for (int i = 0; i < ctx->named_type_count; i++) {
        if (strcmp(ctx->named_types[i].name, name) == 0)
            return ctx->named_types[i].type;
    }
    InterfaceTypeInfo *iface = find_interface_info(ctx, name);
    if (iface) return iface->type;
    return NULL;
}

static LLVMTypeRef resolve_type_full(CodegenCtx *ctx, const char *type_name, int is_ptr) {
    if (!type_name) return llvm_void(ctx);

    size_t len = strlen(type_name);
    if (len > 0 && type_name[len - 1] == '*') {
        char *base = malloc(len);
        memcpy(base, type_name, len - 1);
        base[len - 1] = '\0';
        LLVMTypeRef bt = resolve_type(ctx, base);
        free(base);
        if (bt) return LLVMPointerType(bt, 0);
        return LLVMPointerType(llvm_i8(ctx), 0);
    }

    LLVMTypeRef base = resolve_type(ctx, type_name);
    if (!base) return LLVMPointerType(llvm_i8(ctx), 0);
    if (is_ptr) return LLVMPointerType(base, 0);
    return base;
}

static LLVMTypeRef resolve_sizeof_type(CodegenCtx *ctx, const char *type_name) {
    if (!type_name) return NULL;

    size_t len = strlen(type_name);
    if (len > 0 && type_name[len - 1] == '*') {
        char *base = malloc(len);
        memcpy(base, type_name, len - 1);
        base[len - 1] = '\0';
        LLVMTypeRef bt = resolve_type(ctx, base);
        free(base);
        return LLVMPointerType(bt ? bt : llvm_i8(ctx), 0);
    }

    VarInfo *vi = var_info_find(ctx, type_name);
    if (vi) return vi->pointee_type;

    LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, type_name);
    if (global) return LLVMGlobalGetValueType(global);

    return resolve_type(ctx, type_name);
}

/* ── Named type registration ───────────────────────────────── */

static void register_named_type(CodegenCtx *ctx, const char *name, LLVMTypeRef type) {
    if (ctx->named_type_count == ctx->named_type_cap) {
        ctx->named_type_cap = ctx->named_type_cap ? ctx->named_type_cap * 2 : 16;
        ctx->named_types = realloc(ctx->named_types, sizeof(NamedType) * ctx->named_type_cap);
    }
    ctx->named_types[ctx->named_type_count].name = strdup(name);
    ctx->named_types[ctx->named_type_count].type = type;
    ctx->named_type_count++;
}

static void register_const(CodegenCtx *ctx, const char *name, LLVMValueRef value, LLVMTypeRef type) {
    if (ctx->const_count == ctx->const_cap) {
        ctx->const_cap = ctx->const_cap ? ctx->const_cap * 2 : 16;
        ctx->consts = realloc(ctx->consts, sizeof(ConstInfo) * ctx->const_cap);
    }
    ctx->consts[ctx->const_count].name = strdup(name);
    ctx->consts[ctx->const_count].value = value;
    ctx->consts[ctx->const_count].type = type;
    ctx->const_count++;
}

static ConstInfo *find_const(CodegenCtx *ctx, const char *name) {
    for (int i = ctx->const_count - 1; i >= 0; i--) {
        if (strcmp(ctx->consts[i].name, name) == 0)
            return &ctx->consts[i];
    }
    return NULL;
}

/* ── Variable info tracking ────────────────────────────────── */

static void var_info_add(CodegenCtx *ctx, const char *name, LLVMTypeRef value_type,
                         LLVMTypeRef element_type, const char *source_type,
                         int is_ptr, LLVMValueRef alloca) {
    if (ctx->var_info_count == ctx->var_info_cap) {
        ctx->var_info_cap = ctx->var_info_cap ? ctx->var_info_cap * 2 : 16;
        ctx->var_infos = realloc(ctx->var_infos, sizeof(VarInfo) * ctx->var_info_cap);
    }
    ctx->var_infos[ctx->var_info_count].name = strdup(name);
    ctx->var_infos[ctx->var_info_count].pointee_type = value_type;
    ctx->var_infos[ctx->var_info_count].element_type = element_type;
    ctx->var_infos[ctx->var_info_count].source_type = source_type ? strdup(source_type) : NULL;
    ctx->var_infos[ctx->var_info_count].is_ptr = is_ptr;
    ctx->var_infos[ctx->var_info_count].alloca = alloca;
    ctx->var_info_count++;
}

static VarInfo *var_info_find(CodegenCtx *ctx, const char *name) {
    for (int i = ctx->var_info_count - 1; i >= 0; i--) {
        if (strcmp(ctx->var_infos[i].name, name) == 0)
            return &ctx->var_infos[i];
    }
    return NULL;
}

static void register_global_type(CodegenCtx *ctx, const char *name, const char *source_type) {
    if (!name || !source_type) return;
    for (int i = 0; i < ctx->global_type_count; i++) {
        if (strcmp(ctx->global_types[i].name, name) == 0)
            return;
    }
    if (ctx->global_type_count == ctx->global_type_cap) {
        ctx->global_type_cap = ctx->global_type_cap ? ctx->global_type_cap * 2 : 16;
        ctx->global_types = realloc(ctx->global_types, sizeof(GlobalTypeInfo) * ctx->global_type_cap);
    }
    ctx->global_types[ctx->global_type_count].name = strdup(name);
    ctx->global_types[ctx->global_type_count].source_type = strdup(source_type);
    ctx->global_type_count++;
}

static const char *find_global_source_type(CodegenCtx *ctx, const char *name) {
    for (int i = ctx->global_type_count - 1; i >= 0; i--) {
        if (strcmp(ctx->global_types[i].name, name) == 0)
            return ctx->global_types[i].source_type;
    }
    return NULL;
}

static InterfaceTypeInfo *find_interface_info(CodegenCtx *ctx, const char *name) {
    if (!name) return NULL;
    for (int i = ctx->interface_count - 1; i >= 0; i--) {
        if (strcmp(ctx->interfaces[i].name, name) == 0)
            return &ctx->interfaces[i];
    }
    return NULL;
}

static InterfaceTypeInfo *find_interface_info_by_type(CodegenCtx *ctx, LLVMTypeRef type) {
    if (!type) return NULL;
    for (int i = ctx->interface_count - 1; i >= 0; i--) {
        if (ctx->interfaces[i].type == type)
            return &ctx->interfaces[i];
    }
    return NULL;
}

static int interface_method_index(InterfaceTypeInfo *info, const char *name) {
    if (!info || !name) return -1;
    int idx = 0;
    for (InterfaceMethodList *ml = info->methods; ml; ml = ml->next, idx++) {
        if (ml->method.name && strcmp(ml->method.name, name) == 0)
            return idx;
    }
    return -1;
}

static FunctionInfo *find_function_info(CodegenCtx *ctx, const char *name) {
    if (!name) return NULL;
    for (int i = ctx->function_count - 1; i >= 0; i--) {
        if (strcmp(ctx->functions[i].name, name) == 0)
            return &ctx->functions[i];
    }
    return NULL;
}

static void register_function_info(CodegenCtx *ctx, const char *name,
                                   FuncParamList *params, int hidden_self) {
    if (!name || find_function_info(ctx, name)) return;
    if (ctx->function_count == ctx->function_cap) {
        ctx->function_cap = ctx->function_cap ? ctx->function_cap * 2 : 16;
        ctx->functions = realloc(ctx->functions, sizeof(FunctionInfo) * ctx->function_cap);
    }
    ctx->functions[ctx->function_count].name = strdup(name);
    ctx->functions[ctx->function_count].params = params;
    ctx->functions[ctx->function_count].hidden_self = hidden_self;
    ctx->function_count++;
}

static FuncParam *function_param_at(FunctionInfo *info, int arg_index) {
    if (!info || arg_index < 0) return NULL;
    int idx = 0;
    for (FuncParamList *pl = info->params; pl; pl = pl->next, idx++) {
        if (idx == arg_index)
            return &pl->param;
    }
    return NULL;
}

/* ── Struct field tracking ─────────────────────────────────── */

static void register_struct_fields(CodegenCtx *ctx, const char *struct_name,
                                    char **field_names,
                                    LLVMTypeRef *field_element_types,
                                    int field_count) {
    if (ctx->struct_field_count == ctx->struct_field_cap) {
        ctx->struct_field_cap = ctx->struct_field_cap ? ctx->struct_field_cap * 2 : 16;
        ctx->struct_fields = realloc(ctx->struct_fields,
            sizeof(StructFieldInfo) * ctx->struct_field_cap);
    }
    StructFieldInfo *sf = &ctx->struct_fields[ctx->struct_field_count];
    sf->struct_name = strdup(struct_name);
    sf->field_count = field_count;
    sf->field_names = malloc(sizeof(char*) * field_count);
    sf->field_element_types = malloc(sizeof(LLVMTypeRef) * field_count);
    for (int i = 0; i < field_count; i++) {
        sf->field_names[i] = strdup(field_names[i]);
        sf->field_element_types[i] = field_element_types ? field_element_types[i] : NULL;
    }
    ctx->struct_field_count++;
}

/* ── Find alloca by name ──────────────────────────────────── */

static LLVMValueRef find_alloca(CodegenCtx *ctx, const char *name) {
    LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(ctx->current_func);
    for (LLVMValueRef instr = LLVMGetFirstInstruction(entry);
         instr; instr = LLVMGetNextInstruction(instr)) {
        if (LLVMGetInstructionOpcode(instr) == LLVMAlloca &&
            value_name_eq(instr, name))
            return instr;
    }
    return NULL;
}

/* ── Typed load: load a value from a ptr/alloca ──────────── */

static TypedValue typed_load(CodegenCtx *ctx, TypedValue tv) {
    if (!tv.val) return tv_null();
    /* Global constants are already pointers to data — bitcast to i8* if needed */
    if (LLVMIsAGlobalVariable(tv.val) && LLVMIsGlobalConstant(tv.val)) {
        LLVMTypeRef ptr_ty = LLVMPointerType(llvm_i8(ctx), 0);
        LLVMValueRef casted = LLVMBuildBitCast(ctx->builder, tv.val, ptr_ty, "");
        return tv_make(casted, ptr_ty, 0);
    }
    if (tv.is_ptr) {
        LLVMValueRef loaded = LLVMBuildLoad2(ctx->builder, tv.type, tv.val, "load");
        return tv_make_p_t(loaded, tv.type, tv.pointee_type, 0, tv.source_type);
    }
    return tv;
}

static LLVMValueRef coerce_value(CodegenCtx *ctx, LLVMValueRef val, LLVMTypeRef target_ty) {
    if (!val || !target_ty) return val;
    LLVMTypeRef src_ty = LLVMTypeOf(val);
    if (src_ty == target_ty) return val;

    LLVMTypeKind src_kind = LLVMGetTypeKind(src_ty);
    LLVMTypeKind dst_kind = LLVMGetTypeKind(target_ty);
    if (src_kind == LLVMIntegerTypeKind && dst_kind == LLVMIntegerTypeKind) {
        unsigned sw = LLVMGetIntTypeWidth(src_ty);
        unsigned dw = LLVMGetIntTypeWidth(target_ty);
        if (sw < dw) return LLVMBuildSExt(ctx->builder, val, target_ty, "sext");
        if (sw > dw) return LLVMBuildTrunc(ctx->builder, val, target_ty, "trunc");
        return val;
    }
    if ((src_kind == LLVMFloatTypeKind || src_kind == LLVMDoubleTypeKind) &&
        (dst_kind == LLVMFloatTypeKind || dst_kind == LLVMDoubleTypeKind)) {
        if (src_kind == LLVMFloatTypeKind && dst_kind == LLVMDoubleTypeKind)
            return LLVMBuildFPExt(ctx->builder, val, target_ty, "fpext");
        if (src_kind == LLVMDoubleTypeKind && dst_kind == LLVMFloatTypeKind)
            return LLVMBuildFPTrunc(ctx->builder, val, target_ty, "fptrunc");
        return val;
    }
    if (src_kind == LLVMIntegerTypeKind &&
        (dst_kind == LLVMFloatTypeKind || dst_kind == LLVMDoubleTypeKind)) {
        return LLVMBuildSIToFP(ctx->builder, val, target_ty, "sitofp");
    }
    if ((src_kind == LLVMFloatTypeKind || src_kind == LLVMDoubleTypeKind) &&
        dst_kind == LLVMIntegerTypeKind) {
        return LLVMBuildFPToSI(ctx->builder, val, target_ty, "fptosi");
    }
    if (src_kind == LLVMPointerTypeKind && dst_kind == LLVMPointerTypeKind) {
        return LLVMBuildBitCast(ctx->builder, val, target_ty, "ptrcast");
    }
    return val;
}

static int llvm_is_float_type(LLVMTypeRef ty) {
    LLVMTypeKind kind = LLVMGetTypeKind(ty);
    return kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind;
}

static int source_type_is_unsigned(const char *type) {
    if (!type) return 0;
    return strcmp(type, "u8") == 0 || strcmp(type, "u16") == 0 ||
           strcmp(type, "u32") == 0 || strcmp(type, "u64") == 0;
}

static int typed_value_is_unsigned(TypedValue tv) {
    return source_type_is_unsigned(tv.source_type);
}

static LLVMTypeRef wider_integer_type(LLVMTypeRef a, LLVMTypeRef b) {
    if (LLVMGetTypeKind(a) != LLVMIntegerTypeKind ||
        LLVMGetTypeKind(b) != LLVMIntegerTypeKind)
        return a;
    return LLVMGetIntTypeWidth(a) >= LLVMGetIntTypeWidth(b) ? a : b;
}

static void coerce_binary_values(CodegenCtx *ctx, TypedValue *left, TypedValue *right) {
    if (!left->val || !right->val) return;
    LLVMTypeRef lt = LLVMTypeOf(left->val);
    LLVMTypeRef rt = LLVMTypeOf(right->val);
    if (lt == rt) {
        left->type = lt;
        right->type = rt;
        return;
    }

    LLVMTypeKind lk = LLVMGetTypeKind(lt);
    LLVMTypeKind rk = LLVMGetTypeKind(rt);
    LLVMTypeRef target = lt;
    if (llvm_is_float_type(lt) || llvm_is_float_type(rt)) {
        if (lk == LLVMDoubleTypeKind || rk == LLVMDoubleTypeKind)
            target = LLVMDoubleTypeInContext(ctx->context);
        else
            target = LLVMFloatTypeInContext(ctx->context);
    } else if (lk == LLVMIntegerTypeKind && rk == LLVMIntegerTypeKind) {
        target = wider_integer_type(lt, rt);
    } else if (lk == LLVMPointerTypeKind && rk == LLVMPointerTypeKind) {
        target = lt;
    } else {
        return;
    }

    left->val = coerce_value(ctx, left->val, target);
    right->val = coerce_value(ctx, right->val, target);
    left->type = target;
    right->type = target;
}

/* ── Find struct field index ──────────────────────────────── */

static int find_field_index(CodegenCtx *ctx, LLVMTypeRef struct_type, const char *field_name) {
    if (LLVMGetTypeKind(struct_type) != LLVMStructTypeKind) return -1;

    /* Find the struct name from named_types */
    const char *struct_name = NULL;
    for (int i = 0; i < ctx->named_type_count; i++) {
        if (ctx->named_types[i].type == struct_type) {
            struct_name = ctx->named_types[i].name;
            break;
        }
    }
    if (!struct_name) return -1;

    /* Look up field names from struct_fields */
    for (int i = 0; i < ctx->struct_field_count; i++) {
        if (strcmp(ctx->struct_fields[i].struct_name, struct_name) == 0) {
            for (int j = 0; j < ctx->struct_fields[i].field_count; j++) {
                if (strcmp(ctx->struct_fields[i].field_names[j], field_name) == 0)
                    return j;
            }
            return -1;
        }
    }
    return -1;
}

static LLVMTypeRef find_field_element_type(CodegenCtx *ctx, LLVMTypeRef struct_type, int field_index) {
    if (LLVMGetTypeKind(struct_type) != LLVMStructTypeKind || field_index < 0)
        return NULL;

    const char *struct_name = NULL;
    for (int i = 0; i < ctx->named_type_count; i++) {
        if (ctx->named_types[i].type == struct_type) {
            struct_name = ctx->named_types[i].name;
            break;
        }
    }
    if (!struct_name) return NULL;

    for (int i = 0; i < ctx->struct_field_count; i++) {
        if (strcmp(ctx->struct_fields[i].struct_name, struct_name) == 0) {
            if (field_index < ctx->struct_fields[i].field_count)
                return ctx->struct_fields[i].field_element_types[field_index];
            return NULL;
        }
    }
    return NULL;
}

/* ── Forward declarations ──────────────────────────────────── */

static void codegen_decl(CodegenCtx *ctx, Decl *decl);
static void codegen_func(CodegenCtx *ctx, Decl *decl);
static void codegen_struct(CodegenCtx *ctx, Decl *decl);
static TypedValue codegen_expr(CodegenCtx *ctx, Expr *expr);
static void codegen_stmt(CodegenCtx *ctx, Stmt *stmt);
static void codegen_block(CodegenCtx *ctx, Stmt *stmt);
static void codegen_initializer_store(CodegenCtx *ctx, LLVMValueRef slot,
                                      LLVMTypeRef target_type, Expr *init);
static LLVMValueRef codegen_initializer_value(CodegenCtx *ctx, LLVMTypeRef target_type,
                                              Expr *init);

static TypedValue codegen_member_ptr(CodegenCtx *ctx, Expr *object, const char *field) {
    TypedValue obj = codegen_expr(ctx, object);
    if (!obj.val) return tv_null();

    LLVMValueRef base = obj.val;
    LLVMTypeRef struct_type = obj.type;

    if (LLVMGetTypeKind(struct_type) == LLVMPointerTypeKind) {
        if (obj.is_ptr) {
            TypedValue loaded = typed_load(ctx, obj);
            if (!loaded.val) return tv_null();
            base = loaded.val;
            if (loaded.pointee_type)
                struct_type = loaded.pointee_type;
        } else if (obj.pointee_type) {
            struct_type = obj.pointee_type;
        }
    }

    if (LLVMGetTypeKind(struct_type) != LLVMStructTypeKind)
        return tv_null();

    int idx = find_field_index(ctx, struct_type, field);
    if (idx < 0) return tv_null();

    LLVMValueRef indices[2] = {
        LLVMConstInt(llvm_i32(ctx), 0, 0),
        LLVMConstInt(llvm_i32(ctx), idx, 0)
    };
    LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, struct_type,
        base, indices, 2, field);
    LLVMTypeRef field_type = LLVMStructGetTypeAtIndex(struct_type, (unsigned)idx);
    LLVMTypeRef elem_type = find_field_element_type(ctx, struct_type, idx);
    return tv_make_p(gep, field_type, elem_type, 1);
}

static TypedValue codegen_self_field_ptr(CodegenCtx *ctx, const char *field) {
    if (!ctx->current_struct_name || !field) return tv_null();
    VarInfo *self_vi = var_info_find(ctx, "self");
    if (!self_vi || !self_vi->alloca || !self_vi->element_type) return tv_null();
    if (LLVMGetTypeKind(self_vi->element_type) != LLVMStructTypeKind) return tv_null();

    int idx = find_field_index(ctx, self_vi->element_type, field);
    if (idx < 0) return tv_null();

    LLVMValueRef self_ptr = LLVMBuildLoad2(ctx->builder, self_vi->pointee_type,
        self_vi->alloca, "self_field_base");
    LLVMValueRef indices[2] = {
        LLVMConstInt(llvm_i32(ctx), 0, 0),
        LLVMConstInt(llvm_i32(ctx), idx, 0)
    };
    LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, self_vi->element_type,
        self_ptr, indices, 2, field);
    LLVMTypeRef field_type = LLVMStructGetTypeAtIndex(self_vi->element_type, (unsigned)idx);
    LLVMTypeRef elem_type = find_field_element_type(ctx, self_vi->element_type, idx);
    return tv_make_p(gep, field_type, elem_type, 1);
}

static char *codegen_qualified_member_name_if_package(CodegenCtx *ctx,
                                                      Expr *object,
                                                      const char *field) {
    if (!object || object->type != EXPR_IDENTIFIER || !field)
        return NULL;
    const char *package_name = object->data.identifier.name;
    if (var_info_find(ctx, package_name) ||
        find_const(ctx, package_name) ||
        LLVMGetNamedGlobal(ctx->module, package_name) ||
        LLVMGetNamedFunction(ctx->module, package_name))
        return NULL;
    LLVMTypeRef current_struct = ctx->current_struct_name
        ? resolve_type(ctx, ctx->current_struct_name)
        : NULL;
    if (current_struct && find_field_index(ctx, current_struct, package_name) >= 0)
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

static TypedValue codegen_index_ptr(CodegenCtx *ctx, Expr *array_expr, Expr *index_expr) {
    TypedValue arr = codegen_expr(ctx, array_expr);
    TypedValue idx_tv = codegen_expr(ctx, index_expr);
    TypedValue idx = typed_load(ctx, idx_tv);
    if (!arr.val || !idx.val) return tv_null();

    if (LLVMGetTypeKind(arr.type) == LLVMArrayTypeKind) {
        LLVMTypeRef elem_type = LLVMGetElementType(arr.type);
        LLVMValueRef indices[2] = {
            LLVMConstInt(llvm_i32(ctx), 0, 0),
            idx.val
        };
        LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, arr.type, arr.val,
            indices, 2, "idx");
        return tv_make_p_t(gep, elem_type, NULL, 1, arr.source_type);
    }

    LLVMTypeRef elem_type = arr.pointee_type ? arr.pointee_type : arr.type;
    if (arr.source_type && strcmp(arr.source_type, "str") == 0)
        elem_type = llvm_i8(ctx);
    LLVMValueRef base = arr.is_ptr
        ? LLVMBuildLoad2(ctx->builder, arr.type, arr.val, "deref")
        : arr.val;
    LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, elem_type, base,
        (LLVMValueRef[]){idx.val}, 1, "idx");
    return tv_make_p_t(gep, elem_type, NULL, 1, arr.source_type);
}

static TypedValue decay_array_to_pointer(CodegenCtx *ctx, TypedValue value) {
    if (!value.val || !value.is_ptr || LLVMGetTypeKind(value.type) != LLVMArrayTypeKind)
        return value;

    LLVMTypeRef elem_type = LLVMGetElementType(value.type);
    LLVMValueRef indices[2] = {
        LLVMConstInt(llvm_i32(ctx), 0, 0),
        LLVMConstInt(llvm_i32(ctx), 0, 0)
    };
    LLVMValueRef ptr = LLVMBuildGEP2(ctx->builder, value.type, value.val,
        indices, 2, "array.decay");
    return tv_make_p_t(ptr, LLVMPointerType(elem_type, 0), elem_type, 0, value.source_type);
}

static LLVMValueRef codegen_build_interface_value(CodegenCtx *ctx,
                                                  InterfaceTypeInfo *iface,
                                                  TypedValue concrete,
                                                  int heap_alloc) {
    if (!iface || !concrete.val || !concrete.source_type)
        return NULL;

    const char *struct_name = concrete.source_type;
    size_t len = strlen(struct_name);
    char *base_name = NULL;
    if (len > 0 && struct_name[len - 1] == '*') {
        base_name = malloc(len);
        memcpy(base_name, struct_name, len - 1);
        base_name[len - 1] = '\0';
        struct_name = base_name;
    }

    LLVMTypeRef struct_ty = resolve_type(ctx, struct_name);
    if (!struct_ty || LLVMGetTypeKind(struct_ty) != LLVMStructTypeKind) {
        free(base_name);
        return NULL;
    }

    LLVMValueRef object_ptr = NULL;
    if (concrete.is_ptr) {
        TypedValue loaded = typed_load(ctx, concrete);
        object_ptr = loaded.val;
    } else {
        object_ptr = concrete.val;
    }
    if (!object_ptr) {
        free(base_name);
        return NULL;
    }

    LLVMValueRef iface_alloca = NULL;
    if (heap_alloc) {
        iface_alloca = LLVMBuildMalloc(ctx->builder, iface->type, "iface.heap");
    } else {
        LLVMBuilderRef tmp = LLVMCreateBuilderInContext(ctx->context);
        LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(ctx->current_func);
        LLVMValueRef first_inst = LLVMGetFirstInstruction(entry);
        if (first_inst)
            LLVMPositionBuilderBefore(tmp, first_inst);
        else
            LLVMPositionBuilderAtEnd(tmp, entry);
        iface_alloca = LLVMBuildAlloca(tmp, iface->type, "iface.tmp");
        LLVMDisposeBuilder(tmp);
    }

    LLVMValueRef obj_slot = LLVMBuildStructGEP2(ctx->builder, iface->type,
                                                iface_alloca, 0, "iface.obj");
    LLVMBuildStore(ctx->builder,
                   LLVMBuildBitCast(ctx->builder, object_ptr,
                                    LLVMPointerType(llvm_i8(ctx), 0), "iface.objcast"),
                   obj_slot);

    int idx = 0;
    for (InterfaceMethodList *ml = iface->methods; ml; ml = ml->next, idx++) {
        char method_name[512];
        snprintf(method_name, sizeof(method_name), "%s.%s",
                 struct_name, ml->method.name ? ml->method.name : "");
        const char *resolved_method_name = method_name;
        LLVMValueRef method = codegen_lookup_function(ctx, method_name,
                                                      &resolved_method_name);
        if (!method) {
            free(base_name);
            return NULL;
        }
        if (resolved_method_name != method_name)
            free((char*)resolved_method_name);
        LLVMValueRef method_slot = LLVMBuildStructGEP2(ctx->builder, iface->type,
                                                       iface_alloca, (unsigned)(idx + 1),
                                                       "iface.method");
        LLVMBuildStore(ctx->builder,
                       LLVMBuildBitCast(ctx->builder, method,
                                        LLVMPointerType(llvm_i8(ctx), 0), "iface.fncast"),
                       method_slot);
    }

    free(base_name);
    return iface_alloca;
}

static LLVMValueRef coerce_to_interface_pointer(CodegenCtx *ctx,
                                                LLVMValueRef value,
                                                LLVMTypeRef target_ty,
                                                TypedValue original) {
    if (!target_ty || LLVMGetTypeKind(target_ty) != LLVMPointerTypeKind)
        return value;
    LLVMTypeRef target_pointee = LLVMGetElementType(target_ty);
    InterfaceTypeInfo *iface = find_interface_info_by_type(ctx, target_pointee);
    if (!iface)
        return value;
    if (LLVMTypeOf(value) == target_ty)
        return value;
    LLVMValueRef iface_value = codegen_build_interface_value(ctx, iface, original, 0);
    return iface_value ? iface_value : value;
}

static LLVMValueRef coerce_to_declared_interface_pointer(CodegenCtx *ctx,
                                                         LLVMValueRef value,
                                                         const char *type_name,
                                                         int is_ptr,
                                                         TypedValue original,
                                                         int heap_alloc) {
    if (!type_name || !is_ptr)
        return value;
    size_t len = strlen(type_name);
    if (len == 0 || type_name[len - 1] != '*')
        return value;

    char *base = malloc(len);
    memcpy(base, type_name, len - 1);
    base[len - 1] = '\0';
    InterfaceTypeInfo *iface = find_interface_info(ctx, base);
    free(base);
    if (!iface)
        return value;

    if (original.source_type) {
        const char *source = original.source_type;
        size_t source_len = strlen(source);
        char *source_base = NULL;
        if (source_len > 0 && source[source_len - 1] == '*') {
            source_base = malloc(source_len);
            memcpy(source_base, source, source_len - 1);
            source_base[source_len - 1] = '\0';
            source = source_base;
        }
        int already_interface = find_interface_info(ctx, source) != NULL;
        free(source_base);
        if (already_interface)
            return value;
    }

    if (!original.source_type)
        return value;
    LLVMValueRef iface_value = codegen_build_interface_value(ctx, iface, original,
                                                             heap_alloc);
    return iface_value ? iface_value : value;
}

static TypedValue codegen_named_call(CodegenCtx *ctx, const char *func_name, LLVMValueRef func,
                                     ExprList *arg_exprs,
                                     int implicit_method_call) {
    if (!func) return tv_null();

    int argc = 0;
    for (ExprList *al = arg_exprs; al; al = al->next) argc++;
    int total_argc = argc + (implicit_method_call ? 1 : 0);

    LLVMTypeRef func_ty = LLVMGlobalGetValueType(func);
    LLVMTypeRef ret_ty = LLVMGetReturnType(func_ty);

    LLVMValueRef *args = NULL;
    if (total_argc > 0) args = malloc(sizeof(LLVMValueRef) * total_argc);
    unsigned param_count = LLVMCountParamTypes(func_ty);
    LLVMTypeRef *param_types = NULL;
    if (param_count > 0) param_types = malloc(sizeof(LLVMTypeRef) * param_count);
    LLVMGetParamTypes(func_ty, param_types);
    FunctionInfo *finfo = find_function_info(ctx, func_name);

    int idx = 0;
    if (implicit_method_call) {
        VarInfo *self_vi = var_info_find(ctx, "self");
        if (!self_vi || !self_vi->alloca) {
            free(param_types);
            free(args);
            return tv_null();
        }
        LLVMValueRef self_arg = LLVMBuildLoad2(ctx->builder,
            self_vi->pointee_type, self_vi->alloca, "self_arg");
        if (param_count > 0)
            self_arg = coerce_value(ctx, self_arg, param_types[0]);
        args[idx++] = self_arg;
    }

    for (ExprList *al = arg_exprs; al; al = al->next, idx++) {
        LLVMValueRef arg_val;
        if (idx < (int)param_count && al->expr &&
            al->expr->type == EXPR_ARRAY_LIT &&
            (LLVMGetTypeKind(param_types[idx]) == LLVMStructTypeKind ||
             LLVMGetTypeKind(param_types[idx]) == LLVMArrayTypeKind)) {
            arg_val = codegen_initializer_value(ctx, param_types[idx], al->expr);
        } else {
            TypedValue a = decay_array_to_pointer(ctx, codegen_expr(ctx, al->expr));
            if (a.is_ptr && !LLVMIsAGlobalVariable(a.val)) {
                arg_val = LLVMBuildLoad2(ctx->builder, a.type, a.val, "arg");
            } else {
                TypedValue loaded = typed_load(ctx, a);
                arg_val = loaded.val;
            }
            FuncParam *decl_param = finfo
                ? function_param_at(finfo, idx - (implicit_method_call ? 1 : 0))
                : NULL;
            if (decl_param) {
                arg_val = coerce_to_declared_interface_pointer(ctx, arg_val,
                                                               decl_param->type_name,
                                                               decl_param->is_ptr, a, 0);
            } else if (idx < (int)param_count) {
                arg_val = coerce_to_interface_pointer(ctx, arg_val, param_types[idx], a);
            }
        }
        if (idx < (int)param_count)
            arg_val = coerce_value(ctx, arg_val, param_types[idx]);
        args[idx] = arg_val;
    }
    free(param_types);

    LLVMValueRef result;
    if (LLVMGetTypeKind(ret_ty) == LLVMVoidTypeKind) {
        result = LLVMBuildCall2(ctx->builder, func_ty, func,
                                args, (unsigned)total_argc, "");
    } else {
        result = LLVMBuildCall2(ctx->builder, func_ty, func,
                                args, (unsigned)total_argc, "call");
    }
    free(args);
    return tv_make(result, ret_ty, 0);
}

/* ── Expression codegen ────────────────────────────────────── */

static TypedValue codegen_expr(CodegenCtx *ctx, Expr *expr) {
    if (!expr) return tv_null();

    switch (expr->type) {

    case EXPR_INT_LIT: {
        long long val = parse_int_literal_value(expr->data.int_lit.value);
        return tv_make(LLVMConstInt(llvm_i32(ctx), (unsigned long long)val, 0),
                       llvm_i32(ctx), 0);
    }

    case EXPR_FLOAT_LIT: {
        double val = parse_float_literal_value(expr->data.float_lit.value);
        LLVMTypeRef ty = LLVMDoubleTypeInContext(ctx->context);
        return tv_make(LLVMConstReal(ty, val), ty, 0);
    }

    case EXPR_CHAR_LIT: {
        unsigned char c = parse_char_literal_value(expr->data.char_lit.value);
        return tv_make(LLVMConstInt(llvm_i8(ctx), (unsigned long long)c, 0),
                       llvm_i8(ctx), 0);
    }

    case EXPR_BOOL_LIT:
        return tv_make(LLVMConstInt(llvm_i1(ctx), (unsigned long long)expr->data.bool_lit.value, 0),
                       llvm_i1(ctx), 0);

    case EXPR_STRING_LIT: {
        const char *raw = expr->data.string_lit.value;
        /* strip surrounding quotes if present */
        const char *str = raw;
        if (str[0] == '"') str++;
        size_t slen = strlen(str);
        if (slen > 0 && str[slen - 1] == '"') slen--;
        unsigned len = (unsigned)slen;
        LLVMTypeRef arr_type = LLVMArrayType(llvm_i8(ctx), len + 1);
        LLVMValueRef glob = LLVMAddGlobal(ctx->module, arr_type, ".str");
        LLVMSetLinkage(glob, LLVMPrivateLinkage);
        LLVMSetGlobalConstant(glob, 1);
        LLVMSetInitializer(glob, LLVMConstStringInContext(ctx->context, str, len, 0));
        LLVMTypeRef ptr_ty = LLVMPointerType(llvm_i8(ctx), 0);
        return tv_make(glob, ptr_ty, 1);
    }

    case EXPR_NULL: {
        LLVMTypeRef ptr_ty = LLVMPointerType(llvm_i8(ctx), 0);
        return tv_make(LLVMConstNull(ptr_ty), ptr_ty, 0);
    }

    case EXPR_SIZEOF: {
        LLVMTypeRef ty = resolve_sizeof_type(ctx, expr->data.sizeof_expr.type_name);
        if (!ty) return tv_make(LLVMConstInt(llvm_i64(ctx), 0, 0), llvm_i64(ctx), 0);
        LLVMValueRef size = LLVMConstTruncOrBitCast(LLVMSizeOf(ty), llvm_i64(ctx));
        return tv_make(size, llvm_i64(ctx), 0);
    }

    case EXPR_ARRAY_LIT:
        return tv_null();

    case EXPR_CONDITIONAL: {
        TypedValue cond = codegen_expr(ctx, expr->data.conditional.condition);
        if (!cond.val) return tv_null();
        TypedValue cond_loaded = typed_load(ctx, cond);
        LLVMValueRef cond_i1 = ensure_i1(ctx, cond_loaded.val);

        LLVMBasicBlockRef then_bb = make_bb(ctx->context, ctx->current_func, "cond.then");
        LLVMBasicBlockRef else_bb = make_bb(ctx->context, ctx->current_func, "cond.else");
        LLVMBasicBlockRef merge_bb = make_bb(ctx->context, ctx->current_func, "cond.end");
        LLVMBuildCondBr(ctx->builder, cond_i1, then_bb, else_bb);

        LLVMPositionBuilderAtEnd(ctx->builder, then_bb);
        TypedValue then_tv = codegen_expr(ctx, expr->data.conditional.then_expr);
        if (!then_tv.val) return tv_null();
        TypedValue then_loaded = typed_load(ctx, then_tv);
        LLVMBasicBlockRef then_end = LLVMGetInsertBlock(ctx->builder);
        LLVMBuildBr(ctx->builder, merge_bb);

        LLVMPositionBuilderAtEnd(ctx->builder, else_bb);
        TypedValue else_tv = codegen_expr(ctx, expr->data.conditional.else_expr);
        if (!else_tv.val) return tv_null();
        TypedValue else_loaded = typed_load(ctx, else_tv);
        LLVMBasicBlockRef else_end = LLVMGetInsertBlock(ctx->builder);
        LLVMBuildBr(ctx->builder, merge_bb);

        LLVMPositionBuilderAtEnd(ctx->builder, merge_bb);
        coerce_binary_values(ctx, &then_loaded, &else_loaded);
        LLVMValueRef phi = LLVMBuildPhi(ctx->builder, then_loaded.type, "cond");
        LLVMValueRef vals[2] = { then_loaded.val, else_loaded.val };
        LLVMBasicBlockRef bbs[2] = { then_end, else_end };
        LLVMAddIncoming(phi, vals, bbs, 2);
        return tv_make_t(phi, then_loaded.type, 0, then_loaded.source_type);
    }

    case EXPR_IDENTIFIER: {
        const char *name = expr->data.identifier.name;

        VarInfo *vi = var_info_find(ctx, name);
        if (vi) return tv_make_p_t(vi->alloca, vi->pointee_type, vi->element_type, 1, vi->source_type);

        ConstInfo *ci = find_const(ctx, name);
        if (ci) return tv_make(ci->value, ci->type, 0);

        const char *resolved_global_name = name;
        LLVMValueRef global = codegen_lookup_global(ctx, name, &resolved_global_name);
        if (global) {
            LLVMTypeRef gty = LLVMGlobalGetValueType(global);
            const char *source_type = find_global_source_type(ctx, resolved_global_name);
            if (!source_type && resolved_global_name != name)
                source_type = find_global_source_type(ctx, name);
            TypedValue result = tv_make_p_t(global, gty, NULL, 1, source_type);
            if (resolved_global_name != name)
                free((char*)resolved_global_name);
            return result;
        }

        const char *resolved_func_name = name;
        LLVMValueRef func = codegen_lookup_function(ctx, name, &resolved_func_name);
        if (func) {
            LLVMTypeRef fty = LLVMGlobalGetValueType(func);
            TypedValue result = tv_make(func, fty, 0);
            if (resolved_func_name != name)
                free((char*)resolved_func_name);
            return result;
        }

        TypedValue self_field = codegen_self_field_ptr(ctx, name);
        if (self_field.val) return self_field;

        return tv_null();
    }

    case EXPR_BINARY: {
        if (expr->data.binary.op == AmpersandAmpersand ||
            expr->data.binary.op == PipePipe) {
            TypedValue left = codegen_expr(ctx, expr->data.binary.left);
            if (!left.val) return tv_null();
            TypedValue ll = typed_load(ctx, left);
            LLVMValueRef left_i1 = ensure_i1(ctx, ll.val);

            LLVMBasicBlockRef lhs_bb = LLVMGetInsertBlock(ctx->builder);
            LLVMBasicBlockRef rhs_bb = make_bb(ctx->context, ctx->current_func, "logic.rhs");
            LLVMBasicBlockRef merge_bb = make_bb(ctx->context, ctx->current_func, "logic.end");
            int is_and = expr->data.binary.op == AmpersandAmpersand;

            if (is_and)
                LLVMBuildCondBr(ctx->builder, left_i1, rhs_bb, merge_bb);
            else
                LLVMBuildCondBr(ctx->builder, left_i1, merge_bb, rhs_bb);

            LLVMPositionBuilderAtEnd(ctx->builder, rhs_bb);
            TypedValue right = codegen_expr(ctx, expr->data.binary.right);
            if (!right.val) return tv_null();
            TypedValue rr = typed_load(ctx, right);
            LLVMValueRef right_i1 = ensure_i1(ctx, rr.val);
            LLVMBasicBlockRef rhs_end_bb = LLVMGetInsertBlock(ctx->builder);
            LLVMBuildBr(ctx->builder, merge_bb);

            LLVMPositionBuilderAtEnd(ctx->builder, merge_bb);
            LLVMValueRef phi = LLVMBuildPhi(ctx->builder, llvm_i1(ctx),
                                            is_and ? "land" : "lor");
            LLVMValueRef short_value = LLVMConstInt(llvm_i1(ctx), is_and ? 0 : 1, 0);
            LLVMValueRef incoming_vals[2] = { short_value, right_i1 };
            LLVMBasicBlockRef incoming_bbs[2] = { lhs_bb, rhs_end_bb };
            LLVMAddIncoming(phi, incoming_vals, incoming_bbs, 2);
            return tv_make(LLVMBuildZExt(ctx->builder, phi, llvm_i32(ctx), "logic_ext"),
                           llvm_i32(ctx), 0);
        }

        if (expr->data.binary.op == Equals &&
            expr->data.binary.right &&
            expr->data.binary.right->type == EXPR_ARRAY_LIT) {
            Expr *lhs = expr->data.binary.left;
            LLVMValueRef ptr = NULL;
            LLVMTypeRef target_ty = NULL;
            if (lhs->type == EXPR_IDENTIFIER) {
                VarInfo *vi = var_info_find(ctx, lhs->data.identifier.name);
                ptr = vi ? vi->alloca : NULL;
                target_ty = vi ? vi->pointee_type : NULL;
                if (!ptr) {
                    const char *resolved_global_name = lhs->data.identifier.name;
                    ptr = codegen_lookup_global(ctx, lhs->data.identifier.name,
                                                &resolved_global_name);
                    if (ptr) target_ty = LLVMGlobalGetValueType(ptr);
                    if (resolved_global_name != lhs->data.identifier.name)
                        free((char*)resolved_global_name);
                }
                if (!ptr) {
                    TypedValue self_field = codegen_self_field_ptr(ctx, lhs->data.identifier.name);
                    ptr = self_field.val;
                    target_ty = self_field.type;
                }
            } else if (lhs->type == EXPR_MEMBER) {
                TypedValue field = codegen_member_ptr(ctx, lhs->data.member.object,
                                                      lhs->data.member.field);
                ptr = field.val;
                target_ty = field.type;
            } else if (lhs->type == EXPR_DEREF) {
                TypedValue operand = codegen_expr(ctx, lhs->data.deref.operand);
                if (operand.val) {
                    TypedValue loaded_ptr = typed_load(ctx, operand);
                    ptr = loaded_ptr.val;
                    target_ty = loaded_ptr.pointee_type ? loaded_ptr.pointee_type : loaded_ptr.type;
                }
            } else if (lhs->type == EXPR_INDEX) {
                TypedValue indexed = codegen_index_ptr(ctx, lhs->data.index.array,
                                                       lhs->data.index.index);
                ptr = indexed.val;
                target_ty = indexed.type;
            }
            if (ptr && target_ty) {
                codegen_initializer_store(ctx, ptr, target_ty, expr->data.binary.right);
                LLVMValueRef loaded = LLVMBuildLoad2(ctx->builder, target_ty, ptr, "assign.load");
                return tv_make(loaded, target_ty, 0);
            }
            return tv_null();
        }

        TypedValue left = codegen_expr(ctx, expr->data.binary.left);
        TypedValue right = codegen_expr(ctx, expr->data.binary.right);
        if (!left.val || !right.val) return tv_null();

        switch (expr->data.binary.op) {
        case Plus: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef result = llvm_is_float_type(ll.type)
                ? LLVMBuildFAdd(ctx->builder, ll.val, rr.val, "fadd")
                : LLVMBuildAdd(ctx->builder, ll.val, rr.val, "add");
            return tv_make(result,
                           ll.type, 0);
        }
        case Minus: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef result = llvm_is_float_type(ll.type)
                ? LLVMBuildFSub(ctx->builder, ll.val, rr.val, "fsub")
                : LLVMBuildSub(ctx->builder, ll.val, rr.val, "sub");
            return tv_make(result,
                           ll.type, 0);
        }
        case Star: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef result = llvm_is_float_type(ll.type)
                ? LLVMBuildFMul(ctx->builder, ll.val, rr.val, "fmul")
                : LLVMBuildMul(ctx->builder, ll.val, rr.val, "mul");
            return tv_make(result,
                           ll.type, 0);
        }
        case Slash: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            int use_unsigned = typed_value_is_unsigned(ll) || typed_value_is_unsigned(rr);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef result = llvm_is_float_type(ll.type)
                ? LLVMBuildFDiv(ctx->builder, ll.val, rr.val, "fdiv")
                : (use_unsigned
                    ? LLVMBuildUDiv(ctx->builder, ll.val, rr.val, "udiv")
                    : LLVMBuildSDiv(ctx->builder, ll.val, rr.val, "div"));
            return tv_make(result,
                           ll.type, 0);
        }
        case Percent: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            int use_unsigned = typed_value_is_unsigned(ll) || typed_value_is_unsigned(rr);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef result = llvm_is_float_type(ll.type)
                ? LLVMBuildFRem(ctx->builder, ll.val, rr.val, "frem")
                : (use_unsigned
                    ? LLVMBuildURem(ctx->builder, ll.val, rr.val, "urem")
                    : LLVMBuildSRem(ctx->builder, ll.val, rr.val, "rem"));
            return tv_make(result,
                           ll.type, 0);
        }
        case EqualsEquals: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef cmp = llvm_is_float_type(ll.type)
                ? LLVMBuildFCmp(ctx->builder, LLVMRealOEQ, ll.val, rr.val, "feq")
                : LLVMBuildICmp(ctx->builder, LLVMIntEQ, ll.val, rr.val, "eq");
            return tv_make(LLVMBuildZExt(ctx->builder, cmp, llvm_i32(ctx), "eq_ext"),
                           llvm_i32(ctx), 0);
        }
        case BangEquals: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef cmp = llvm_is_float_type(ll.type)
                ? LLVMBuildFCmp(ctx->builder, LLVMRealONE, ll.val, rr.val, "fne")
                : LLVMBuildICmp(ctx->builder, LLVMIntNE, ll.val, rr.val, "ne");
            return tv_make(LLVMBuildZExt(ctx->builder, cmp, llvm_i32(ctx), "ne_ext"),
                           llvm_i32(ctx), 0);
        }
        case Less: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            int use_unsigned = typed_value_is_unsigned(ll) || typed_value_is_unsigned(rr);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef cmp = llvm_is_float_type(ll.type)
                ? LLVMBuildFCmp(ctx->builder, LLVMRealOLT, ll.val, rr.val, "flt")
                : LLVMBuildICmp(ctx->builder, use_unsigned ? LLVMIntULT : LLVMIntSLT, ll.val, rr.val, "slt");
            return tv_make(LLVMBuildZExt(ctx->builder, cmp, llvm_i32(ctx), "slt_ext"),
                           llvm_i32(ctx), 0);
        }
        case LessEquals: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            int use_unsigned = typed_value_is_unsigned(ll) || typed_value_is_unsigned(rr);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef cmp = llvm_is_float_type(ll.type)
                ? LLVMBuildFCmp(ctx->builder, LLVMRealOLE, ll.val, rr.val, "fle")
                : LLVMBuildICmp(ctx->builder, use_unsigned ? LLVMIntULE : LLVMIntSLE, ll.val, rr.val, "sle");
            return tv_make(LLVMBuildZExt(ctx->builder, cmp, llvm_i32(ctx), "sle_ext"),
                           llvm_i32(ctx), 0);
        }
        case Greater: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            int use_unsigned = typed_value_is_unsigned(ll) || typed_value_is_unsigned(rr);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef cmp = llvm_is_float_type(ll.type)
                ? LLVMBuildFCmp(ctx->builder, LLVMRealOGT, ll.val, rr.val, "fgt")
                : LLVMBuildICmp(ctx->builder, use_unsigned ? LLVMIntUGT : LLVMIntSGT, ll.val, rr.val, "sgt");
            return tv_make(LLVMBuildZExt(ctx->builder, cmp, llvm_i32(ctx), "sgt_ext"),
                           llvm_i32(ctx), 0);
        }
        case GreaterEquals: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            int use_unsigned = typed_value_is_unsigned(ll) || typed_value_is_unsigned(rr);
            coerce_binary_values(ctx, &ll, &rr);
            LLVMValueRef cmp = llvm_is_float_type(ll.type)
                ? LLVMBuildFCmp(ctx->builder, LLVMRealOGE, ll.val, rr.val, "fge")
                : LLVMBuildICmp(ctx->builder, use_unsigned ? LLVMIntUGE : LLVMIntSGE, ll.val, rr.val, "sge");
            return tv_make(LLVMBuildZExt(ctx->builder, cmp, llvm_i32(ctx), "sge_ext"),
                           llvm_i32(ctx), 0);
        }
        case AmpersandAmpersand: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            LLVMValueRef cmp = LLVMBuildAnd(ctx->builder, ensure_i1(ctx, ll.val), ensure_i1(ctx, rr.val), "and");
            return tv_make(LLVMBuildZExt(ctx->builder, cmp, llvm_i32(ctx), "and_ext"),
                           llvm_i32(ctx), 0);
        }
        case Ampersand: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            coerce_binary_values(ctx, &ll, &rr);
            return tv_make(LLVMBuildAnd(ctx->builder, ll.val, rr.val, "band"),
                           ll.type, 0);
        }
        case Caret: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            coerce_binary_values(ctx, &ll, &rr);
            return tv_make(LLVMBuildXor(ctx->builder, ll.val, rr.val, "xor"),
                           ll.type, 0);
        }
        case LeftShift: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            coerce_binary_values(ctx, &ll, &rr);
            return tv_make(LLVMBuildShl(ctx->builder, ll.val, rr.val, "shl"),
                           ll.type, 0);
        }
        case RightShift: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            int use_unsigned = typed_value_is_unsigned(ll);
            coerce_binary_values(ctx, &ll, &rr);
            return tv_make((use_unsigned
                                ? LLVMBuildLShr(ctx->builder, ll.val, rr.val, "lshr")
                                : LLVMBuildAShr(ctx->builder, ll.val, rr.val, "ashr")),
                           ll.type, 0);
        }
        case Pipe: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            coerce_binary_values(ctx, &ll, &rr);
            return tv_make(LLVMBuildOr(ctx->builder, ll.val, rr.val, "or"),
                           ll.type, 0);
        }
        case PipePipe: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            LLVMValueRef cmp = LLVMBuildOr(ctx->builder, ensure_i1(ctx, ll.val), ensure_i1(ctx, rr.val), "lor");
            return tv_make(LLVMBuildZExt(ctx->builder, cmp, llvm_i32(ctx), "or_ext"),
                           llvm_i32(ctx), 0);
        }
        case PlusEquals: case MinusEquals:
        case StarEquals: case SlashEquals:
        case PercentEquals:
        case AmpersandEquals: case PipeEquals: case CaretEquals:
        case LeftShiftEquals: case RightShiftEquals: {
            Expr *lhs = expr->data.binary.left;
            LLVMValueRef ptr = NULL;
            LLVMTypeRef ptr_type = NULL;
            const char *ptr_source_type = NULL;
            if (lhs->type == EXPR_IDENTIFIER) {
                VarInfo *vi = var_info_find(ctx, lhs->data.identifier.name);
                ptr = vi ? vi->alloca : NULL;
                if (vi) {
                    ptr_type = vi->pointee_type ? vi->pointee_type : LLVMGetElementType(LLVMTypeOf(ptr));
                    ptr_source_type = vi->source_type;
                }
                if (!ptr) {
                    const char *resolved_global_name = lhs->data.identifier.name;
                    ptr = codegen_lookup_global(ctx, lhs->data.identifier.name,
                                                &resolved_global_name);
                    if (ptr) {
                        ptr_type = LLVMGlobalGetValueType(ptr);
                        ptr_source_type = find_global_source_type(ctx, resolved_global_name);
                        if (!ptr_source_type && resolved_global_name != lhs->data.identifier.name)
                            ptr_source_type = find_global_source_type(ctx, lhs->data.identifier.name);
                    }
                    if (resolved_global_name != lhs->data.identifier.name)
                        free((char*)resolved_global_name);
                }
                if (!ptr) {
                    TypedValue self_field = codegen_self_field_ptr(ctx, lhs->data.identifier.name);
                    ptr = self_field.val;
                    ptr_type = self_field.type;
                    ptr_source_type = self_field.source_type;
                }
            } else if (lhs->type == EXPR_MEMBER) {
                TypedValue field = codegen_member_ptr(ctx, lhs->data.member.object,
                                                      lhs->data.member.field);
                ptr = field.val;
                ptr_type = field.type;
                ptr_source_type = field.source_type;
            } else if (lhs->type == EXPR_DEREF) {
                TypedValue operand = codegen_expr(ctx, lhs->data.deref.operand);
                if (operand.val) {
                    TypedValue loaded_ptr = typed_load(ctx, operand);
                    LLVMTypeRef target_ty = loaded_ptr.pointee_type;
                    if (!target_ty && LLVMGetTypeKind(loaded_ptr.type) == LLVMPointerTypeKind)
                        target_ty = LLVMGetElementType(loaded_ptr.type);
                    if (!target_ty) target_ty = loaded_ptr.type;
                    ptr = loaded_ptr.val;
                    ptr_type = target_ty;
                    ptr_source_type = loaded_ptr.source_type;
                }
            } else if (lhs->type == EXPR_INDEX) {
                TypedValue indexed = codegen_index_ptr(ctx, lhs->data.index.array,
                                                       lhs->data.index.index);
                if (indexed.val) {
                    ptr = indexed.val;
                    ptr_type = indexed.type;
                    ptr_source_type = indexed.source_type;
                }
            }
            if (ptr) {
                LLVMValueRef cur = LLVMBuildLoad2(ctx->builder, ptr_type, ptr, "cur");
                TypedValue rval = typed_load(ctx, right);
                LLVMValueRef rhs = coerce_value(ctx, rval.val, ptr_type);
                    LLVMValueRef result = NULL;
                    switch (expr->data.binary.op) {
                    case PlusEquals:
                        result = llvm_is_float_type(ptr_type)
                            ? LLVMBuildFAdd(ctx->builder, cur, rhs, "fadd")
                            : LLVMBuildAdd(ctx->builder, cur, rhs, "add");
                        break;
                    case MinusEquals:
                        result = llvm_is_float_type(ptr_type)
                            ? LLVMBuildFSub(ctx->builder, cur, rhs, "fsub")
                            : LLVMBuildSub(ctx->builder, cur, rhs, "sub");
                        break;
                    case StarEquals:
                        result = llvm_is_float_type(ptr_type)
                            ? LLVMBuildFMul(ctx->builder, cur, rhs, "fmul")
                            : LLVMBuildMul(ctx->builder, cur, rhs, "mul");
                        break;
                    case SlashEquals:
                        result = llvm_is_float_type(ptr_type)
                            ? LLVMBuildFDiv(ctx->builder, cur, rhs, "fdiv")
                            : (source_type_is_unsigned(ptr_source_type)
                                ? LLVMBuildUDiv(ctx->builder, cur, rhs, "udiv")
                                : LLVMBuildSDiv(ctx->builder, cur, rhs, "div"));
                        break;
                    case PercentEquals:
                        result = llvm_is_float_type(ptr_type)
                            ? LLVMBuildFRem(ctx->builder, cur, rhs, "frem")
                            : (source_type_is_unsigned(ptr_source_type)
                                ? LLVMBuildURem(ctx->builder, cur, rhs, "urem")
                                : LLVMBuildSRem(ctx->builder, cur, rhs, "rem"));
                        break;
                    case AmpersandEquals: result = LLVMBuildAnd(ctx->builder, cur, rhs, "band"); break;
                    case PipeEquals:    result = LLVMBuildOr(ctx->builder, cur, rhs, "bor"); break;
                    case CaretEquals:   result = LLVMBuildXor(ctx->builder, cur, rhs, "bxor"); break;
                    case LeftShiftEquals:
                        result = LLVMBuildShl(ctx->builder, cur, rhs, "shl");
                        break;
                    case RightShiftEquals:
                        result = source_type_is_unsigned(ptr_source_type)
                            ? LLVMBuildLShr(ctx->builder, cur, rhs, "lshr")
                            : LLVMBuildAShr(ctx->builder, cur, rhs, "ashr");
                        break;
                    default: break;
                }
                if (result) {
                    LLVMBuildStore(ctx->builder, result, ptr);
                    return tv_make(result, ptr_type, 0);
                }
            }
            return tv_null();
        }
        case Equals: {
            Expr *lhs = expr->data.binary.left;
            if (lhs->type == EXPR_IDENTIFIER) {
                VarInfo *vi = var_info_find(ctx, lhs->data.identifier.name);
                LLVMValueRef ptr = vi ? vi->alloca : NULL;
                if (!ptr) {
                    const char *resolved_global_name = lhs->data.identifier.name;
                    ptr = codegen_lookup_global(ctx, lhs->data.identifier.name,
                                                &resolved_global_name);
                    if (resolved_global_name != lhs->data.identifier.name)
                        free((char*)resolved_global_name);
                }
                if (ptr) {
                    TypedValue rval = typed_load(ctx, right);
                    LLVMTypeRef target_ty = vi ? vi->pointee_type : LLVMGlobalGetValueType(ptr);
                    LLVMValueRef coerced = coerce_value(ctx, rval.val, target_ty);
                    LLVMBuildStore(ctx->builder, coerced, ptr);
                    return tv_make(coerced, target_ty, 0);
                }
            }
            if (lhs->type == EXPR_DEREF) {
                TypedValue operand = codegen_expr(ctx, lhs->data.deref.operand);
                if (operand.val) {
                    TypedValue loaded_ptr = typed_load(ctx, operand);
                    LLVMTypeRef target_ty = loaded_ptr.pointee_type;
                    if (!target_ty && LLVMGetTypeKind(loaded_ptr.type) == LLVMPointerTypeKind)
                        target_ty = LLVMGetElementType(loaded_ptr.type);
                    if (!target_ty) target_ty = loaded_ptr.type;
                    TypedValue rval = typed_load(ctx, right);
                    LLVMValueRef coerced = coerce_value(ctx, rval.val, target_ty);
                    LLVMBuildStore(ctx->builder, coerced, loaded_ptr.val);
                    return tv_make(coerced, target_ty, 0);
                }
            }
            if (lhs->type == EXPR_MEMBER) {
                TypedValue field = codegen_member_ptr(ctx, lhs->data.member.object,
                                                      lhs->data.member.field);
                if (field.val) {
                    TypedValue rval = typed_load(ctx, right);
                    LLVMValueRef coerced = coerce_value(ctx, rval.val, field.type);
                    LLVMBuildStore(ctx->builder, coerced, field.val);
                    return tv_make(coerced, field.type, 0);
                }
            }
            if (lhs->type == EXPR_INDEX) {
                TypedValue indexed = codegen_index_ptr(ctx, lhs->data.index.array,
                                                       lhs->data.index.index);
                if (indexed.val) {
                    TypedValue rval = typed_load(ctx, right);
                    LLVMTypeRef elem_type = indexed.type;
                    LLVMValueRef coerced = coerce_value(ctx, rval.val, elem_type);
                    LLVMBuildStore(ctx->builder, coerced, indexed.val);
                    return tv_make(coerced, elem_type, 0);
                }
            }
            TypedValue rval = typed_load(ctx, right);
            return rval;
        }
        default:
            break;
        }
        return tv_null();
    }

    case EXPR_UNARY: {
        TypedValue operand = codegen_expr(ctx, expr->data.unary.operand);
        if (!operand.val) return tv_null();

        switch (expr->data.unary.op) {
        case Minus: {
            TypedValue loaded = typed_load(ctx, operand);
            LLVMValueRef result = llvm_is_float_type(loaded.type)
                ? LLVMBuildFNeg(ctx->builder, loaded.val, "fneg")
                : LLVMBuildNeg(ctx->builder, loaded.val, "neg");
            return tv_make(result,
                           loaded.type, 0);
        }
        case Star: {
            TypedValue loaded = typed_load(ctx, operand);
            if (loaded.is_ptr)
                return tv_make(loaded.val, loaded.type, 1);
            LLVMTypeRef ptr_ty = LLVMPointerType(operand.type, 0);
            LLVMValueRef derefed = LLVMBuildLoad2(ctx->builder, ptr_ty, operand.val, "deref");
            return tv_make(derefed, operand.type, 1);
        }
        case Ampersand:
            return tv_make(operand.val, operand.type, 0);
        case Bang: {
            TypedValue loaded = typed_load(ctx, operand);
            LLVMValueRef truthy = ensure_i1(ctx, loaded.val);
            LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, LLVMIntEQ, truthy,
                LLVMConstInt(llvm_i1(ctx), 0, 0), "not");
            return tv_make(LLVMBuildZExt(ctx->builder, cmp, llvm_i32(ctx), "not_ext"),
                           llvm_i32(ctx), 0);
        }
        case Tilde: {
            TypedValue loaded = typed_load(ctx, operand);
            return tv_make(LLVMBuildNot(ctx->builder, loaded.val, "bnot"),
                           loaded.type, 0);
        }
        default:
            return operand;
        }
    }

    case EXPR_CALL: {
        Expr *callee = expr->data.call.callee;
        if (callee->type == EXPR_MEMBER) {
            char *qualified_name = codegen_qualified_member_name_if_package(
                ctx, callee->data.member.object, callee->data.member.field);
            LLVMValueRef qualified_func = qualified_name
                ? LLVMGetNamedFunction(ctx->module, qualified_name)
                : NULL;
            if (qualified_func) {
                TypedValue result = codegen_named_call(ctx, qualified_name, qualified_func,
                                                       expr->data.call.args, 0);
                free(qualified_name);
                return result;
            }
            free(qualified_name);

            TypedValue receiver = codegen_expr(ctx, callee->data.member.object);
            if (!receiver.val) return tv_null();

            InterfaceTypeInfo *iface = receiver.pointee_type
                ? find_interface_info_by_type(ctx, receiver.pointee_type)
                : find_interface_info_by_type(ctx, receiver.type);
            if (iface) {
                TypedValue iface_ptr_tv = receiver.is_ptr ? typed_load(ctx, receiver) : receiver;
                LLVMValueRef iface_ptr = iface_ptr_tv.val;
                if (!iface_ptr) return tv_null();

                int method_idx = interface_method_index(iface, callee->data.member.field);
                if (method_idx < 0) return tv_null();

                InterfaceMethodList *ml = iface->methods;
                for (int i = 0; ml && i < method_idx; i++)
                    ml = ml->next;
                if (!ml) return tv_null();

                LLVMValueRef obj_slot = LLVMBuildStructGEP2(ctx->builder, iface->type,
                                                            iface_ptr, 0, "iface.obj");
                LLVMValueRef obj = LLVMBuildLoad2(ctx->builder,
                                                  LLVMPointerType(llvm_i8(ctx), 0),
                                                  obj_slot, "iface.obj.load");

                LLVMValueRef fn_slot = LLVMBuildStructGEP2(ctx->builder, iface->type,
                                                           iface_ptr,
                                                           (unsigned)(method_idx + 1),
                                                           "iface.fn");
                LLVMValueRef fn = LLVMBuildLoad2(ctx->builder,
                                                 LLVMPointerType(llvm_i8(ctx), 0),
                                                 fn_slot, "iface.fn.load");

                int explicit_argc = 0;
                for (ExprList *al = expr->data.call.args; al; al = al->next) explicit_argc++;
                int total_argc = explicit_argc + 1;
                int fixed_param_count = 0;
                for (FuncParamList *pl = ml->method.params; pl; pl = pl->next)
                    fixed_param_count++;
                int fn_param_count = fixed_param_count + 1;
                LLVMTypeRef *param_types = malloc(sizeof(LLVMTypeRef) * fn_param_count);
                param_types[0] = LLVMPointerType(llvm_i8(ctx), 0);
                int pidx = 1;
                for (FuncParamList *pl = ml->method.params; pl; pl = pl->next, pidx++) {
                    param_types[pidx] = resolve_type_full(ctx, pl->param.type_name,
                                                          pl->param.is_ptr);
                    if (!param_types[pidx]) param_types[pidx] = llvm_i8(ctx);
                }
                LLVMTypeRef ret_ty = resolve_type_full(ctx, ml->method.return_type, 0);
                if (!ret_ty) ret_ty = llvm_void(ctx);
                LLVMTypeRef fn_ty = LLVMFunctionType(ret_ty, param_types,
                                                     (unsigned)fn_param_count,
                                                     ml->method.is_variadic);

                LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * total_argc);
                args[0] = obj;
                int aidx = 1;
                for (ExprList *al = expr->data.call.args; al; al = al->next, aidx++) {
                    TypedValue a = decay_array_to_pointer(ctx, codegen_expr(ctx, al->expr));
                    LLVMValueRef arg_val;
                    if (a.is_ptr && !LLVMIsAGlobalVariable(a.val)) {
                        arg_val = LLVMBuildLoad2(ctx->builder, a.type, a.val, "arg");
                    } else {
                        TypedValue loaded = typed_load(ctx, a);
                        arg_val = loaded.val;
                    }
                    if (aidx < fn_param_count) {
                        arg_val = coerce_to_interface_pointer(ctx, arg_val,
                                                              param_types[aidx], a);
                        arg_val = coerce_value(ctx, arg_val, param_types[aidx]);
                    }
                    args[aidx] = arg_val;
                }
                free(param_types);

                LLVMValueRef result;
                if (LLVMGetTypeKind(ret_ty) == LLVMVoidTypeKind)
                    result = LLVMBuildCall2(ctx->builder, fn_ty, fn,
                                            args, (unsigned)total_argc, "");
                else
                    result = LLVMBuildCall2(ctx->builder, fn_ty, fn,
                                            args, (unsigned)total_argc, "iface.call");
                free(args);
                return tv_make(result, ret_ty, 0);
            }

            LLVMTypeRef struct_ty = receiver.type;
            if (LLVMGetTypeKind(struct_ty) == LLVMPointerTypeKind && receiver.pointee_type)
                struct_ty = receiver.pointee_type;
            if (LLVMGetTypeKind(struct_ty) != LLVMStructTypeKind)
                return tv_null();

            const char *struct_name = LLVMGetStructName(struct_ty);
            if (!struct_name) return tv_null();

            char fname[256];
            snprintf(fname, sizeof(fname), "%s.%s", struct_name, callee->data.member.field);
            const char *resolved_func_name = fname;
            LLVMValueRef func = codegen_lookup_function(ctx, fname, &resolved_func_name);
            if (!func) return tv_null();

            int explicit_argc = 0;
            for (ExprList *al = expr->data.call.args; al; al = al->next) explicit_argc++;
            int argc = explicit_argc + 1;

            LLVMTypeRef func_ty = LLVMGlobalGetValueType(func);
            LLVMTypeRef ret_ty = LLVMGetReturnType(func_ty);
            unsigned param_count = LLVMCountParamTypes(func_ty);
            LLVMTypeRef *param_types = NULL;
            if (param_count > 0) param_types = malloc(sizeof(LLVMTypeRef) * param_count);
            LLVMGetParamTypes(func_ty, param_types);

            LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * argc);
            if (receiver.is_ptr && LLVMGetTypeKind(receiver.type) == LLVMPointerTypeKind) {
                TypedValue loaded_receiver = typed_load(ctx, receiver);
                args[0] = loaded_receiver.val;
            } else {
                args[0] = receiver.val;
            }
            if (param_count > 0)
                args[0] = coerce_value(ctx, args[0], param_types[0]);

            int idx = 1;
            for (ExprList *al = expr->data.call.args; al; al = al->next, idx++) {
                LLVMValueRef arg_val;
                if (idx < (int)param_count && al->expr &&
                    al->expr->type == EXPR_ARRAY_LIT &&
                    (LLVMGetTypeKind(param_types[idx]) == LLVMStructTypeKind ||
                     LLVMGetTypeKind(param_types[idx]) == LLVMArrayTypeKind)) {
                    arg_val = codegen_initializer_value(ctx, param_types[idx], al->expr);
                } else {
                    TypedValue a = decay_array_to_pointer(ctx, codegen_expr(ctx, al->expr));
                    if (a.is_ptr && !LLVMIsAGlobalVariable(a.val)) {
                        arg_val = LLVMBuildLoad2(ctx->builder, a.type, a.val, "arg");
                    } else {
                        TypedValue loaded = typed_load(ctx, a);
                        arg_val = loaded.val;
                    }
                    if (idx < (int)param_count)
                        arg_val = coerce_to_interface_pointer(ctx, arg_val, param_types[idx], a);
                }
                if (idx < (int)param_count)
                    arg_val = coerce_value(ctx, arg_val, param_types[idx]);
                args[idx] = arg_val;
            }
            free(param_types);

            LLVMValueRef result;
            if (LLVMGetTypeKind(ret_ty) == LLVMVoidTypeKind) {
                result = LLVMBuildCall2(ctx->builder, func_ty, func,
                                        args, (unsigned)argc, "");
            } else {
                result = LLVMBuildCall2(ctx->builder, func_ty, func,
                                        args, (unsigned)argc, "call");
            }
            free(args);
            if (resolved_func_name != fname)
                free((char*)resolved_func_name);
            return tv_make(result, ret_ty, 0);
        }

        if (callee->type != EXPR_IDENTIFIER) return tv_null();

        const char *fname = callee->data.identifier.name;
        const char *resolved_func_name = fname;
        LLVMValueRef func = codegen_lookup_function(ctx, fname, &resolved_func_name);
        int implicit_method_call = 0;
        char method_name[256];
        if (!func && ctx->current_struct_name) {
            snprintf(method_name, sizeof(method_name), "%s.%s",
                     ctx->current_struct_name, fname);
            resolved_func_name = method_name;
            func = codegen_lookup_function(ctx, method_name, &resolved_func_name);
            implicit_method_call = func != NULL;
        }
        if (!func) return tv_null();
        const char *call_name = resolved_func_name;
        FunctionInfo *finfo = find_function_info(ctx, call_name);
        int pass_implicit_receiver = implicit_method_call &&
            (!finfo || finfo->hidden_self);

        int argc = 0;
        for (ExprList *al = expr->data.call.args; al; al = al->next) argc++;
        int total_argc = argc + (pass_implicit_receiver ? 1 : 0);

        LLVMTypeRef func_ty = LLVMGlobalGetValueType(func);
        LLVMTypeRef ret_ty = LLVMGetReturnType(func_ty);

        LLVMValueRef *args = NULL;
        if (total_argc > 0) args = malloc(sizeof(LLVMValueRef) * total_argc);
        unsigned param_count = LLVMCountParamTypes(func_ty);
        LLVMTypeRef *param_types = NULL;
        if (param_count > 0) param_types = malloc(sizeof(LLVMTypeRef) * param_count);
        LLVMGetParamTypes(func_ty, param_types);
        int idx = 0;
        if (pass_implicit_receiver) {
            VarInfo *self_vi = var_info_find(ctx, "self");
            if (!self_vi || !self_vi->alloca) {
                free(param_types);
                free(args);
                if (resolved_func_name != fname && resolved_func_name != method_name)
                    free((char*)resolved_func_name);
                return tv_null();
            }
            LLVMValueRef self_arg = LLVMBuildLoad2(ctx->builder,
                self_vi->pointee_type, self_vi->alloca, "self_arg");
            if (param_count > 0)
                self_arg = coerce_value(ctx, self_arg, param_types[0]);
            args[idx++] = self_arg;
        }
        for (ExprList *al = expr->data.call.args; al; al = al->next, idx++) {
            LLVMValueRef arg_val;
            if (idx < (int)param_count && al->expr &&
                al->expr->type == EXPR_ARRAY_LIT &&
                (LLVMGetTypeKind(param_types[idx]) == LLVMStructTypeKind ||
                 LLVMGetTypeKind(param_types[idx]) == LLVMArrayTypeKind)) {
                arg_val = codegen_initializer_value(ctx, param_types[idx], al->expr);
            } else {
                TypedValue a = decay_array_to_pointer(ctx, codegen_expr(ctx, al->expr));
                if (a.is_ptr && !LLVMIsAGlobalVariable(a.val)) {
                    arg_val = LLVMBuildLoad2(ctx->builder, a.type, a.val, "arg");
                } else {
                    TypedValue loaded = typed_load(ctx, a);
                    arg_val = loaded.val;
                }
                FuncParam *decl_param = finfo
                    ? function_param_at(finfo, idx - (pass_implicit_receiver ? 1 : 0))
                    : NULL;
                if (decl_param) {
                    arg_val = coerce_to_declared_interface_pointer(ctx, arg_val,
                                                                   decl_param->type_name,
                                                                   decl_param->is_ptr, a, 0);
                } else if (idx < (int)param_count) {
                    arg_val = coerce_to_interface_pointer(ctx, arg_val, param_types[idx], a);
                }
            }
            if (idx < (int)param_count)
                arg_val = coerce_value(ctx, arg_val, param_types[idx]);
            args[idx] = arg_val;
        }
        free(param_types);

        LLVMValueRef result;
        if (LLVMGetTypeKind(ret_ty) == LLVMVoidTypeKind) {
            result = LLVMBuildCall2(ctx->builder, func_ty, func,
                                    args, (unsigned)total_argc, "");
        } else {
            result = LLVMBuildCall2(ctx->builder, func_ty, func,
                                    args, (unsigned)total_argc, "call");
        }
        free(args);
        if (resolved_func_name != fname && resolved_func_name != method_name)
            free((char*)resolved_func_name);
        return tv_make(result, ret_ty, 0);
    }

    case EXPR_MEMBER: {
        char *qualified_name = codegen_qualified_member_name_if_package(
            ctx, expr->data.member.object, expr->data.member.field);
        if (qualified_name) {
            ConstInfo *ci = find_const(ctx, qualified_name);
            if (ci) {
                TypedValue result = tv_make(ci->value, ci->type, 0);
                free(qualified_name);
                return result;
            }

            LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, qualified_name);
            if (global) {
                LLVMTypeRef gty = LLVMGlobalGetValueType(global);
                TypedValue result = tv_make_p_t(global, gty, NULL, 1,
                                                find_global_source_type(ctx, qualified_name));
                free(qualified_name);
                return result;
            }

            LLVMValueRef func = LLVMGetNamedFunction(ctx->module, qualified_name);
            if (func) {
                LLVMTypeRef fty = LLVMGlobalGetValueType(func);
                TypedValue result = tv_make(func, fty, 0);
                free(qualified_name);
                return result;
            }

            free(qualified_name);
        }
        return codegen_member_ptr(ctx, expr->data.member.object,
                                  expr->data.member.field);
    }

    case EXPR_DEREF: {
        TypedValue operand = codegen_expr(ctx, expr->data.deref.operand);
        if (!operand.val) return tv_null();

        TypedValue loaded_ptr = typed_load(ctx, operand);
        if (!loaded_ptr.val) return tv_null();
        LLVMTypeRef target_ty = loaded_ptr.pointee_type;
        if (!target_ty && LLVMGetTypeKind(loaded_ptr.type) == LLVMPointerTypeKind)
            target_ty = LLVMGetElementType(loaded_ptr.type);
        if (!target_ty) target_ty = loaded_ptr.type;
        return tv_make(loaded_ptr.val, target_ty, 1);
    }

    case EXPR_ADDROF: {
        TypedValue operand = codegen_expr(ctx, expr->data.addrof.operand);
        if (!operand.val) return tv_null();
        return tv_make_p_t(operand.val, operand.type, operand.pointee_type,
                           0, operand.source_type);
    }

    case EXPR_CAST: {
        TypedValue operand = codegen_expr(ctx, expr->data.cast.operand);
        if (!operand.val) return tv_null();
        LLVMTypeRef target = resolve_type_full(ctx, expr->data.cast.type_name, 0);
        if (!target) return operand;
        TypedValue loaded = typed_load(ctx, operand);
        return tv_make(coerce_value(ctx, loaded.val, target), target, 0);
    }

    case EXPR_ASSIGN: {
        if (expr->data.assign.value &&
            expr->data.assign.value->type == EXPR_ARRAY_LIT) {
            LLVMValueRef ptr = find_alloca(ctx, expr->data.assign.name);
            const char *resolved_global_name = expr->data.assign.name;
            if (!ptr)
                ptr = codegen_lookup_global(ctx, expr->data.assign.name,
                                            &resolved_global_name);
            LLVMTypeRef target_ty = NULL;
            if (ptr) {
                VarInfo *vi = var_info_find(ctx, expr->data.assign.name);
                target_ty = vi ? vi->pointee_type : LLVMGlobalGetValueType(ptr);
            } else {
                TypedValue self_field = codegen_self_field_ptr(ctx, expr->data.assign.name);
                ptr = self_field.val;
                target_ty = self_field.type;
            }
            if (ptr && target_ty) {
                codegen_initializer_store(ctx, ptr, target_ty, expr->data.assign.value);
                LLVMValueRef loaded = LLVMBuildLoad2(ctx->builder, target_ty, ptr, "assign.load");
                if (resolved_global_name != expr->data.assign.name)
                    free((char*)resolved_global_name);
                return tv_make(loaded, target_ty, 0);
            }
            if (resolved_global_name != expr->data.assign.name)
                free((char*)resolved_global_name);
            return tv_null();
        }
        TypedValue val = codegen_expr(ctx, expr->data.assign.value);
        if (!val.val) return tv_null();
        TypedValue loaded = typed_load(ctx, val);
        LLVMValueRef ptr = find_alloca(ctx, expr->data.assign.name);
        const char *resolved_global_name = expr->data.assign.name;
        if (!ptr)
            ptr = codegen_lookup_global(ctx, expr->data.assign.name,
                                        &resolved_global_name);
        if (ptr) {
            VarInfo *vi = var_info_find(ctx, expr->data.assign.name);
            LLVMTypeRef target_ty = vi ? vi->pointee_type : LLVMGlobalGetValueType(ptr);
            LLVMValueRef coerced = vi
                ? coerce_to_declared_interface_pointer(ctx, loaded.val,
                                                       vi->source_type, vi->is_ptr, val, 0)
                : coerce_to_interface_pointer(ctx, loaded.val, target_ty, val);
            coerced = coerce_value(ctx, coerced, target_ty);
            LLVMBuildStore(ctx->builder, coerced, ptr);
            if (resolved_global_name != expr->data.assign.name)
                free((char*)resolved_global_name);
            return tv_make(coerced, target_ty, 0);
        }
        if (resolved_global_name != expr->data.assign.name)
            free((char*)resolved_global_name);
        TypedValue self_field = codegen_self_field_ptr(ctx, expr->data.assign.name);
        if (self_field.val) {
            LLVMValueRef coerced = coerce_value(ctx, loaded.val, self_field.type);
            LLVMBuildStore(ctx->builder, coerced, self_field.val);
            return tv_make(coerced, self_field.type, 0);
        }
        return loaded;
    }

    case EXPR_NEW: {
        LLVMTypeRef elem_type = resolve_type(ctx, expr->data.new_expr.type_name);
        if (!elem_type) return tv_null();
        
        LLVMTypeRef i64_ty = LLVMInt64TypeInContext(ctx->context);
        LLVMTypeRef i8_ptr_ty = LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0);
        
        LLVMValueRef total_count = NULL;
        for (ExprList *dl = expr->data.new_expr.dims; dl; dl = dl->next) {
            TypedValue dim = codegen_expr(ctx, dl->expr);
            TypedValue dim_loaded = typed_load(ctx, dim);
            LLVMValueRef dim_val = dim_loaded.val;
            if (LLVMGetTypeKind(LLVMTypeOf(dim_val)) != LLVMIntegerTypeKind ||
                LLVMGetIntTypeWidth(LLVMTypeOf(dim_val)) < 64) {
                dim_val = LLVMBuildZExt(ctx->builder, dim_val, i64_ty, "dim_zext");
            }
            if (!total_count) {
                total_count = dim_val;
            } else {
                total_count = LLVMBuildMul(ctx->builder, total_count, dim_val, "mul");
            }
        }
        
        LLVMTargetDataRef td = LLVMCreateTargetData(LLVMGetDataLayoutStr(ctx->module));
        LLVMValueRef elem_size = LLVMConstInt(i64_ty, LLVMABISizeOfType(td, elem_type), 0);
        LLVMDisposeTargetData(td);
        
        LLVMValueRef alloc_size = LLVMBuildMul(ctx->builder, total_count, elem_size, "alloc_size");
        
        LLVMValueRef malloc_fn = LLVMGetNamedFunction(ctx->module, "malloc");
        if (!malloc_fn) {
            LLVMTypeRef malloc_ty = LLVMFunctionType(i8_ptr_ty, (LLVMTypeRef[]){i64_ty}, 1, 0);
            malloc_fn = LLVMAddFunction(ctx->module, "malloc", malloc_ty);
        }
        
        LLVMTypeRef func_ty = LLVMFunctionType(i8_ptr_ty, (LLVMTypeRef[]){i64_ty}, 1, 0);
        LLVMValueRef raw_ptr = LLVMBuildCall2(ctx->builder, func_ty,
                                               malloc_fn, &alloc_size, 1, "malloc");
        
        LLVMTypeRef ptr_ty = LLVMPointerType(elem_type, 0);
        LLVMValueRef typed_ptr = LLVMBuildBitCast(ctx->builder, raw_ptr, ptr_ty, "new_ptr");
        
        return tv_make_p(typed_ptr, ptr_ty, elem_type, 0);
    }
    
    case EXPR_DELETE: {
        TypedValue operand = codegen_expr(ctx, expr->data.delete_expr.operand);
        if (!operand.val) return tv_null();
        
        LLVMTypeRef i8_ptr_ty = LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0);
        LLVMValueRef ptr;
        if (operand.is_ptr) {
            LLVMTypeRef ptr_ty = LLVMPointerType(operand.type, 0);
            ptr = LLVMBuildLoad2(ctx->builder, ptr_ty, operand.val, "del_ptr");
        } else {
            ptr = operand.val;
        }
        
        if (LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind) {
            if (LLVMTypeOf(ptr) != i8_ptr_ty) {
                ptr = LLVMBuildBitCast(ctx->builder, ptr, i8_ptr_ty, "free_cast");
            }
        }
        
        LLVMValueRef free_fn = LLVMGetNamedFunction(ctx->module, "free");
        if (!free_fn) {
            LLVMTypeRef free_ty = LLVMFunctionType(LLVMVoidTypeInContext(ctx->context), 
                                                   (LLVMTypeRef[]){i8_ptr_ty}, 1, 0);
            free_fn = LLVMAddFunction(ctx->module, "free", free_ty);
        }
        
        LLVMTypeRef free_func_ty = LLVMFunctionType(LLVMVoidTypeInContext(ctx->context), 
                                                    (LLVMTypeRef[]){i8_ptr_ty}, 1, 0);
        LLVMBuildCall2(ctx->builder, free_func_ty,
                       free_fn, &ptr, 1, "");
        
        return tv_null();
    }

    case EXPR_INDEX: {
        return codegen_index_ptr(ctx, expr->data.index.array, expr->data.index.index);
    }
    }

    return tv_null();
}

/* ── Statement codegen ────────────────────────────────────── */

static void codegen_initializer_store(CodegenCtx *ctx, LLVMValueRef slot,
                                      LLVMTypeRef target_type, Expr *init);

static void codegen_array_initializer_store(CodegenCtx *ctx, LLVMValueRef alloca,
                                            LLVMTypeRef array_type, LLVMTypeRef elem_type,
                                            Expr *init, int array_size) {
    if (!alloca || !array_type || !elem_type || !init || init->type != EXPR_ARRAY_LIT)
        return;

    (void)elem_type;
    (void)array_size;
    codegen_initializer_store(ctx, alloca, array_type, init);
}

static void codegen_struct_initializer_store(CodegenCtx *ctx, LLVMValueRef alloca,
                                             LLVMTypeRef struct_type, Expr *init) {
    if (!alloca || !struct_type || !init || init->type != EXPR_ARRAY_LIT ||
        LLVMGetTypeKind(struct_type) != LLVMStructTypeKind)
        return;

    codegen_initializer_store(ctx, alloca, struct_type, init);
}

static void codegen_initializer_store(CodegenCtx *ctx, LLVMValueRef slot,
                                      LLVMTypeRef target_type, Expr *init) {
    if (!slot || !target_type || !init) return;

    LLVMTypeKind kind = LLVMGetTypeKind(target_type);
    if (init->type == EXPR_ARRAY_LIT &&
        (kind == LLVMArrayTypeKind || kind == LLVMStructTypeKind)) {
        LLVMBuildStore(ctx->builder, LLVMConstNull(target_type), slot);

        if (kind == LLVMArrayTypeKind) {
            LLVMTypeRef elem_type = LLVMGetElementType(target_type);
            unsigned count = LLVMGetArrayLength(target_type);
            unsigned idx = 0;
            for (ExprList *el = init->data.array_lit.elements; el && idx < count;
                 el = el->next, idx++) {
                LLVMValueRef indices[2] = {
                    LLVMConstInt(llvm_i32(ctx), 0, 0),
                    LLVMConstInt(llvm_i32(ctx), idx, 0)
                };
                LLVMValueRef elem_slot = LLVMBuildGEP2(ctx->builder, target_type, slot,
                    indices, 2, "arr.init");
                codegen_initializer_store(ctx, elem_slot, elem_type, el->expr);
            }
            return;
        }

        unsigned count = LLVMCountStructElementTypes(target_type);
        unsigned idx = 0;
        for (ExprList *el = init->data.array_lit.elements; el && idx < count;
             el = el->next, idx++) {
            LLVMValueRef indices[2] = {
                LLVMConstInt(llvm_i32(ctx), 0, 0),
                LLVMConstInt(llvm_i32(ctx), idx, 0)
            };
            LLVMValueRef field_slot = LLVMBuildGEP2(ctx->builder, target_type, slot,
                indices, 2, "struct.init");
            LLVMTypeRef field_type = LLVMStructGetTypeAtIndex(target_type, idx);
            codegen_initializer_store(ctx, field_slot, field_type, el->expr);
        }
        return;
    }

    TypedValue value = codegen_expr(ctx, init);
    if (!value.val) return;
    TypedValue loaded = typed_load(ctx, value);
    LLVMValueRef coerced = coerce_value(ctx, loaded.val, target_type);
    LLVMBuildStore(ctx->builder, coerced, slot);
}

static LLVMValueRef codegen_initializer_value(CodegenCtx *ctx, LLVMTypeRef target_type,
                                              Expr *init) {
    if (!target_type || !init) return NULL;
    LLVMValueRef tmp = LLVMBuildAlloca(ctx->builder, target_type, "agg.tmp");
    codegen_initializer_store(ctx, tmp, target_type, init);
    return LLVMBuildLoad2(ctx->builder, target_type, tmp, "agg.load");
}

static void codegen_block(CodegenCtx *ctx, Stmt *stmt) {
    if (!stmt || stmt->type != STMT_BLOCK) return;
    for (StmtList *sl = stmt->data.block.stmts; sl; sl = sl->next) {
        if (ctx->has_terminator) break;
        codegen_stmt(ctx, sl->stmt);
    }
}

static void codegen_stmt_list(CodegenCtx *ctx, StmtList *list) {
    for (StmtList *sl = list; sl; sl = sl->next) {
        if (ctx->has_terminator) break;
        codegen_stmt(ctx, sl->stmt);
    }
}

static void codegen_stmt(CodegenCtx *ctx, Stmt *stmt) {
    if (!stmt || ctx->has_terminator) return;

    switch (stmt->type) {

    case STMT_EXPR:
        if (stmt->data.expr_stmt.expr)
            codegen_expr(ctx, stmt->data.expr_stmt.expr);
        break;

    case STMT_RETURN:
        if (stmt->data.return_stmt.value) {
            if (stmt->data.return_stmt.value->type == EXPR_ARRAY_LIT &&
                (LLVMGetTypeKind(ctx->current_func_return_type) == LLVMStructTypeKind ||
                 LLVMGetTypeKind(ctx->current_func_return_type) == LLVMArrayTypeKind)) {
                LLVMValueRef ret_val = codegen_initializer_value(ctx,
                    ctx->current_func_return_type, stmt->data.return_stmt.value);
                if (ret_val)
                    LLVMBuildRet(ctx->builder, ret_val);
                ctx->has_terminator = 1;
                break;
            }
            TypedValue val = codegen_expr(ctx, stmt->data.return_stmt.value);
            if (val.val) {
                TypedValue loaded = typed_load(ctx, val);
                if (LLVMGetTypeKind(ctx->current_func_return_type) == LLVMVoidTypeKind)
                    LLVMBuildRetVoid(ctx->builder);
                else {
                    LLVMValueRef ret_val = loaded.val;
                    LLVMTypeRef ret_ty = ctx->current_func_return_type;
                    LLVMTypeRef val_ty = LLVMTypeOf(loaded.val);
                    if (LLVMGetTypeKind(ret_ty) == LLVMIntegerTypeKind &&
                        LLVMGetTypeKind(val_ty) == LLVMIntegerTypeKind) {
                        unsigned ret_w = LLVMGetIntTypeWidth(ret_ty);
                        unsigned val_w = LLVMGetIntTypeWidth(val_ty);
                        if (ret_w < val_w)
                            ret_val = LLVMBuildTrunc(ctx->builder, ret_val, ret_ty, "");
                        else if (ret_w > val_w) {
                            if (val_w == 1)
                                ret_val = LLVMBuildZExt(ctx->builder, ret_val, ret_ty, "");
                            else
                                ret_val = LLVMBuildSExt(ctx->builder, ret_val, ret_ty, "");
                        }
                    } else if (LLVMGetTypeKind(ret_ty) == LLVMDoubleTypeKind &&
                               LLVMGetTypeKind(val_ty) == LLVMFloatTypeKind) {
                        ret_val = LLVMBuildFPExt(ctx->builder, ret_val, ret_ty, "");
                    } else if (LLVMGetTypeKind(ret_ty) == LLVMFloatTypeKind &&
                               LLVMGetTypeKind(val_ty) == LLVMDoubleTypeKind) {
                        ret_val = LLVMBuildFPTrunc(ctx->builder, ret_val, ret_ty, "");
                    } else if (LLVMGetTypeKind(ret_ty) == LLVMPointerTypeKind) {
                        ret_val = coerce_to_declared_interface_pointer(
                            ctx, ret_val,
                            ctx->current_func_return_source_type,
                            ctx->current_func_return_is_ptr,
                            val, 1);
                        ret_val = coerce_to_interface_pointer(ctx, ret_val, ret_ty, val);
                        ret_val = coerce_value(ctx, ret_val, ret_ty);
                    } else {
                        ret_val = coerce_value(ctx, ret_val, ret_ty);
                    }
                    LLVMBuildRet(ctx->builder, ret_val);
                }
            }
        } else {
            LLVMBuildRetVoid(ctx->builder);
        }
        ctx->has_terminator = 1;
        break;

    case STMT_BLOCK:
        codegen_block(ctx, stmt);
        break;

    case STMT_VAR_DECL: {
        const char *name = stmt->data.var_decl.name;
        LLVMTypeRef alloca_type = resolve_type_full(ctx, stmt->data.var_decl.type_name,
                                                    stmt->data.var_decl.is_ptr);
        if (!alloca_type) return;

        LLVMTypeRef pointee_type = NULL;
        {
            const char *tn = stmt->data.var_decl.type_name;
            size_t tlen = tn ? strlen(tn) : 0;
            if (tlen > 0 && tn[tlen - 1] == '*') {
                char *base = malloc(tlen);
                memcpy(base, tn, tlen - 1);
                base[tlen - 1] = '\0';
                pointee_type = resolve_type(ctx, base);
                free(base);
                if (!pointee_type || LLVMGetTypeKind(pointee_type) == LLVMVoidTypeKind)
                    pointee_type = llvm_i8(ctx);
            } else if (tn && strcmp(tn, "str") == 0) {
                pointee_type = llvm_i8(ctx);
            } else {
                pointee_type = resolve_type(ctx, tn);
            }
        }
        if (!pointee_type) pointee_type = alloca_type;

        if (stmt->data.var_decl.array_size > 0) {
            LLVMTypeRef elem_type = resolve_type_full(ctx, stmt->data.var_decl.type_name,
                                                      stmt->data.var_decl.is_ptr);
            if (!elem_type) elem_type = llvm_i8(ctx);
            alloca_type = LLVMArrayType(elem_type, (unsigned)stmt->data.var_decl.array_size);
            pointee_type = elem_type;
        }

        LLVMBuilderRef tmp = LLVMCreateBuilderInContext(ctx->context);
        LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(ctx->current_func);
        LLVMValueRef first_inst = LLVMGetFirstInstruction(entry);
        if (first_inst)
            LLVMPositionBuilderBefore(tmp, first_inst);
        else
            LLVMPositionBuilderAtEnd(tmp, entry);
        LLVMValueRef alloca = LLVMBuildAlloca(tmp, alloca_type, name);
        LLVMDisposeBuilder(tmp);

        var_info_add(ctx, name, alloca_type, pointee_type,
                     stmt->data.var_decl.type_name, stmt->data.var_decl.is_ptr, alloca);

        if (stmt->data.var_decl.init && stmt->data.var_decl.array_size > 0) {
            codegen_array_initializer_store(ctx, alloca, alloca_type, pointee_type,
                                            stmt->data.var_decl.init,
                                            stmt->data.var_decl.array_size);
        } else if (stmt->data.var_decl.init &&
                   stmt->data.var_decl.init->type == EXPR_ARRAY_LIT &&
                   LLVMGetTypeKind(alloca_type) == LLVMStructTypeKind) {
            codegen_struct_initializer_store(ctx, alloca, alloca_type,
                                             stmt->data.var_decl.init);
        } else if (stmt->data.var_decl.init) {
            TypedValue init_val = codegen_expr(ctx, stmt->data.var_decl.init);
            if (init_val.val) {
                TypedValue loaded = typed_load(ctx, init_val);
                LLVMValueRef coerced = loaded.val;
                coerced = coerce_to_declared_interface_pointer(ctx, coerced,
                                                               stmt->data.var_decl.type_name,
                                                               stmt->data.var_decl.is_ptr,
                                                               init_val, 0);
                coerced = coerce_value(ctx, coerced, alloca_type);
                LLVMBuildStore(ctx->builder, coerced, alloca);
            }
        }
        break;
    }

    case STMT_IF: {
        TypedValue cond = stmt->data.if_stmt.condition
            ? codegen_expr(ctx, stmt->data.if_stmt.condition) : tv_null();
        if (!cond.val) return;
        TypedValue cond_loaded = typed_load(ctx, cond);

        LLVMValueRef cond_val = ensure_i1(ctx, cond_loaded.val);

        LLVMBasicBlockRef then_bb  = make_bb(ctx->context, ctx->current_func, "if.then");
        LLVMBasicBlockRef merge_bb = make_bb(ctx->context, ctx->current_func, "if.end");

        LLVMBasicBlockRef else_bb = NULL;
        if (stmt->data.if_stmt.elifs || stmt->data.if_stmt.else_block)
            else_bb = make_bb(ctx->context, ctx->current_func, "if.else");

        LLVMBuildCondBr(ctx->builder, cond_val, then_bb, else_bb ? else_bb : merge_bb);

        LLVMPositionBuilderAtEnd(ctx->builder, then_bb);
        codegen_block(ctx, stmt->data.if_stmt.then_block);
        if (!ctx->has_terminator)
            LLVMBuildBr(ctx->builder, merge_bb);
        ctx->has_terminator = 0;

        LLVMBasicBlockRef prev_else = else_bb ? else_bb : merge_bb;
        for (ElifClause *ec = stmt->data.if_stmt.elifs; ec; ec = ec->next) {
            LLVMBasicBlockRef elif_then = make_bb(ctx->context, ctx->current_func, "elif.then");
            LLVMBasicBlockRef elif_next = make_bb(ctx->context, ctx->current_func, "elif.next");

            LLVMPositionBuilderAtEnd(ctx->builder, prev_else);
            TypedValue ec_cond = codegen_expr(ctx, ec->condition);
            TypedValue ec_loaded = typed_load(ctx, ec_cond);
            if (ec_loaded.val) {
                LLVMValueRef ec_i1 = ensure_i1(ctx, ec_loaded.val);
                LLVMBuildCondBr(ctx->builder, ec_i1, elif_then, elif_next);
            }

            LLVMPositionBuilderAtEnd(ctx->builder, elif_then);
            codegen_block(ctx, ec->body);
            if (!ctx->has_terminator)
                LLVMBuildBr(ctx->builder, merge_bb);
            ctx->has_terminator = 0;
            prev_else = elif_next;
        }

        if (stmt->data.if_stmt.else_block) {
            LLVMPositionBuilderAtEnd(ctx->builder, prev_else);
            codegen_block(ctx, stmt->data.if_stmt.else_block);
            if (!ctx->has_terminator)
                LLVMBuildBr(ctx->builder, merge_bb);
            ctx->has_terminator = 0;
        } else if (prev_else != merge_bb) {
            LLVMPositionBuilderAtEnd(ctx->builder, prev_else);
            LLVMBuildBr(ctx->builder, merge_bb);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, merge_bb);
        break;
    }

    case STMT_SWITCH: {
        TypedValue subject = stmt->data.switch_stmt.expr
            ? codegen_expr(ctx, stmt->data.switch_stmt.expr) : tv_null();
        if (!subject.val) return;
        TypedValue subject_loaded = typed_load(ctx, subject);

        int case_count = 0;
        for (SwitchCase *sc = stmt->data.switch_stmt.cases; sc; sc = sc->next)
            case_count++;

        LLVMBasicBlockRef end_bb = make_bb(ctx->context, ctx->current_func, "switch.end");
        LLVMBasicBlockRef default_bb = stmt->data.switch_stmt.default_stmts
            ? make_bb(ctx->context, ctx->current_func, "switch.default")
            : end_bb;

        LLVMBasicBlockRef *case_bbs = NULL;
        LLVMBasicBlockRef *cmp_bbs = NULL;
        if (case_count > 0) {
            case_bbs = malloc(sizeof(LLVMBasicBlockRef) * case_count);
            cmp_bbs = malloc(sizeof(LLVMBasicBlockRef) * case_count);
            for (int i = 0; i < case_count; i++) {
                case_bbs[i] = make_bb(ctx->context, ctx->current_func, "switch.case");
                cmp_bbs[i] = make_bb(ctx->context, ctx->current_func, "switch.cmp");
            }
            LLVMBuildBr(ctx->builder, cmp_bbs[0]);
        } else {
            LLVMBuildBr(ctx->builder, default_bb);
        }

        int idx = 0;
        for (SwitchCase *sc = stmt->data.switch_stmt.cases; sc; sc = sc->next, idx++) {
            LLVMPositionBuilderAtEnd(ctx->builder, cmp_bbs[idx]);
            TypedValue case_val = codegen_expr(ctx, sc->value);
            TypedValue case_loaded = typed_load(ctx, case_val);
            TypedValue left = subject_loaded;
            TypedValue right = case_loaded;
            coerce_binary_values(ctx, &left, &right);
            LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, LLVMIntEQ,
                                             left.val, right.val, "switch.eq");
            LLVMBasicBlockRef next_bb = (idx + 1 < case_count) ? cmp_bbs[idx + 1] : default_bb;
            LLVMBuildCondBr(ctx->builder, cmp, case_bbs[idx], next_bb);
        }

        LLVMBasicBlockRef saved_break = ctx->break_block;
        ctx->break_block = end_bb;

        idx = 0;
        for (SwitchCase *sc = stmt->data.switch_stmt.cases; sc; sc = sc->next, idx++) {
            LLVMPositionBuilderAtEnd(ctx->builder, case_bbs[idx]);
            ctx->has_terminator = 0;
            codegen_stmt_list(ctx, sc->stmts);
            if (!ctx->has_terminator)
                LLVMBuildBr(ctx->builder, end_bb);
            ctx->has_terminator = 0;
        }

        if (stmt->data.switch_stmt.default_stmts) {
            LLVMPositionBuilderAtEnd(ctx->builder, default_bb);
            ctx->has_terminator = 0;
            codegen_stmt_list(ctx, stmt->data.switch_stmt.default_stmts);
            if (!ctx->has_terminator)
                LLVMBuildBr(ctx->builder, end_bb);
            ctx->has_terminator = 0;
        }

        ctx->break_block = saved_break;
        free(case_bbs);
        free(cmp_bbs);
        LLVMPositionBuilderAtEnd(ctx->builder, end_bb);
        break;
    }

    case STMT_WHILE: {
        LLVMBasicBlockRef cond_bb = make_bb(ctx->context, ctx->current_func, "while.cond");
        LLVMBasicBlockRef body_bb = make_bb(ctx->context, ctx->current_func, "while.body");
        LLVMBasicBlockRef end_bb  = make_bb(ctx->context, ctx->current_func, "while.end");

        LLVMBuildBr(ctx->builder, cond_bb);

        LLVMPositionBuilderAtEnd(ctx->builder, cond_bb);
        TypedValue cond = stmt->data.while_stmt.condition
            ? codegen_expr(ctx, stmt->data.while_stmt.condition) : tv_null();
        if (cond.val) {
            TypedValue cond_loaded = typed_load(ctx, cond);
            LLVMBuildCondBr(ctx->builder, ensure_i1(ctx, cond_loaded.val), body_bb, end_bb);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, body_bb);
        LLVMBasicBlockRef saved_break = ctx->break_block;
        LLVMBasicBlockRef saved_continue = ctx->continue_block;
        ctx->break_block = end_bb;
        ctx->continue_block = cond_bb;
        ctx->loop_depth++;
        codegen_block(ctx, stmt->data.while_stmt.body);
        ctx->loop_depth--;
        ctx->break_block = saved_break;
        ctx->continue_block = saved_continue;
        if (!ctx->has_terminator)
            LLVMBuildBr(ctx->builder, cond_bb);
        ctx->has_terminator = 0;

        LLVMPositionBuilderAtEnd(ctx->builder, end_bb);
        break;
    }

    case STMT_FOR: {
        LLVMBasicBlockRef cond_bb = make_bb(ctx->context, ctx->current_func, "for.cond");
        LLVMBasicBlockRef body_bb = make_bb(ctx->context, ctx->current_func, "for.body");
        LLVMBasicBlockRef inc_bb  = make_bb(ctx->context, ctx->current_func, "for.inc");
        LLVMBasicBlockRef end_bb  = make_bb(ctx->context, ctx->current_func, "for.end");

        if (stmt->data.for_stmt.init)
            codegen_stmt(ctx, stmt->data.for_stmt.init);

        LLVMBuildBr(ctx->builder, cond_bb);

        LLVMPositionBuilderAtEnd(ctx->builder, cond_bb);
        TypedValue cond = stmt->data.for_stmt.condition
            ? codegen_expr(ctx, stmt->data.for_stmt.condition) : tv_null();
        if (cond.val) {
            TypedValue cond_loaded = typed_load(ctx, cond);
            LLVMBuildCondBr(ctx->builder, ensure_i1(ctx, cond_loaded.val), body_bb, end_bb);
        } else
            LLVMBuildBr(ctx->builder, body_bb);

        LLVMPositionBuilderAtEnd(ctx->builder, body_bb);
        LLVMBasicBlockRef saved_break = ctx->break_block;
        LLVMBasicBlockRef saved_continue = ctx->continue_block;
        ctx->break_block = end_bb;
        ctx->continue_block = inc_bb;
        ctx->loop_depth++;
        codegen_block(ctx, stmt->data.for_stmt.body);
        ctx->loop_depth--;
        ctx->break_block = saved_break;
        ctx->continue_block = saved_continue;
        if (!ctx->has_terminator)
            LLVMBuildBr(ctx->builder, inc_bb);
        ctx->has_terminator = 0;

        LLVMPositionBuilderAtEnd(ctx->builder, inc_bb);
        if (stmt->data.for_stmt.increment)
            codegen_stmt(ctx, stmt->data.for_stmt.increment);
        if (!ctx->has_terminator)
            LLVMBuildBr(ctx->builder, cond_bb);
        ctx->has_terminator = 0;

        LLVMPositionBuilderAtEnd(ctx->builder, end_bb);
        break;
    }

    case STMT_DO_WHILE: {
        LLVMBasicBlockRef body_bb = make_bb(ctx->context, ctx->current_func, "dowhile.body");
        LLVMBasicBlockRef cond_bb = make_bb(ctx->context, ctx->current_func, "dowhile.cond");
        LLVMBasicBlockRef end_bb  = make_bb(ctx->context, ctx->current_func, "dowhile.end");

        LLVMBuildBr(ctx->builder, body_bb);

        LLVMPositionBuilderAtEnd(ctx->builder, body_bb);
        LLVMBasicBlockRef saved_break = ctx->break_block;
        LLVMBasicBlockRef saved_continue = ctx->continue_block;
        ctx->break_block = end_bb;
        ctx->continue_block = cond_bb;
        ctx->loop_depth++;
        codegen_block(ctx, stmt->data.do_while_stmt.body);
        ctx->loop_depth--;
        ctx->break_block = saved_break;
        ctx->continue_block = saved_continue;
        if (!ctx->has_terminator)
            LLVMBuildBr(ctx->builder, cond_bb);
        ctx->has_terminator = 0;

        LLVMPositionBuilderAtEnd(ctx->builder, cond_bb);
        if (stmt->data.do_while_stmt.condition) {
            TypedValue cond = codegen_expr(ctx, stmt->data.do_while_stmt.condition);
            TypedValue cond_loaded = typed_load(ctx, cond);
            if (cond_loaded.val)
                LLVMBuildCondBr(ctx->builder, ensure_i1(ctx, cond_loaded.val), body_bb, end_bb);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, end_bb);
        break;
    }

    case STMT_BREAK:
        if (ctx->break_block)
            LLVMBuildBr(ctx->builder, ctx->break_block);
        ctx->has_terminator = 1;
        break;

    case STMT_CONTINUE:
        if (ctx->continue_block) {
            LLVMBuildBr(ctx->builder, ctx->continue_block);
            ctx->has_terminator = 1;
        }
        break;
    }
}

/* ── Declaration codegen ──────────────────────────────────── */

static void codegen_struct(CodegenCtx *ctx, Decl *decl) {
    StructDecl *s = &decl->data.struct_decl;

    int field_count = 0;
    for (StructFieldList *fl = s->fields; fl; fl = fl->next) field_count++;

    LLVMTypeRef *field_types = malloc(sizeof(LLVMTypeRef) * field_count);
    LLVMTypeRef *field_element_types = malloc(sizeof(LLVMTypeRef) * field_count);
    char **field_names = malloc(sizeof(char*) * field_count);
    int idx = 0;
    for (StructFieldList *fl = s->fields; fl; fl = fl->next, idx++) {
        field_types[idx] = resolve_type_full(ctx, fl->field.type_name, fl->field.is_ptr);
        if (!field_types[idx])
            field_types[idx] = llvm_i8(ctx);
        field_element_types[idx] = NULL;
        if (fl->field.array_size > 0) {
            LLVMTypeRef elem_type = resolve_type_full(ctx, fl->field.type_name,
                                                      fl->field.is_ptr);
            if (!elem_type) elem_type = llvm_i8(ctx);
            field_types[idx] = LLVMArrayType(elem_type, (unsigned)fl->field.array_size);
            field_element_types[idx] = elem_type;
        }
        if (fl->field.array_size == 0 && fl->field.type_name) {
            size_t len = strlen(fl->field.type_name);
            if (len > 0 && fl->field.type_name[len - 1] == '*') {
                char *base = malloc(len);
                memcpy(base, fl->field.type_name, len - 1);
                base[len - 1] = '\0';
                field_element_types[idx] = resolve_type(ctx, base);
                free(base);
            }
        }
        field_names[idx] = fl->field.name;
    }

    LLVMTypeRef named = resolve_type(ctx, s->name);
    if (!named || LLVMGetTypeKind(named) != LLVMStructTypeKind) {
        named = LLVMStructCreateNamed(ctx->context, s->name);
        register_named_type(ctx, s->name, named);
    }
    LLVMStructSetBody(named, field_types, (unsigned)field_count, 0);
    register_struct_fields(ctx, s->name, field_names, field_element_types, field_count);

    free(field_types);
    free(field_element_types);
    free(field_names);

}

static void codegen_predeclare_struct(CodegenCtx *ctx, Decl *decl) {
    if (!ctx || !decl || decl->type != DECL_STRUCT) return;
    const char *name = decl->data.struct_decl.name;
    if (resolve_type(ctx, name)) return;
    LLVMTypeRef named = LLVMStructCreateNamed(ctx->context, name);
    register_named_type(ctx, name, named);
}

static void codegen_type_alias(CodegenCtx *ctx, Decl *decl) {
    TypeAliasDecl *alias = &decl->data.type_alias;
    LLVMTypeRef target = resolve_type_full(ctx, alias->target_type, 0);
    if (!target) target = llvm_i8(ctx);
    register_named_type(ctx, alias->name, target);
}

static void codegen_enum(CodegenCtx *ctx, Decl *decl) {
    EnumDecl *en = &decl->data.enum_decl;
    LLVMTypeRef enum_ty = llvm_i32(ctx);
    register_named_type(ctx, en->name, enum_ty);
    for (EnumVariantList *v = en->variants; v; v = v->next) {
        register_const(ctx, v->name,
                       LLVMConstInt(enum_ty, (unsigned long long)v->value, 0),
                       enum_ty);
    }
}

static LLVMValueRef codegen_string_constant(CodegenCtx *ctx, const char *raw) {
    const char *str = raw;
    if (str[0] == '"') str++;
    size_t slen = strlen(str);
    if (slen > 0 && str[slen - 1] == '"') slen--;
    unsigned len = (unsigned)slen;
    LLVMTypeRef arr_type = LLVMArrayType(llvm_i8(ctx), len + 1);
    LLVMValueRef backing = LLVMAddGlobal(ctx->module, arr_type, ".str");
    LLVMSetLinkage(backing, LLVMPrivateLinkage);
    LLVMSetGlobalConstant(backing, 1);
    LLVMSetInitializer(backing, LLVMConstStringInContext(ctx->context, str, len, 0));
    return LLVMConstBitCast(backing, LLVMPointerType(llvm_i8(ctx), 0));
}

static int eval_const_int_mode(CodegenCtx *ctx, Expr *expr, int unsigned_mode,
                               unsigned long long *out) {
    if (!expr || !out) return 0;
    switch (expr->type) {
    case EXPR_INT_LIT:
        *out = parse_uint_literal_value(expr->data.int_lit.value);
        return 1;
    case EXPR_CHAR_LIT:
        *out = parse_char_literal_value(expr->data.char_lit.value);
        return 1;
    case EXPR_BOOL_LIT:
        *out = expr->data.bool_lit.value ? 1 : 0;
        return 1;
    case EXPR_NULL:
        *out = 0;
        return 1;
    case EXPR_IDENTIFIER: {
        ConstInfo *ci = find_const(ctx, expr->data.identifier.name);
        if (!ci || LLVMGetTypeKind(ci->type) != LLVMIntegerTypeKind)
            return 0;
        *out = unsigned_mode
            ? LLVMConstIntGetZExtValue(ci->value)
            : (unsigned long long)LLVMConstIntGetSExtValue(ci->value);
        return 1;
    }
    case EXPR_SIZEOF: {
        LLVMTypeRef ty = resolve_sizeof_type(ctx, expr->data.sizeof_expr.type_name);
        if (!ty) return 0;
        LLVMTargetDataRef td = LLVMCreateTargetData(LLVMGetDataLayoutStr(ctx->module));
        *out = (long long)LLVMABISizeOfType(td, ty);
        LLVMDisposeTargetData(td);
        return 1;
    }
    case EXPR_UNARY: {
        unsigned long long val;
        if (!eval_const_int_mode(ctx, expr->data.unary.operand, unsigned_mode, &val))
            return 0;
        if (expr->data.unary.op == Minus) {
            *out = unsigned_mode
                ? (0ULL - val)
                : (unsigned long long)(-((long long)val));
            return 1;
        }
        if (expr->data.unary.op == Tilde) {
            *out = unsigned_mode
                ? ~val
                : (unsigned long long)(~((long long)val));
            return 1;
        }
        return 0;
    }
    case EXPR_BINARY: {
        unsigned long long left, right;
        if (!eval_const_int_mode(ctx, expr->data.binary.left, unsigned_mode, &left) ||
            !eval_const_int_mode(ctx, expr->data.binary.right, unsigned_mode, &right))
            return 0;
        long long signed_left = (long long)left;
        long long signed_right = (long long)right;
        switch (expr->data.binary.op) {
        case Plus:
            *out = unsigned_mode
                ? left + right
                : (unsigned long long)(signed_left + signed_right);
            return 1;
        case Minus:
            *out = unsigned_mode
                ? left - right
                : (unsigned long long)(signed_left - signed_right);
            return 1;
        case Star:
            *out = unsigned_mode
                ? left * right
                : (unsigned long long)(signed_left * signed_right);
            return 1;
        case Slash:
            if (right == 0) return 0;
            *out = unsigned_mode
                ? left / right
                : (unsigned long long)(signed_left / signed_right);
            return 1;
        case Percent:
            if (right == 0) return 0;
            *out = unsigned_mode
                ? left % right
                : (unsigned long long)(signed_left % signed_right);
            return 1;
        case Ampersand:  *out = left & right; return 1;
        case Pipe:       *out = left | right; return 1;
        case Caret:      *out = left ^ right; return 1;
        case LeftShift:
            *out = unsigned_mode ? left << right : (unsigned long long)(signed_left << right);
            return 1;
        case RightShift:
            *out = unsigned_mode ? left >> right : (unsigned long long)(signed_left >> right);
            return 1;
        default:         return 0;
        }
    }
    default:
        return 0;
    }
}

static int eval_const_int(CodegenCtx *ctx, Expr *expr, long long *out) {
    unsigned long long value;
    if (!eval_const_int_mode(ctx, expr, 0, &value))
        return 0;
    *out = (long long)value;
    return 1;
}

static int eval_const_float(CodegenCtx *ctx, Expr *expr, double *out) {
    if (!expr || !out) return 0;
    switch (expr->type) {
    case EXPR_FLOAT_LIT:
        *out = parse_float_literal_value(expr->data.float_lit.value);
        return 1;
    case EXPR_INT_LIT:
        *out = (double)parse_int_literal_value(expr->data.int_lit.value);
        return 1;
    case EXPR_CHAR_LIT:
        *out = (double)parse_char_literal_value(expr->data.char_lit.value);
        return 1;
    case EXPR_BOOL_LIT:
        *out = expr->data.bool_lit.value ? 1.0 : 0.0;
        return 1;
    case EXPR_IDENTIFIER: {
        ConstInfo *ci = find_const(ctx, expr->data.identifier.name);
        if (!ci) return 0;
        LLVMTypeKind kind = LLVMGetTypeKind(ci->type);
        if (kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind) {
            LLVMBool loses_info = 0;
            *out = LLVMConstRealGetDouble(ci->value, &loses_info);
            return 1;
        }
        if (kind == LLVMIntegerTypeKind) {
            *out = (double)LLVMConstIntGetSExtValue(ci->value);
            return 1;
        }
        return 0;
    }
    case EXPR_UNARY: {
        double val;
        if (expr->data.unary.op != Minus ||
            !eval_const_float(ctx, expr->data.unary.operand, &val))
            return 0;
        *out = -val;
        return 1;
    }
    case EXPR_BINARY: {
        double left, right;
        if (!eval_const_float(ctx, expr->data.binary.left, &left) ||
            !eval_const_float(ctx, expr->data.binary.right, &right))
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

static LLVMValueRef codegen_const_initializer(CodegenCtx *ctx, Expr *expr,
                                              LLVMTypeRef target_ty,
                                              const char *target_source_type) {
    if (!expr || !target_ty) return LLVMConstNull(target_ty);

    LLVMTypeKind kind = LLVMGetTypeKind(target_ty);
    int unsigned_target = source_type_is_unsigned(target_source_type);
    switch (expr->type) {
    case EXPR_INT_LIT: {
        unsigned long long val = parse_uint_literal_value(expr->data.int_lit.value);
        if (kind == LLVMIntegerTypeKind)
            return LLVMConstInt(target_ty, val, unsigned_target ? 0 : 1);
        if (kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind)
            return LLVMConstReal(target_ty, (double)((long long)val));
        break;
    }
    case EXPR_FLOAT_LIT: {
        double val = parse_float_literal_value(expr->data.float_lit.value);
        if (kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind)
            return LLVMConstReal(target_ty, val);
        break;
    }
    case EXPR_CHAR_LIT: {
        unsigned char c = parse_char_literal_value(expr->data.char_lit.value);
        if (kind == LLVMIntegerTypeKind)
            return LLVMConstInt(target_ty, (unsigned long long)c, 0);
        break;
    }
    case EXPR_BOOL_LIT:
        if (kind == LLVMIntegerTypeKind)
            return LLVMConstInt(target_ty, (unsigned long long)expr->data.bool_lit.value, 0);
        break;
    case EXPR_STRING_LIT:
        if (kind == LLVMPointerTypeKind)
            return codegen_string_constant(ctx, expr->data.string_lit.value);
        break;
    case EXPR_NULL:
        if (kind == LLVMPointerTypeKind)
            return LLVMConstNull(target_ty);
        break;
    case EXPR_SIZEOF: {
        LLVMTypeRef ty = resolve_sizeof_type(ctx, expr->data.sizeof_expr.type_name);
        if (ty && kind == LLVMIntegerTypeKind)
            return LLVMConstTruncOrBitCast(LLVMSizeOf(ty), target_ty);
        break;
    }
    case EXPR_IDENTIFIER: {
        ConstInfo *ci = find_const(ctx, expr->data.identifier.name);
        if (ci && LLVMTypeOf(ci->value) == target_ty)
            return ci->value;
        break;
    }
    case EXPR_ARRAY_LIT:
        if (kind == LLVMArrayTypeKind) {
            LLVMTypeRef elem_ty = LLVMGetElementType(target_ty);
            unsigned count = LLVMGetArrayLength(target_ty);
            LLVMValueRef *values = malloc(sizeof(LLVMValueRef) * count);
            for (unsigned i = 0; i < count; i++)
                values[i] = LLVMConstNull(elem_ty);
            unsigned idx = 0;
            for (ExprList *el = expr->data.array_lit.elements; el && idx < count;
                 el = el->next, idx++) {
                values[idx] = codegen_const_initializer(ctx, el->expr, elem_ty,
                                                        target_source_type);
            }
            LLVMValueRef result = LLVMConstArray(elem_ty, values, count);
            free(values);
            return result;
        }
        if (kind == LLVMStructTypeKind) {
            unsigned count = LLVMCountStructElementTypes(target_ty);
            LLVMValueRef *values = malloc(sizeof(LLVMValueRef) * count);
            for (unsigned i = 0; i < count; i++) {
                LLVMTypeRef field_ty = LLVMStructGetTypeAtIndex(target_ty, i);
                values[i] = LLVMConstNull(field_ty);
            }
            unsigned idx = 0;
            for (ExprList *el = expr->data.array_lit.elements; el && idx < count;
                 el = el->next, idx++) {
                LLVMTypeRef field_ty = LLVMStructGetTypeAtIndex(target_ty, idx);
                values[idx] = codegen_const_initializer(ctx, el->expr, field_ty, NULL);
            }
            LLVMValueRef result = LLVMIsPackedStruct(target_ty)
                ? LLVMConstStructInContext(ctx->context, values, count, 1)
                : LLVMConstNamedStruct(target_ty, values, count);
            free(values);
            return result;
        }
        break;
    case EXPR_UNARY:
        if (expr->data.unary.op == Minus || expr->data.unary.op == Tilde) {
            if (kind == LLVMIntegerTypeKind) {
                unsigned long long val;
                if (eval_const_int_mode(ctx, expr, unsigned_target, &val))
                    return LLVMConstInt(target_ty, val, unsigned_target ? 0 : 1);
            }
            if (kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind) {
                double val;
                if (eval_const_float(ctx, expr, &val))
                    return LLVMConstReal(target_ty, val);
            }
        }
        break;
    case EXPR_BINARY: {
        if (kind == LLVMIntegerTypeKind) {
            unsigned long long val;
            if (eval_const_int_mode(ctx, expr, unsigned_target, &val))
                return LLVMConstInt(target_ty, val, unsigned_target ? 0 : 1);
        }
        if (kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind) {
            double val;
            if (eval_const_float(ctx, expr, &val))
                return LLVMConstReal(target_ty, val);
        }
        break;
    }
    default:
        break;
    }

    return LLVMConstNull(target_ty);
}

static void codegen_interface(CodegenCtx *ctx, Decl *decl) {
    InterfaceDecl *iface = &decl->data.interface;
    if (find_interface_info(ctx, iface->name))
        return;

    LLVMTypeRef ty = LLVMStructCreateNamed(ctx->context, iface->name);
    int method_count = 0;
    for (InterfaceMethodList *ml = iface->methods; ml; ml = ml->next)
        method_count++;

    LLVMTypeRef *fields = malloc(sizeof(LLVMTypeRef) * (method_count + 1));
    fields[0] = LLVMPointerType(llvm_i8(ctx), 0);
    for (int i = 0; i < method_count; i++)
        fields[i + 1] = LLVMPointerType(llvm_i8(ctx), 0);
    LLVMStructSetBody(ty, fields, (unsigned)(method_count + 1), 0);
    free(fields);

    if (ctx->named_type_count == ctx->named_type_cap) {
        ctx->named_type_cap = ctx->named_type_cap ? ctx->named_type_cap * 2 : 16;
        ctx->named_types = realloc(ctx->named_types, sizeof(NamedType) * ctx->named_type_cap);
    }
    ctx->named_types[ctx->named_type_count].name = strdup(iface->name);
    ctx->named_types[ctx->named_type_count].type = ty;
    ctx->named_type_count++;

    if (ctx->interface_count == ctx->interface_cap) {
        ctx->interface_cap = ctx->interface_cap ? ctx->interface_cap * 2 : 16;
        ctx->interfaces = realloc(ctx->interfaces, sizeof(InterfaceTypeInfo) * ctx->interface_cap);
    }
    ctx->interfaces[ctx->interface_count].name = strdup(iface->name);
    ctx->interfaces[ctx->interface_count].methods = iface->methods;
    ctx->interfaces[ctx->interface_count].type = ty;
    ctx->interface_count++;
}

static void codegen_global_var(CodegenCtx *ctx, Decl *decl) {
    GlobalVarDecl *g = &decl->data.global_var;
    LLVMTypeRef ty = resolve_type_full(ctx, g->type_name, g->is_ptr);
    if (!ty) ty = llvm_i8(ctx);
    if (g->array_size > 0) {
        LLVMTypeRef elem_type = resolve_type_full(ctx, g->type_name, g->is_ptr);
        if (!elem_type) elem_type = llvm_i8(ctx);
        ty = LLVMArrayType(elem_type, (unsigned)g->array_size);
    }
    char *symbol_name = (!g->is_extern && !g->is_static)
        ? codegen_package_symbol_name(ctx, g->name)
        : strdup(g->name);
    register_global_type(ctx, symbol_name, g->type_name);
    if (strcmp(symbol_name, g->name) != 0)
        register_global_type(ctx, g->name, g->type_name);

    if (LLVMGetNamedGlobal(ctx->module, symbol_name)) {
        free(symbol_name);
        return;
    }

    LLVMValueRef global = LLVMAddGlobal(ctx->module, ty, symbol_name);
    if (g->is_static)
        LLVMSetLinkage(global, LLVMInternalLinkage);
    if (!g->is_extern)
        LLVMSetInitializer(global, codegen_const_initializer(ctx, g->init, ty, g->type_name));
    if (!g->is_extern && !g->is_static && strcmp(symbol_name, g->name) != 0)
        codegen_add_weak_alias(ctx, g->name, global);
    free(symbol_name);
}

static int codegen_method_has_explicit_receiver(CodegenCtx *ctx, FuncDecl *f) {
    if (!ctx->current_struct_name || !f || !f->params) return 0;
    FuncParam *p = &f->params->param;
    if (!p->name || strcmp(p->name, "self") != 0) return 0;
    if (strcmp(p->type_name, "self*") == 0) return 1;
    size_t len = strlen(ctx->current_struct_name);
    return strncmp(p->type_name, ctx->current_struct_name, len) == 0 &&
           strcmp(p->type_name + len, "*") == 0;
}

static LLVMValueRef codegen_func_signature_named(CodegenCtx *ctx, Decl *decl, const char *name) {
    FuncDecl *f = &decl->data.func;
    int internal_static = f->is_static && !ctx->current_struct_name;
    char *symbol_name = (!f->is_extern && !internal_static)
        ? codegen_package_symbol_name(ctx, name)
        : strdup(name);

    LLVMValueRef existing = LLVMGetNamedFunction(ctx->module, symbol_name);
    if (existing) {
        free(symbol_name);
        return existing;
    }

    int param_count = 0;
    for (FuncParamList *pl = f->params; pl; pl = pl->next) param_count++;
    int hidden_self = ctx->current_struct_name && !f->is_static &&
        !codegen_method_has_explicit_receiver(ctx, f);
    if (hidden_self) param_count++;
    register_function_info(ctx, symbol_name, f->params, hidden_self);
    if (strcmp(symbol_name, name) != 0)
        register_function_info(ctx, name, f->params, hidden_self);

    LLVMTypeRef *param_types = NULL;
    if (param_count > 0)
        param_types = malloc(sizeof(LLVMTypeRef) * param_count);

    int idx = 0;
    if (hidden_self) {
        LLVMTypeRef self_ty = resolve_type(ctx, ctx->current_struct_name);
        param_types[idx++] = LLVMPointerType(self_ty ? self_ty : llvm_i8(ctx), 0);
    }
    for (FuncParamList *pl = f->params; pl; pl = pl->next, idx++) {
        param_types[idx] = resolve_type_full(ctx, pl->param.type_name, pl->param.is_ptr);
        if (!param_types[idx]) param_types[idx] = llvm_i8(ctx);
    }

    LLVMTypeRef ret_type = resolve_type_full(ctx, f->return_type, 0);
    if (!ret_type) ret_type = llvm_void(ctx);

    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types,
                                              (unsigned)param_count, f->is_variadic);
    free(param_types);

    LLVMValueRef func = LLVMAddFunction(ctx->module, symbol_name, func_type);
    if (internal_static)
        LLVMSetLinkage(func, LLVMInternalLinkage);
    if (!f->is_extern && !internal_static && strcmp(symbol_name, name) != 0)
        codegen_add_weak_alias(ctx, name, func);
    free(symbol_name);
    return func;
}

static LLVMValueRef codegen_func_signature(CodegenCtx *ctx, Decl *decl) {
    return codegen_func_signature_named(ctx, decl, decl->data.func.name);
}

static void codegen_func_named(CodegenCtx *ctx, Decl *decl, const char *name) {
    FuncDecl *f = &decl->data.func;
    LLVMValueRef func = codegen_func_signature_named(ctx, decl, name);
    LLVMTypeRef func_type = LLVMGlobalGetValueType(func);
    LLVMTypeRef ret_type = LLVMGetReturnType(func_type);

    if (!f->body) return;

    ctx->current_func = func;
    ctx->current_func_return_type = ret_type;
    ctx->current_func_return_source_type = f->return_type;
    ctx->current_func_return_is_ptr = f->return_type &&
        strlen(f->return_type) > 0 &&
        f->return_type[strlen(f->return_type) - 1] == '*';

    LLVMBasicBlockRef entry = make_bb(ctx->context, func, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry);
    ctx->has_terminator = 0;

    int hidden_self = ctx->current_struct_name && !f->is_static &&
        !codegen_method_has_explicit_receiver(ctx, f);
    int idx = 0;
    if (hidden_self) {
        LLVMTypeRef self_struct_ty = resolve_type(ctx, ctx->current_struct_name);
        LLVMTypeRef self_ty = LLVMPointerType(self_struct_ty ? self_struct_ty : llvm_i8(ctx), 0);
        LLVMValueRef param = LLVMGetParam(func, 0);
        LLVMSetValueName(param, "self");

        LLVMBuilderRef tmp = LLVMCreateBuilderInContext(ctx->context);
        LLVMPositionBuilderAtEnd(tmp, entry);
        LLVMValueRef alloca = LLVMBuildAlloca(tmp, self_ty, "self");
        LLVMDisposeBuilder(tmp);
        LLVMBuildStore(ctx->builder, param, alloca);

        var_info_add(ctx, "self", self_ty, self_struct_ty ? self_struct_ty : llvm_i8(ctx),
                     "self*", 1, alloca);
        idx = 1;
    }

    for (FuncParamList *pl = f->params; pl; pl = pl->next, idx++) {
        LLVMValueRef param = LLVMGetParam(func, (unsigned)idx);
        LLVMSetValueName(param, pl->param.name);

        LLVMTypeRef ptype = resolve_type_full(ctx, pl->param.type_name, pl->param.is_ptr);
        if (!ptype) ptype = llvm_i8(ctx);

        LLVMTypeRef pointee;
        if (pl->param.is_ptr && strlen(pl->param.type_name) > 1 &&
            pl->param.type_name[strlen(pl->param.type_name) - 1] == '*') {
            char *base = strndup(pl->param.type_name, strlen(pl->param.type_name) - 1);
            pointee = resolve_type(ctx, base);
            free(base);
            if (!pointee || LLVMGetTypeKind(pointee) == LLVMVoidTypeKind)
                pointee = llvm_i8(ctx);
        } else if (strcmp(pl->param.type_name, "str") == 0) {
            pointee = llvm_i8(ctx);
        } else {
            pointee = resolve_type(ctx, pl->param.type_name);
            if (!pointee || LLVMGetTypeKind(pointee) == LLVMVoidTypeKind)
                pointee = llvm_i8(ctx);
        }
        if (!pointee) pointee = ptype;

        LLVMBuilderRef tmp = LLVMCreateBuilderInContext(ctx->context);
        LLVMPositionBuilderAtEnd(tmp, entry);
        LLVMValueRef alloca = LLVMBuildAlloca(tmp, ptype, pl->param.name);
        LLVMDisposeBuilder(tmp);
        LLVMBuildStore(ctx->builder, param, alloca);

        var_info_add(ctx, pl->param.name, ptype, pointee,
                     pl->param.type_name, pl->param.is_ptr, alloca);
    }

    codegen_block(ctx, f->body);

    if (!ctx->has_terminator) {
        if (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind)
            LLVMBuildRetVoid(ctx->builder);
        else
            LLVMBuildRet(ctx->builder, LLVMConstNull(ret_type));
    }

    ctx->current_func = NULL;
    ctx->current_func_return_type = NULL;
    ctx->current_func_return_source_type = NULL;
    ctx->current_func_return_is_ptr = 0;
}

static void codegen_func(CodegenCtx *ctx, Decl *decl) {
    codegen_func_named(ctx, decl, decl->data.func.name);
}

static void codegen_decl(CodegenCtx *ctx, Decl *decl) {
    if (!decl) return;
    switch (decl->type) {
    case DECL_STRUCT:  codegen_struct(ctx, decl); break;
    case DECL_TYPE_ALIAS: codegen_type_alias(ctx, decl); break;
    case DECL_ENUM:    codegen_enum(ctx, decl); break;
    case DECL_GLOBAL_VAR: codegen_global_var(ctx, decl); break;
    case DECL_FUNC:    codegen_func(ctx, decl);   break;
    case DECL_INTERFACE: codegen_interface(ctx, decl); break;
    case DECL_IMPORT:
        break;
    default: break;
    }
}

static void codegen_declare_funcs_in_decl(CodegenCtx *ctx, Decl *decl) {
    if (!decl) return;
    if (decl->type == DECL_FUNC) {
        codegen_func_signature(ctx, decl);
        return;
    }
    if (decl->type == DECL_STRUCT) {
        const char *saved_struct = ctx->current_struct_name;
        ctx->current_struct_name = decl->data.struct_decl.name;
        for (DeclList *ml = decl->data.struct_decl.methods; ml; ml = ml->next) {
            if (ml->decl && ml->decl->type == DECL_FUNC) {
                char full_name[256];
                snprintf(full_name, sizeof(full_name), "%s.%s",
                         decl->data.struct_decl.name, ml->decl->data.func.name);
                codegen_func_signature_named(ctx, ml->decl, full_name);
            }
        }
        ctx->current_struct_name = saved_struct;
    }
}

static void codegen_emit_funcs_in_decl(CodegenCtx *ctx, Decl *decl) {
    if (!decl) return;
    if (decl->type == DECL_FUNC) {
        codegen_func(ctx, decl);
        return;
    }
    if (decl->type == DECL_STRUCT) {
        const char *saved_struct = ctx->current_struct_name;
        ctx->current_struct_name = decl->data.struct_decl.name;
        for (DeclList *ml = decl->data.struct_decl.methods; ml; ml = ml->next) {
            if (ml->decl && ml->decl->type == DECL_FUNC) {
                char full_name[256];
                snprintf(full_name, sizeof(full_name), "%s.%s",
                         decl->data.struct_decl.name, ml->decl->data.func.name);
                codegen_func_named(ctx, ml->decl, full_name);
            }
        }
        ctx->current_struct_name = saved_struct;
    }
}

/* ── Public API ────────────────────────────────────────────── */

CodegenCtx *codegen_new(const char *module_name) {
    CodegenCtx *ctx = calloc(1, sizeof(CodegenCtx));
    ctx->context = LLVMContextCreate();
    ctx->module = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
    ctx->builder = LLVMCreateBuilderInContext(ctx->context);
    return ctx;
}

void codegen_set_package_prefix(CodegenCtx *ctx, const char *prefix) {
    if (!ctx) return;
    free(ctx->package_prefix);
    ctx->package_prefix = (prefix && *prefix) ? strdup(prefix) : NULL;
}

void codegen_free(CodegenCtx *ctx) {
    if (!ctx) return;
    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
    LLVMContextDispose(ctx->context);
    for (int i = 0; i < ctx->named_type_count; i++)
        free(ctx->named_types[i].name);
    free(ctx->named_types);
    for (int i = 0; i < ctx->const_count; i++)
        free(ctx->consts[i].name);
    free(ctx->consts);
    for (int i = 0; i < ctx->struct_field_count; i++) {
        free(ctx->struct_fields[i].struct_name);
        for (int j = 0; j < ctx->struct_fields[i].field_count; j++)
            free(ctx->struct_fields[i].field_names[j]);
        free(ctx->struct_fields[i].field_names);
        free(ctx->struct_fields[i].field_element_types);
    }
    free(ctx->struct_fields);
    for (int i = 0; i < ctx->var_info_count; i++) {
        free(ctx->var_infos[i].name);
        free(ctx->var_infos[i].source_type);
    }
    free(ctx->var_infos);
    for (int i = 0; i < ctx->global_type_count; i++) {
        free(ctx->global_types[i].name);
        free(ctx->global_types[i].source_type);
    }
    free(ctx->global_types);
    for (int i = 0; i < ctx->interface_count; i++)
        free(ctx->interfaces[i].name);
    free(ctx->interfaces);
    for (int i = 0; i < ctx->function_count; i++)
        free(ctx->functions[i].name);
    free(ctx->functions);
    free(ctx->package_prefix);
    free(ctx);
}

bool codegen_program(CodegenCtx *ctx, Program *program) {
    if (!ctx || !program) return false;
    for (DeclList *dl = program->decls; dl; dl = dl->next) {
        if (!dl->decl) continue;
        if (dl->decl->type == DECL_INTERFACE)
            codegen_decl(ctx, dl->decl);
    }
    for (DeclList *dl = program->decls; dl; dl = dl->next) {
        if (!dl->decl) continue;
        if (dl->decl->type == DECL_STRUCT)
            codegen_predeclare_struct(ctx, dl->decl);
    }
    for (DeclList *dl = program->decls; dl; dl = dl->next) {
        if (!dl->decl) continue;
        if (dl->decl->type == DECL_TYPE_ALIAS ||
            dl->decl->type == DECL_ENUM)
            codegen_decl(ctx, dl->decl);
    }
    for (DeclList *dl = program->decls; dl; dl = dl->next) {
        if (!dl->decl) continue;
        if (dl->decl->type == DECL_STRUCT)
            codegen_decl(ctx, dl->decl);
    }
    for (DeclList *dl = program->decls; dl; dl = dl->next) {
        if (!dl->decl) continue;
        if (dl->decl->type == DECL_GLOBAL_VAR)
            codegen_decl(ctx, dl->decl);
    }
    for (DeclList *dl = program->decls; dl; dl = dl->next)
        codegen_declare_funcs_in_decl(ctx, dl->decl);
    for (DeclList *dl = program->decls; dl; dl = dl->next)
        codegen_emit_funcs_in_decl(ctx, dl->decl);
    return true;
}

bool codegen_write_ir(CodegenCtx *ctx, const char *filename) {
    if (!ctx) return false;
    char *ir = LLVMPrintModuleToString(ctx->module);
    FILE *f = fopen(filename, "w");
    if (!f) { LLVMDisposeMessage(ir); return false; }
    fputs(ir, f);
    fclose(f);
    LLVMDisposeMessage(ir);
    return true;
}

bool codegen_write_object(CodegenCtx *ctx, const char *filename) {
    if (!ctx) return false;
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmPrinter();

    char *triple = LLVMGetDefaultTargetTriple();
    LLVMTargetRef target = NULL;
    char *err = NULL;

    if (LLVMGetTargetFromTriple(triple, &target, &err)) {
        LLVMDisposeMessage(err);
        LLVMDisposeMessage(triple);
        return false;
    }

    LLVMTargetMachineRef tm = LLVMCreateTargetMachine(
        target, triple, "generic", "", LLVMCodeGenLevelDefault,
        LLVMRelocDefault, LLVMCodeModelDefault);
    LLVMDisposeMessage(triple);

    if (LLVMTargetMachineEmitToFile(tm, ctx->module, (char *)filename,
                                    LLVMObjectFile, &err)) {
        fprintf(stderr, "codegen error: %s\n", err);
        LLVMDisposeMessage(err);
        LLVMDisposeTargetMachine(tm);
        return false;
    }

    LLVMDisposeTargetMachine(tm);
    return true;
}
