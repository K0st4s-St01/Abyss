#ifndef CODEGEN_H
#define CODEGEN_H

#include "../ast/ast.h"
#include <stdbool.h>

typedef struct CodegenCtx CodegenCtx;

CodegenCtx *codegen_new(const char *module_name);
void codegen_free(CodegenCtx *ctx);

bool codegen_program(CodegenCtx *ctx, Program *program);
bool codegen_write_ir(CodegenCtx *ctx, const char *filename);
bool codegen_write_object(CodegenCtx *ctx, const char *filename);

#endif
