#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "../include/lexer/lexer.h"
#include "../include/lexer/vec_token.h"
#include "../include/parser/parser.h"
#include "../include/ast/ast.h"
#include "../include/semantic/semantic.h"
#include "../include/codegen/codegen.h"
#include "../include/import_resolver.h"


char* read_file(char* filename){
	FILE *file = fopen(filename,"rb");
 	if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }
    rewind(file);

    char *buffer = malloc(size + 1);

    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';

    fclose(file);

    return buffer;
}

static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
}

static void print_expr(Expr *e, int depth);
static void print_stmt(Stmt *s, int depth);

static void print_expr(Expr *e, int depth) {
    if (!e) return;
    switch (e->type) {
        case EXPR_INT_LIT:
            print_indent(depth); printf("IntLit(%s)\n", e->data.int_lit.value); break;
        case EXPR_FLOAT_LIT:
            print_indent(depth); printf("FloatLit(%s)\n", e->data.float_lit.value); break;
        case EXPR_CHAR_LIT:
            print_indent(depth); printf("CharLit(%s)\n", e->data.char_lit.value); break;
        case EXPR_STRING_LIT:
            print_indent(depth); printf("StringLit(%s)\n", e->data.string_lit.value); break;
        case EXPR_BOOL_LIT:
            print_indent(depth); printf("BoolLit(%s)\n", e->data.bool_lit.value ? "true" : "false"); break;
        case EXPR_NULL:
            print_indent(depth); printf("Null\n"); break;
        case EXPR_IDENTIFIER:
            print_indent(depth); printf("Ident(%s)\n", e->data.identifier.name); break;
        case EXPR_BINARY:
            print_indent(depth); printf("Binary(%d)\n", e->data.binary.op);
            print_expr(e->data.binary.left, depth + 1);
            print_expr(e->data.binary.right, depth + 1);
            break;
        case EXPR_UNARY:
            print_indent(depth); printf("Unary(%d)\n", e->data.unary.op);
            print_expr(e->data.unary.operand, depth + 1);
            break;
        case EXPR_CALL:
            print_indent(depth); printf("Call(\n");
            print_expr(e->data.call.callee, depth + 1);
            for (ExprList *a = e->data.call.args; a; a = a->next)
                print_expr(a->expr, depth + 1);
            print_indent(depth); printf(")\n");
            break;
        case EXPR_MEMBER:
            print_indent(depth); printf("Member(%s)\n", e->data.member.field);
            print_expr(e->data.member.object, depth + 1);
            break;
        case EXPR_INDEX:
            print_indent(depth); printf("Index(\n");
            print_expr(e->data.index.array, depth + 1);
            print_expr(e->data.index.index, depth + 1);
            print_indent(depth); printf(")\n");
            break;
        case EXPR_DEREF:
            print_indent(depth); printf("Deref(\n");
            print_expr(e->data.deref.operand, depth + 1);
            print_indent(depth); printf(")\n");
            break;
        case EXPR_ADDROF:
            print_indent(depth); printf("AddrOf(\n");
            print_expr(e->data.addrof.operand, depth + 1);
            print_indent(depth); printf(")\n");
            break;
        case EXPR_CAST:
            print_indent(depth); printf("Cast(%s)\n", e->data.cast.type_name);
            print_expr(e->data.cast.operand, depth + 1);
            break;
        case EXPR_ASSIGN:
            print_indent(depth); printf("Assign(%s)\n", e->data.assign.name);
            print_expr(e->data.assign.value, depth + 1);
            break;
        case EXPR_NEW:
            print_indent(depth); printf("New(%s)\n", e->data.new_expr.type_name);
            for (ExprList *dl = e->data.new_expr.dims; dl; dl = dl->next)
                print_expr(dl->expr, depth + 1);
            break;
        case EXPR_DELETE:
            print_indent(depth); printf("Delete[%d]\n", e->data.delete_expr.dim_count);
            print_expr(e->data.delete_expr.operand, depth + 1);
            break;
        case EXPR_SIZEOF:
            print_indent(depth); printf("Sizeof(%s)\n", e->data.sizeof_expr.type_name);
            break;
        case EXPR_ARRAY_LIT:
            print_indent(depth); printf("ArrayLit(\n");
            for (ExprList *el = e->data.array_lit.elements; el; el = el->next)
                print_expr(el->expr, depth + 1);
            print_indent(depth); printf(")\n");
            break;
        case EXPR_CONDITIONAL:
            print_indent(depth); printf("Conditional(\n");
            print_expr(e->data.conditional.condition, depth + 1);
            print_expr(e->data.conditional.then_expr, depth + 1);
            print_expr(e->data.conditional.else_expr, depth + 1);
            print_indent(depth); printf(")\n");
            break;
    }
}

static void print_stmt(Stmt *s, int depth) {
    if (!s) return;
    switch (s->type) {
        case STMT_EXPR:
            print_indent(depth); printf("ExprStmt:\n");
            print_expr(s->data.expr_stmt.expr, depth + 1);
            break;
        case STMT_RETURN:
            print_indent(depth); printf("Return:\n");
            print_expr(s->data.return_stmt.value, depth + 1);
            break;
        case STMT_BLOCK:
            print_indent(depth); printf("Block:\n");
            for (StmtList *sl = s->data.block.stmts; sl; sl = sl->next)
                print_stmt(sl->stmt, depth + 1);
            break;
        case STMT_VAR_DECL:
            print_indent(depth); printf("VarDecl(%s %s, ptr=%d, array=%d)\n",
                s->data.var_decl.type_name, s->data.var_decl.name,
                s->data.var_decl.is_ptr, s->data.var_decl.array_size);
            if (s->data.var_decl.init)
                print_expr(s->data.var_decl.init, depth + 1);
            break;
        case STMT_IF:
            print_indent(depth); printf("If:\n");
            print_expr(s->data.if_stmt.condition, depth + 1);
            print_stmt(s->data.if_stmt.then_block, depth + 1);
            for (ElifClause *ec = s->data.if_stmt.elifs; ec; ec = ec->next) {
                print_indent(depth + 1); printf("Elif:\n");
                print_expr(ec->condition, depth + 2);
                print_stmt(ec->body, depth + 2);
            }
            if (s->data.if_stmt.else_block) {
                print_indent(depth + 1); printf("Else:\n");
                print_stmt(s->data.if_stmt.else_block, depth + 2);
            }
            break;
        case STMT_SWITCH:
            print_indent(depth); printf("Switch:\n");
            print_expr(s->data.switch_stmt.expr, depth + 1);
            for (SwitchCase *sc = s->data.switch_stmt.cases; sc; sc = sc->next) {
                print_indent(depth + 1); printf("Case:\n");
                print_expr(sc->value, depth + 2);
                for (StmtList *sl = sc->stmts; sl; sl = sl->next)
                    print_stmt(sl->stmt, depth + 2);
            }
            if (s->data.switch_stmt.default_stmts) {
                print_indent(depth + 1); printf("Default:\n");
                for (StmtList *sl = s->data.switch_stmt.default_stmts; sl; sl = sl->next)
                    print_stmt(sl->stmt, depth + 2);
            }
            break;
        case STMT_WHILE:
            print_indent(depth); printf("While:\n");
            print_expr(s->data.while_stmt.condition, depth + 1);
            print_stmt(s->data.while_stmt.body, depth + 1);
            break;
        case STMT_FOR:
            print_indent(depth); printf("For:\n");
            print_stmt(s->data.for_stmt.init, depth + 1);
            print_expr(s->data.for_stmt.condition, depth + 1);
            print_stmt(s->data.for_stmt.increment, depth + 1);
            print_stmt(s->data.for_stmt.body, depth + 1);
            break;
        case STMT_DO_WHILE:
            print_indent(depth); printf("DoWhile:\n");
            print_stmt(s->data.do_while_stmt.body, depth + 1);
            print_expr(s->data.do_while_stmt.condition, depth + 1);
            break;
        case STMT_BREAK:
            print_indent(depth); printf("Break\n"); break;
        case STMT_CONTINUE:
            print_indent(depth); printf("Continue\n"); break;
    }
}

static void print_generic_params(GenericParamList *params, int depth) {
    for (GenericParamList *gl = params; gl; gl = gl->next) {
        print_indent(depth);
        printf("GenericParam(%s)\n", gl->param.name);
    }
}

static void print_func_params(FuncParamList *params, int depth) {
    for (FuncParamList *pl = params; pl; pl = pl->next) {
        print_indent(depth);
        printf("Param(%s %s, ptr=%d)\n",
            pl->param.type_name, pl->param.name, pl->param.is_ptr);
    }
}

static void print_struct_fields(StructFieldList *fields, int depth) {
    for (StructFieldList *fl = fields; fl; fl = fl->next) {
        print_indent(depth);
        printf("Field(%s %s, ptr=%d, array=%d)\n",
            fl->field.type_name, fl->field.name, fl->field.is_ptr,
            fl->field.array_size);
    }
}

static void print_decl(Decl *d, int depth) {
    if (!d) return;
    switch (d->type) {
        case DECL_FUNC:
            print_indent(depth);
            printf("Func(%s%s %s)\n",
                d->data.func.is_static ? "static " : "",
                d->data.func.return_type, d->data.func.name);
            if (d->data.func.generic_params)
                print_generic_params(d->data.func.generic_params, depth + 1);
            print_func_params(d->data.func.params, depth + 1);
            print_stmt(d->data.func.body, depth + 1);
            break;
        case DECL_GLOBAL_VAR:
            print_indent(depth);
            printf("GlobalVar(%s%s%s %s, array=%d)\n",
                d->data.global_var.is_static ? "static " : "",
                d->data.global_var.is_extern ? "extern " : "",
                d->data.global_var.type_name, d->data.global_var.name,
                d->data.global_var.array_size);
            if (d->data.global_var.init)
                print_expr(d->data.global_var.init, depth + 1);
            break;
        case DECL_STRUCT:
            print_indent(depth);
            printf("Struct(%s)\n", d->data.struct_decl.name);
            if (d->data.struct_decl.generic_params)
                print_generic_params(d->data.struct_decl.generic_params, depth + 1);
            print_struct_fields(d->data.struct_decl.fields, depth + 1);
            for (DeclList *ml = d->data.struct_decl.methods; ml; ml = ml->next)
                print_decl(ml->decl, depth + 1);
            break;
        case DECL_TYPE_ALIAS:
            print_indent(depth);
            printf("TypeAlias(%s = %s)\n",
                d->data.type_alias.name, d->data.type_alias.target_type);
            break;
        case DECL_ENUM:
            print_indent(depth);
            printf("Enum(%s)\n", d->data.enum_decl.name);
            for (EnumVariantList *v = d->data.enum_decl.variants; v; v = v->next) {
                print_indent(depth + 1);
                printf("Variant(%s = %d)\n", v->name, v->value);
            }
            break;
        case DECL_INTERFACE:
            print_indent(depth);
            printf("Interface(%s)\n", d->data.interface.name);
            break;
        case DECL_IMPORT:
            print_indent(depth);
            printf("Import(%s)\n", d->data.import.module_name);
            break;
        case DECL_STRUCT_FIELD:
        case DECL_INTERFACE_METHOD:
            break;
    }
}

int main(int argc, char **argv) {
    int emit_llvm = 0;
    int compile = 0;
    int link_exe = 0;
    const char *output_name = NULL;

    if(argc < 2){
        puts("abyssc:");
        puts("-f <file_1,..,file_n>");
        puts("-emit-llvm <file>   emit LLVM IR");
        puts("-c <file>           compile to .o via clang");
        puts("-link <file>        compile and link executable via clang");
        puts("-o <name>           set output file name");
        return 0;
    }

    /* Collected library paths from imports */
    char **lib_paths = NULL;
    int lib_count = 0;
    int lib_cap = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-emit-llvm") == 0) { emit_llvm = 1; continue; }
        if (strcmp(argv[i], "-c") == 0)          { compile = 1;    continue; }
        if (strcmp(argv[i], "-link") == 0 || strcmp(argv[i], "-exe") == 0) {
            link_exe = 1;
            continue;
        }
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) output_name = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-f") == 0) continue;

        char *source = read_file(argv[i]);
        if (source == NULL) {
            fprintf(stderr, "Error: cannot read file '%s'\n", argv[i]);
            continue;
        }

        Lexer *lexer = lexer_new(argv[i], source);
        Tokens *tokens = tokens_vec_new();

        while (lexer_has_next(lexer)) {
            tokens_vec_append(tokens, lexer_next_token(lexer));
        }

        Parser *parser = parser_new(tokens);
        Program *prog = parse_program(parser, argv[i]);

        if (!parser->had_error) {
            /* Resolve imports: inject declarations from package metadata */
            {
                int import_count = 0;
                for (DeclList *dl = prog->decls; dl; dl = dl->next)
                    if (dl->decl && dl->decl->type == DECL_IMPORT)
                        import_count++;
                if (import_count > 0) {
                    Decl **imports = malloc(sizeof(Decl*) * import_count);
                    int idx = 0;
                    for (DeclList *dl = prog->decls; dl; dl = dl->next)
                        if (dl->decl && dl->decl->type == DECL_IMPORT)
                            imports[idx++] = dl->decl;
                    for (int j = 0; j < import_count; j++) {
                        char *lib_path = NULL;
                        if (ir_inject_import(imports[j]->data.import.module_name, prog, imports[j], &lib_path)) {
                            fprintf(stderr, "imported '%s' from %s\n",
                                    imports[j]->data.import.module_name, lib_path);
                            /* Track library path for linking */
                            if (lib_count >= lib_cap) {
                                lib_cap = lib_cap ? lib_cap * 2 : 8;
                                lib_paths = realloc(lib_paths, sizeof(char*) * lib_cap);
                            }
                            lib_paths[lib_count++] = lib_path;
                        }
                    }
                    free(imports);
                }
            }

            SemanticAnalyzer *analyzer = semantic_analyzer_new();
            bool ok = semantic_analyze(analyzer, prog);
            if (!ok) {
                semantic_print_errors(analyzer);
            }
            semantic_analyzer_free(analyzer);

            if (ok && (emit_llvm || compile || link_exe)) {
                CodegenCtx *cg = codegen_new(argv[i]);
                if (codegen_program(cg, prog)) {
                    char llname[512];
                    snprintf(llname, sizeof(llname), "%s.ll", argv[i]);
                    if (codegen_write_ir(cg, llname))
                        fprintf(stderr, "wrote %s\n", llname);
                    else
                        fprintf(stderr, "error writing %s\n", llname);

                    if (compile || link_exe) {
                        char oname[512];
                        if (output_name) {
                            snprintf(oname, sizeof(oname), "%s", output_name);
                        } else {
                            /* Strip .as extension if present, keep directory */
                            size_t len = strlen(argv[i]);
                            if (len > 3 && strcmp(argv[i] + len - 3, ".as") == 0) {
                                snprintf(oname, sizeof(oname), "%.*s", (int)(len - 3), argv[i]);
                            } else {
                                snprintf(oname, sizeof(oname), "%s", argv[i]);
                            }
                        }

                        char cmd[4096];
                        if (compile) {
                            snprintf(cmd, sizeof(cmd), "clang -c \"%s\" -o \"%s\"", llname, oname);
                        } else {
                            char extra_objs[2048] = "";
                            char obj_dir[] = "/tmp/abyssc_import_XXXXXX";
                            char *tmpdir = mkdtemp(obj_dir);
                            for (int j = 0; j < lib_count; j++) {
                                char obj_path[1024];
                                snprintf(obj_path, sizeof(obj_path), "%s/lib_%d.o", tmpdir, j);
                                if (ir_extract_library_object(lib_paths[j], obj_path)) {
                                    char tail[2048];
                                    snprintf(tail, sizeof(tail), " \"%s\"", obj_path);
                                    strcat(extra_objs, tail);
                                }
                            }
                            snprintf(cmd, sizeof(cmd), "clang \"%s\" -o \"%s\" -lm%s",
                                     llname, oname, extra_objs);
                        }
                        fprintf(stderr, "%s\n", cmd);
                        int ret = system(cmd);
                        if (ret != 0)
                            fprintf(stderr, "clang failed with exit code %d\n", ret);
                    }
                }
                codegen_free(cg);
            } else if (ok) {
                for (DeclList *dl = prog->decls; dl; dl = dl->next) {
                    print_decl(dl->decl, 0);
                }
            }
        }

        program_free(prog);
        parser_free(parser);
        tokens_vec_free(tokens);
        free(lexer->source_code);
        free(lexer);
    }

    for (int i = 0; i < lib_count; i++) free(lib_paths[i]);
    free(lib_paths);

    return 0;
}
