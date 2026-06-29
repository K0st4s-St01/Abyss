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
    char *struct_name;
    char **field_names;
    int field_count;
} StructFieldInfo;

typedef struct {
    char *name;
    LLVMTypeRef pointee_type;
    int is_ptr;
    LLVMValueRef alloca;
} VarInfo;

typedef struct {
    LLVMValueRef val;
    LLVMTypeRef type;
    int is_ptr;
} TypedValue;

struct CodegenCtx {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;

    NamedType *named_types;
    int named_type_count;
    int named_type_cap;

    StructFieldInfo *struct_fields;
    int struct_field_count;
    int struct_field_cap;

    VarInfo *var_infos;
    int var_info_count;
    int var_info_cap;

    LLVMValueRef current_func;
    LLVMTypeRef current_func_return_type;
    int has_terminator;
    int loop_depth;
};

/* ── Helpers ───────────────────────────────────────────────── */

static TypedValue tv_null(void) {
    TypedValue r = {NULL, NULL, 0};
    return r;
}

static TypedValue tv_make(LLVMValueRef val, LLVMTypeRef type, int is_ptr) {
    TypedValue r = {val, type, is_ptr};
    return r;
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

static LLVMBasicBlockRef make_bb(LLVMContextRef ctx, LLVMValueRef func, const char *name) {
    LLVMBasicBlockRef bb = LLVMAppendBasicBlockInContext(ctx, func, name);
    return bb;
}

static int value_name_eq(LLVMValueRef v, const char *name) {
    const char *n = LLVMGetValueName(v);
    return n && strcmp(n, name) == 0;
}

/* ── Type mapping ──────────────────────────────────────────── */

static LLVMTypeRef resolve_type(CodegenCtx *ctx, const char *name) {
    if (!name) return llvm_void(ctx);

    if (strcmp(name, "void") == 0) return llvm_void(ctx);
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

/* ── Variable info tracking ────────────────────────────────── */

static void var_info_add(CodegenCtx *ctx, const char *name, LLVMTypeRef pointee_type, int is_ptr, LLVMValueRef alloca) {
    if (ctx->var_info_count == ctx->var_info_cap) {
        ctx->var_info_cap = ctx->var_info_cap ? ctx->var_info_cap * 2 : 16;
        ctx->var_infos = realloc(ctx->var_infos, sizeof(VarInfo) * ctx->var_info_cap);
    }
    ctx->var_infos[ctx->var_info_count].name = strdup(name);
    ctx->var_infos[ctx->var_info_count].pointee_type = pointee_type;
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

/* ── Struct field tracking ─────────────────────────────────── */

static void register_struct_fields(CodegenCtx *ctx, const char *struct_name,
                                    char **field_names, int field_count) {
    if (ctx->struct_field_count == ctx->struct_field_cap) {
        ctx->struct_field_cap = ctx->struct_field_cap ? ctx->struct_field_cap * 2 : 16;
        ctx->struct_fields = realloc(ctx->struct_fields,
            sizeof(StructFieldInfo) * ctx->struct_field_cap);
    }
    StructFieldInfo *sf = &ctx->struct_fields[ctx->struct_field_count];
    sf->struct_name = strdup(struct_name);
    sf->field_count = field_count;
    sf->field_names = malloc(sizeof(char*) * field_count);
    for (int i = 0; i < field_count; i++)
        sf->field_names[i] = strdup(field_names[i]);
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
    LLVMTypeRef val_ty = LLVMTypeOf(tv.val);
    if (LLVMGetTypeKind(val_ty) == LLVMPointerTypeKind) {
        LLVMValueRef loaded = LLVMBuildLoad2(ctx->builder, tv.type, tv.val, "load");
        return tv_make(loaded, tv.type, 0);
    }
    return tv;
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

/* ── Forward declarations ──────────────────────────────────── */

static void codegen_decl(CodegenCtx *ctx, Decl *decl);
static void codegen_func(CodegenCtx *ctx, Decl *decl);
static void codegen_struct(CodegenCtx *ctx, Decl *decl);
static TypedValue codegen_expr(CodegenCtx *ctx, Expr *expr);
static void codegen_stmt(CodegenCtx *ctx, Stmt *stmt);
static void codegen_block(CodegenCtx *ctx, Stmt *stmt);

/* ── Expression codegen ────────────────────────────────────── */

static TypedValue codegen_expr(CodegenCtx *ctx, Expr *expr) {
    if (!expr) return tv_null();

    switch (expr->type) {

    case EXPR_INT_LIT: {
        long long val = strtoll(expr->data.int_lit.value, NULL, 10);
        return tv_make(LLVMConstInt(llvm_i32(ctx), (unsigned long long)val, 0),
                       llvm_i32(ctx), 0);
    }

    case EXPR_FLOAT_LIT: {
        double val = strtod(expr->data.float_lit.value, NULL);
        LLVMTypeRef ty = LLVMDoubleTypeInContext(ctx->context);
        return tv_make(LLVMConstReal(ty, val), ty, 0);
    }

    case EXPR_CHAR_LIT: {
        const char *s = expr->data.char_lit.value;
        char c = (s[0] == '\'') ? s[1] : s[0];
        return tv_make(LLVMConstInt(llvm_i8(ctx), (unsigned long long)c, 0),
                       llvm_i8(ctx), 0);
    }

    case EXPR_STRING_LIT: {
        const char *str = expr->data.string_lit.value;
        unsigned len = (unsigned)strlen(str);
        LLVMTypeRef arr_type = LLVMArrayType(llvm_i8(ctx), len + 1);
        LLVMValueRef glob = LLVMAddGlobal(ctx->module, arr_type, ".str");
        LLVMSetLinkage(glob, LLVMPrivateLinkage);
        LLVMSetGlobalConstant(glob, 1);
        LLVMSetInitializer(glob, LLVMConstStringInContext(ctx->context, str, len, 0));
        LLVMTypeRef ptr_ty = LLVMPointerType(llvm_i8(ctx), 0);
        return tv_make(LLVMBuildBitCast(ctx->builder, glob, ptr_ty, ""), ptr_ty, 1);
    }

    case EXPR_IDENTIFIER: {
        const char *name = expr->data.identifier.name;

        VarInfo *vi = var_info_find(ctx, name);
        if (vi) return tv_make(vi->alloca, vi->pointee_type, vi->is_ptr);

        LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, name);
        if (global) {
            LLVMTypeRef gty = LLVMGlobalGetValueType(global);
            return tv_make(global, gty, 0);
        }

        LLVMValueRef func = LLVMGetNamedFunction(ctx->module, name);
        if (func) {
            LLVMTypeRef fty = LLVMGlobalGetValueType(func);
            return tv_make(func, fty, 0);
        }

        return tv_null();
    }

    case EXPR_BINARY: {
        TypedValue left = codegen_expr(ctx, expr->data.binary.left);
        TypedValue right = codegen_expr(ctx, expr->data.binary.right);
        if (!left.val || !right.val) return tv_null();

        switch (expr->data.binary.op) {
        case Plus: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildAdd(ctx->builder, ll.val, rr.val, "add"),
                           ll.type, 0);
        }
        case Minus: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildSub(ctx->builder, ll.val, rr.val, "sub"),
                           ll.type, 0);
        }
        case Star: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildMul(ctx->builder, ll.val, rr.val, "mul"),
                           ll.type, 0);
        }
        case Slash: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildSDiv(ctx->builder, ll.val, rr.val, "div"),
                           ll.type, 0);
        }
        case Percent: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildSRem(ctx->builder, ll.val, rr.val, "rem"),
                           ll.type, 0);
        }
        case EqualsEquals: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildICmp(ctx->builder, LLVMIntEQ, ll.val, rr.val, "eq"),
                           llvm_i1(ctx), 0);
        }
        case Less: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildICmp(ctx->builder, LLVMIntSLT, ll.val, rr.val, "slt"),
                           llvm_i1(ctx), 0);
        }
        case LessEquals: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildICmp(ctx->builder, LLVMIntSLE, ll.val, rr.val, "sle"),
                           llvm_i1(ctx), 0);
        }
        case Greater: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildICmp(ctx->builder, LLVMIntSGT, ll.val, rr.val, "sgt"),
                           llvm_i1(ctx), 0);
        }
        case GreaterEquals: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildICmp(ctx->builder, LLVMIntSGE, ll.val, rr.val, "sge"),
                           llvm_i1(ctx), 0);
        }
        case AmpersandAmpersand: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildAnd(ctx->builder, ll.val, rr.val, "and"),
                           llvm_i1(ctx), 0);
        }
        case Ampersand: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildAnd(ctx->builder, ll.val, rr.val, "band"),
                           ll.type, 0);
        }
        case Caret: {
            TypedValue ll = typed_load(ctx, left);
            TypedValue rr = typed_load(ctx, right);
            return tv_make(LLVMBuildXor(ctx->builder, ll.val, rr.val, "xor"),
                           ll.type, 0);
        }
        case Equals: {
            Expr *lhs = expr->data.binary.left;
            if (lhs->type == EXPR_IDENTIFIER) {
                VarInfo *vi = var_info_find(ctx, lhs->data.identifier.name);
                LLVMValueRef ptr = vi ? vi->alloca : NULL;
                if (!ptr) ptr = LLVMGetNamedGlobal(ctx->module, lhs->data.identifier.name);
                if (ptr) {
                    TypedValue rval = typed_load(ctx, right);
                    LLVMBuildStore(ctx->builder, rval.val, ptr);
                    return rval;
                }
            }
            if (lhs->type == EXPR_DEREF) {
                TypedValue operand = codegen_expr(ctx, lhs->data.deref.operand);
                if (operand.val) {
                    TypedValue rval = typed_load(ctx, right);
                    LLVMBuildStore(ctx->builder, rval.val, operand.val);
                    return rval;
                }
            }
            if (lhs->type == EXPR_MEMBER) {
                TypedValue obj = codegen_expr(ctx, lhs->data.member.object);
                if (obj.val && LLVMGetTypeKind(obj.type) == LLVMStructTypeKind) {
                    int idx = find_field_index(ctx, obj.type, lhs->data.member.field);
                    if (idx >= 0) {
                        LLVMValueRef indices[2] = {
                            LLVMConstInt(llvm_i32(ctx), 0, 0),
                            LLVMConstInt(llvm_i32(ctx), idx, 0)
                        };
                        LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, obj.type,
                            obj.val, indices, 2, lhs->data.member.field);
                        TypedValue rval = typed_load(ctx, right);
                        LLVMBuildStore(ctx->builder, rval.val, gep);
                        return rval;
                    }
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
            return tv_make(LLVMBuildNeg(ctx->builder, loaded.val, "neg"),
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
            return tv_make(operand.val, operand.type, 1);
        default:
            return operand;
        }
    }

    case EXPR_CALL: {
        Expr *callee = expr->data.call.callee;
        if (callee->type != EXPR_IDENTIFIER) return tv_null();

        const char *fname = callee->data.identifier.name;
        LLVMValueRef func = LLVMGetNamedFunction(ctx->module, fname);
        if (!func) return tv_null();

        int argc = 0;
        for (ExprList *al = expr->data.call.args; al; al = al->next) argc++;

        LLVMTypeRef func_ty = LLVMGlobalGetValueType(func);
        LLVMTypeRef ret_ty = LLVMGetReturnType(func_ty);

        LLVMValueRef *args = NULL;
        if (argc > 0) args = malloc(sizeof(LLVMValueRef) * argc);
        int idx = 0;
        for (ExprList *al = expr->data.call.args; al; al = al->next, idx++) {
            TypedValue a = codegen_expr(ctx, al->expr);
            TypedValue loaded = typed_load(ctx, a);
            args[idx] = loaded.val;
        }

        LLVMValueRef result;
        if (LLVMGetTypeKind(ret_ty) == LLVMVoidTypeKind) {
            result = LLVMBuildCall2(ctx->builder, func_ty, func,
                                    args, (unsigned)argc, "");
        } else {
            result = LLVMBuildCall2(ctx->builder, func_ty, func,
                                    args, (unsigned)argc, "call");
        }
        free(args);
        return tv_make(result, ret_ty, 0);
    }

    case EXPR_MEMBER: {
        TypedValue obj = codegen_expr(ctx, expr->data.member.object);
        if (!obj.val) return tv_null();

        LLVMTypeRef struct_type = obj.type;
        if (LLVMGetTypeKind(struct_type) != LLVMStructTypeKind) return tv_null();

        int idx = find_field_index(ctx, struct_type, expr->data.member.field);
        if (idx < 0) return tv_null();

        LLVMValueRef indices[2] = {
            LLVMConstInt(llvm_i32(ctx), 0, 0),
            LLVMConstInt(llvm_i32(ctx), idx, 0)
        };
        LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, struct_type,
            obj.val, indices, 2, expr->data.member.field);

        /* Get the field type */
        LLVMTypeRef *fields = malloc(sizeof(LLVMTypeRef) * (unsigned)(idx + 1));
        LLVMGetStructElementTypes(struct_type, fields);
        LLVMTypeRef field_type = fields[idx];
        free(fields);

        int field_is_ptr = (LLVMGetTypeKind(field_type) == LLVMPointerTypeKind);
        return tv_make(gep, field_type, field_is_ptr);
    }

    case EXPR_DEREF: {
        TypedValue operand = codegen_expr(ctx, expr->data.deref.operand);
        if (!operand.val) return tv_null();

        LLVMTypeRef ptr_ty = LLVMPointerType(operand.type, 0);
        LLVMValueRef derefed = LLVMBuildLoad2(ctx->builder, ptr_ty, operand.val, "deref");
        return tv_make(derefed, operand.type, 1);
    }

    case EXPR_ADDROF: {
        TypedValue operand = codegen_expr(ctx, expr->data.addrof.operand);
        if (!operand.val) return tv_null();
        return tv_make(operand.val, operand.type, 1);
    }

    case EXPR_CAST: {
        TypedValue operand = codegen_expr(ctx, expr->data.cast.operand);
        if (!operand.val) return tv_null();
        LLVMTypeRef target = resolve_type(ctx, expr->data.cast.type_name);
        if (!target) return operand;
        TypedValue loaded = typed_load(ctx, operand);
        return tv_make(LLVMBuildBitCast(ctx->builder, loaded.val, target, "cast"),
                       target, 0);
    }

    case EXPR_ASSIGN: {
        TypedValue val = codegen_expr(ctx, expr->data.assign.value);
        if (!val.val) return tv_null();
        TypedValue loaded = typed_load(ctx, val);
        LLVMValueRef ptr = find_alloca(ctx, expr->data.assign.name);
        if (!ptr) ptr = LLVMGetNamedGlobal(ctx->module, expr->data.assign.name);
        if (ptr) LLVMBuildStore(ctx->builder, loaded.val, ptr);
        return loaded;
    }

    case EXPR_INDEX:
        break;
    }

    return tv_null();
}

/* ── Statement codegen ────────────────────────────────────── */

static void codegen_block(CodegenCtx *ctx, Stmt *stmt) {
    if (!stmt || stmt->type != STMT_BLOCK) return;
    for (StmtList *sl = stmt->data.block.stmts; sl; sl = sl->next) {
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
            TypedValue val = codegen_expr(ctx, stmt->data.return_stmt.value);
            if (val.val) {
                TypedValue loaded = typed_load(ctx, val);
                if (LLVMGetTypeKind(ctx->current_func_return_type) == LLVMVoidTypeKind)
                    LLVMBuildRetVoid(ctx->builder);
                else
                    LLVMBuildRet(ctx->builder, loaded.val);
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

        LLVMTypeRef pointee_type = resolve_type(ctx, stmt->data.var_decl.type_name);
        if (!pointee_type) pointee_type = alloca_type;

        LLVMBuilderRef tmp = LLVMCreateBuilderInContext(ctx->context);
        LLVMPositionBuilderAtEnd(tmp, LLVMGetEntryBasicBlock(ctx->current_func));
        LLVMValueRef alloca = LLVMBuildAlloca(tmp, alloca_type, name);
        LLVMDisposeBuilder(tmp);

        var_info_add(ctx, name, pointee_type, stmt->data.var_decl.is_ptr, alloca);

        if (stmt->data.var_decl.init) {
            TypedValue init_val = codegen_expr(ctx, stmt->data.var_decl.init);
            if (init_val.val) {
                TypedValue loaded = typed_load(ctx, init_val);
                LLVMBuildStore(ctx->builder, loaded.val, alloca);
            }
        }
        break;
    }

    case STMT_IF: {
        TypedValue cond = stmt->data.if_stmt.condition
            ? codegen_expr(ctx, stmt->data.if_stmt.condition) : tv_null();
        if (!cond.val) return;
        TypedValue cond_loaded = typed_load(ctx, cond);

        LLVMBasicBlockRef then_bb  = make_bb(ctx->context, ctx->current_func, "if.then");
        LLVMBasicBlockRef merge_bb = make_bb(ctx->context, ctx->current_func, "if.end");

        LLVMBasicBlockRef else_bb = NULL;
        if (stmt->data.if_stmt.elifs || stmt->data.if_stmt.else_block)
            else_bb = make_bb(ctx->context, ctx->current_func, "if.else");

        LLVMBuildCondBr(ctx->builder, cond_loaded.val, then_bb, else_bb ? else_bb : merge_bb);

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
            if (ec_loaded.val)
                LLVMBuildCondBr(ctx->builder, ec_loaded.val, elif_then, elif_next);

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
            LLVMBuildCondBr(ctx->builder, cond_loaded.val, body_bb, end_bb);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, body_bb);
        ctx->loop_depth++;
        codegen_block(ctx, stmt->data.while_stmt.body);
        ctx->loop_depth--;
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
            LLVMBuildCondBr(ctx->builder, cond_loaded.val, body_bb, end_bb);
        } else
            LLVMBuildBr(ctx->builder, body_bb);

        LLVMPositionBuilderAtEnd(ctx->builder, body_bb);
        ctx->loop_depth++;
        codegen_block(ctx, stmt->data.for_stmt.body);
        ctx->loop_depth--;
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
        ctx->loop_depth++;
        codegen_block(ctx, stmt->data.do_while_stmt.body);
        ctx->loop_depth--;
        if (!ctx->has_terminator)
            LLVMBuildBr(ctx->builder, cond_bb);
        ctx->has_terminator = 0;

        LLVMPositionBuilderAtEnd(ctx->builder, cond_bb);
        if (stmt->data.do_while_stmt.condition) {
            TypedValue cond = codegen_expr(ctx, stmt->data.do_while_stmt.condition);
            TypedValue cond_loaded = typed_load(ctx, cond);
            if (cond_loaded.val) LLVMBuildCondBr(ctx->builder, cond_loaded.val, body_bb, end_bb);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, end_bb);
        break;
    }

    case STMT_BREAK:
        ctx->has_terminator = 1;
        break;

    case STMT_CONTINUE:
        break;
    }
}

/* ── Declaration codegen ──────────────────────────────────── */

static void codegen_struct(CodegenCtx *ctx, Decl *decl) {
    StructDecl *s = &decl->data.struct_decl;

    int field_count = 0;
    for (StructFieldList *fl = s->fields; fl; fl = fl->next) field_count++;

    LLVMTypeRef *field_types = malloc(sizeof(LLVMTypeRef) * field_count);
    char **field_names = malloc(sizeof(char*) * field_count);
    int idx = 0;
    for (StructFieldList *fl = s->fields; fl; fl = fl->next, idx++) {
        field_types[idx] = resolve_type_full(ctx, fl->field.type_name, fl->field.is_ptr);
        if (!field_types[idx])
            field_types[idx] = llvm_i8(ctx);
        field_names[idx] = fl->field.name;
    }

    LLVMTypeRef named = LLVMStructCreateNamed(ctx->context, s->name);
    LLVMStructSetBody(named, field_types, (unsigned)field_count, 0);
    register_named_type(ctx, s->name, named);
    register_struct_fields(ctx, s->name, field_names, field_count);

    free(field_types);
    free(field_names);

    for (DeclList *ml = s->methods; ml; ml = ml->next)
        codegen_decl(ctx, ml->decl);
}

static void codegen_func(CodegenCtx *ctx, Decl *decl) {
    FuncDecl *f = &decl->data.func;

    int param_count = 0;
    for (FuncParamList *pl = f->params; pl; pl = pl->next) param_count++;

    LLVMTypeRef *param_types = NULL;
    if (param_count > 0)
        param_types = malloc(sizeof(LLVMTypeRef) * param_count);

    int idx = 0;
    for (FuncParamList *pl = f->params; pl; pl = pl->next, idx++) {
        param_types[idx] = resolve_type_full(ctx, pl->param.type_name, pl->param.is_ptr);
        if (!param_types[idx]) param_types[idx] = llvm_i8(ctx);
    }

    LLVMTypeRef ret_type = resolve_type_full(ctx, f->return_type, 0);
    if (!ret_type) ret_type = llvm_void(ctx);

    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types,
                                              (unsigned)param_count, 0);
    free(param_types);

    LLVMValueRef func = LLVMAddFunction(ctx->module, f->name, func_type);

    if (!f->body) return;

    ctx->current_func = func;
    ctx->current_func_return_type = ret_type;

    LLVMBasicBlockRef entry = make_bb(ctx->context, func, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry);
    ctx->has_terminator = 0;

    idx = 0;
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
        } else {
            pointee = resolve_type(ctx, pl->param.type_name);
        }
        if (!pointee) pointee = ptype;

        LLVMBuilderRef tmp = LLVMCreateBuilderInContext(ctx->context);
        LLVMPositionBuilderAtEnd(tmp, entry);
        LLVMValueRef alloca = LLVMBuildAlloca(tmp, ptype, pl->param.name);
        LLVMDisposeBuilder(tmp);
        LLVMBuildStore(ctx->builder, param, alloca);

        var_info_add(ctx, pl->param.name, pointee, pl->param.is_ptr, alloca);
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
}

static void codegen_decl(CodegenCtx *ctx, Decl *decl) {
    if (!decl) return;
    switch (decl->type) {
    case DECL_STRUCT:  codegen_struct(ctx, decl); break;
    case DECL_FUNC:    codegen_func(ctx, decl);   break;
    case DECL_INTERFACE:
    case DECL_IMPORT:
        break;
    default: break;
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

void codegen_free(CodegenCtx *ctx) {
    if (!ctx) return;
    LLVMDisposeBuilder(ctx->builder);
    LLVMDisposeModule(ctx->module);
    LLVMContextDispose(ctx->context);
    for (int i = 0; i < ctx->named_type_count; i++)
        free(ctx->named_types[i].name);
    free(ctx->named_types);
    for (int i = 0; i < ctx->struct_field_count; i++) {
        free(ctx->struct_fields[i].struct_name);
        for (int j = 0; j < ctx->struct_fields[i].field_count; j++)
            free(ctx->struct_fields[i].field_names[j]);
        free(ctx->struct_fields[i].field_names);
    }
    free(ctx->struct_fields);
    for (int i = 0; i < ctx->var_info_count; i++)
        free(ctx->var_infos[i].name);
    free(ctx->var_infos);
    free(ctx);
}

bool codegen_program(CodegenCtx *ctx, Program *program) {
    if (!ctx || !program) return false;
    for (DeclList *dl = program->decls; dl; dl = dl->next)
        codegen_decl(ctx, dl->decl);
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
