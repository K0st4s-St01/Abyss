#include "../../include/parser/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Helpers ────────────────────────────────────────────────── */

static Token *peek(Parser *p) {
    if (p->pos >= p->tokens->size) return NULL;
    return p->tokens->toks[p->pos];
}

static Token *advance(Parser *p) {
    Token *t = peek(p);
    if (t) p->pos++;
    return t;
}

static Token *expect(Parser *p, TokenType type) {
    Token *t = peek(p);
    if (!t || t->type != type) {
        p->had_error = true;
        p->panic_mode = true;
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "expected token type %d, got %d at %s:%d:%d",
                 type, t ? t->type : -1,
                 t ? t->loc.filename : "?",
                 t ? t->loc.line : 0, t ? t->loc.col : 0);
        return NULL;
    }
    return advance(p);
}

static bool check(Parser *p, TokenType type) {
    Token *t = peek(p);
    return t && t->type == type;
}

static bool match(Parser *p, TokenType type) {
    if (check(p, type)) {
        advance(p);
        return true;
    }
    return false;
}

static SourceLocation loc(Parser *p) {
    Token *t = peek(p);
    if (t) return t->loc;
    SourceLocation l = {0, 0, 0};
    return l;
}

static void parser_error(Parser *p, const char *msg) {
    if (p->panic_mode) return;
    p->had_error = true;
    p->panic_mode = true;
    Token *t = peek(p);
    snprintf(p->error_msg, sizeof(p->error_msg), "%s at %s:%d:%d",
             msg,
             t ? t->loc.filename : "?",
             t ? t->loc.line : 0, t ? t->loc.col : 0);
}

static void synchronize_statement(Parser *p) {
    while (!check(p, EndOfFile)) {
        if (match(p, Semicolon)) {
            p->panic_mode = false;
            return;
        }
        Token *t = peek(p);
        if (!t || t->type == RBrace || t->type == Case || t->type == Default ||
            t->type == Return || t->type == If || t->type == Switch ||
            t->type == While || t->type == For || t->type == Do ||
            t->type == Break || t->type == Continue) {
            p->panic_mode = false;
            return;
        }
        advance(p);
    }
    p->panic_mode = false;
}

/* ── Type name parsing ──────────────────────────────────────── */

static char *parse_type_name(Parser *p);
static GenericParamList *parse_generic_params(Parser *p);

static GenericParamList *parse_generic_params(Parser *p) {
    if (!match(p, Generic)) return NULL;
    GenericParamList *params = NULL;
    do {
        Token *t = peek(p);
        if (t && (t->type == Identifier || token_is_primitive_type(t) ||
                  t->type == Void || t->type == Self)) {
            GenericParam gp = generic_param_new(t->text);
            generic_param_list_append(&params, gp);
            advance(p);
        }
    } while (match(p, Comma));
    match(p, Generic);
    return params;
}

static char *parse_type_name(Parser *p) {
    Token *t = peek(p);
    if (!t) return NULL;

    if (token_is_primitive_type(t) || t->type == Identifier ||
        t->type == Void) {
        char *name = strdup(t->text);
        advance(p);
        if (check(p, Generic) || (t->type == Identifier && check(p, Less))) {
            if (check(p, Less)) advance(p);
            GenericParamList *args = parse_generic_params(p);
            GenericParamList *cur = args;
            while (cur) {
                size_t len = strlen(name);
                name = realloc(name, len + 1 + strlen(cur->param.name) + 1 + 1);
                name[len] = '<';
                memcpy(name + len + 1, cur->param.name, strlen(cur->param.name));
                len += 1 + strlen(cur->param.name);
                name[len] = '>';
                name[len + 1] = '\0';
                cur = cur->next;
            }
            generic_param_list_free(args);
        }
        while (match(p, Star)) {
            size_t len = strlen(name);
            name = realloc(name, len + 2);
            name[len] = '*';
            name[len + 1] = '\0';
        }
        return name;
    }

    parser_error(p, "expected type name");
    return NULL;
}

/* ── Forward declarations ───────────────────────────────────── */

static Expr *parse_expr(Parser *p);
static Stmt *parse_statement(Parser *p);
static Stmt *parse_block(Parser *p);
static FuncParamList *parse_param_list(Parser *p, int *is_variadic);

/* ── Expression parser (precedence climbing) ────────────────── */

static int get_binary_precedence(TokenType op) {
    switch (op) {
        case Equals:                          return 1;
        case PlusEquals: case MinusEquals:
        case StarEquals: case SlashEquals:
        case PercentEquals:
        case AmpersandEquals:
        case PipeEquals:
        case CaretEquals:                     return 1;
        case PipePipe:                        return 2;
        case Pipe:                            return 3;
        case Caret:                           return 4;
        case AmpersandAmpersand:              return 5;
        case Ampersand:                       return 6;
        case BangEquals: case EqualsEquals:   return 10;
        case Less: case LessEquals:
        case Greater: case GreaterEquals:     return 11;
        case LeftShift: case RightShift:      return 12;
        case Plus: case Minus:                return 13;
        case Star: case Slash: case Percent:  return 14;
        case Tilde:                           return 15;
        case Dot: case Arrow:                 return 16;
        case LBracket: case LParen:            return 17;
        default:                              return -1;
    }
}

static bool is_assignment_op(TokenType op) {
    return op == Equals || op == PlusEquals || op == MinusEquals ||
           op == StarEquals || op == SlashEquals || op == AmpersandEquals ||
           op == PercentEquals || op == PipeEquals || op == CaretEquals;
}

static Expr *parse_primary(Parser *p) {
    Token *t = peek(p);
    if (!t) { parser_error(p, "unexpected end of input"); return NULL; }

    if (t->type == IntLiteral) {
        advance(p);
        return expr_new_int(t->text, t->loc);
    }
    if (t->type == FloatLiteral) {
        advance(p);
        return expr_new_float(t->text, t->loc);
    }
    if (t->type == CharLiteral) {
        advance(p);
        return expr_new_char(t->text, t->loc);
    }
    if (t->type == StringLiteral) {
        advance(p);
        return expr_new_string(t->text, t->loc);
    }

    if (t->type == Null) {
        advance(p);
        return expr_new_null(t->loc);
    }

    if (t->type == Identifier || t->type == Self) {
        advance(p);
        if (check(p, LParen)) {
            advance(p);
            ExprList *args = NULL;
            if (!check(p, RParen)) {
                do {
                    Expr *arg = parse_expr(p);
                    if (arg) expr_list_append(&args, arg);
                } while (match(p, Comma));
            }
            expect(p, RParen);
            Expr *callee = expr_new_identifier(t->text, t->loc);
            return expr_new_call(callee, args, t->loc);
        }
        return expr_new_identifier(t->text, t->loc);
    }

    if (t->type == LParen) {
        advance(p);
        Expr *e = parse_expr(p);
        expect(p, RParen);
        return e;
    }

    if (t->type == Minus || t->type == Star || t->type == Ampersand || t->type == Bang || t->type == Tilde) {
        TokenType op = t->type;
        advance(p);
        Expr *operand = parse_primary(p);
        if (op == Star) return expr_new_deref(operand, t->loc);
        if (op == Ampersand) return expr_new_addrof(operand, t->loc);
        return expr_new_unary(op, operand, t->loc);
    }

    if (t->type == New) {
        advance(p);
        char *type_name = parse_type_name(p);
        if (!type_name) return NULL;
        
        ExprList *dims = NULL;
        while (check(p, LBracket)) {
            advance(p);
            Expr *dim = parse_expr(p);
            expect(p, RBracket);
            expr_list_append(&dims, dim);
        }
        Expr *e = expr_new_new(type_name, dims, t->loc);
        free(type_name);
        return e;
    }

    if (t->type == Delete) {
        advance(p);
        int dim_count = 0;
        while (check(p, LBracket)) {
            advance(p);
            expect(p, RBracket);
            dim_count++;
        }
        Expr *operand = parse_primary(p);
        return expr_new_delete(operand, dim_count, t->loc);
    }

    parser_error(p, "unexpected token in expression");
    advance(p);
    return NULL;
}

static Expr *parse_expr_precedence(Parser *p, int min_prec) {
    Expr *left = parse_primary(p);
    if (!left) return NULL;

    for (;;) {
        Token *t = peek(p);
        if (!t) break;

        int prec = get_binary_precedence(t->type);
        if (prec < min_prec) break;

        if (is_assignment_op(t->type)) {
            TokenType op = t->type;
            advance(p);
            Expr *right = parse_expr_precedence(p, 1);
            if (op == Equals) {
                if (left->type == EXPR_IDENTIFIER) {
                    Expr *a = expr_new_assign(left->data.identifier.name, right, t->loc);
                    expr_free(left);
                    left = a;
                } else if (left->type == EXPR_DEREF) {
                    left = expr_new_binary(left, op, right, t->loc);
                } else {
                    left = expr_new_binary(left, op, right, t->loc);
                }
            } else {
                left = expr_new_binary(left, op, right, t->loc);
            }
            continue;
        }

        if (t->type == Dot || t->type == Arrow) {
            advance(p);
            Token *field = expect(p, Identifier);
            if (t->type == Arrow) {
                left = expr_new_deref(left, t->loc);
            }
            if (field) {
                left = expr_new_member(left, field->text, field->loc);
            }
            continue;
        }

        if (t->type == LParen) {
            advance(p);
            ExprList *args = NULL;
            if (!check(p, RParen)) {
                do {
                    Expr *arg = parse_expr(p);
                    if (arg) expr_list_append(&args, arg);
                } while (match(p, Comma));
            }
            expect(p, RParen);
            left = expr_new_call(left, args, t->loc);
            continue;
        }

        if (t->type == LBracket) {
            advance(p);
            Expr *index = parse_expr(p);
            expect(p, RBracket);
            left = expr_new_index(left, index, t->loc);
            continue;
        }

        advance(p);
        Expr *right = parse_expr_precedence(p, prec + 1);
        left = expr_new_binary(left, t->type, right, t->loc);
    }

    return left;
}

static Expr *parse_expr(Parser *p) {
    return parse_expr_precedence(p, 1);
}

/* ── Statement parser ───────────────────────────────────────── */

static Stmt *parse_return_stmt(Parser *p) {
    Token *t = advance(p);
    Expr *val = NULL;
    if (!check(p, Semicolon)) {
        val = parse_expr(p);
    }
    expect(p, Semicolon);
    return stmt_new_return(val, t->loc);
}

static Stmt *parse_if_stmt(Parser *p) {
    Token *t = advance(p);
    expect(p, LParen);
    Expr *cond = parse_expr(p);
    expect(p, RParen);
    Stmt *then_b = parse_block(p);

    ElifClause *elifs = NULL;
    ElifClause *elif_tail = NULL;
    while (match(p, Elif)) {
        expect(p, LParen);
        Expr *econd = parse_expr(p);
        expect(p, RParen);
        Stmt *ebody = parse_block(p);
        ElifClause *ec = elif_clause_new(econd, ebody);
        if (!elifs) { elifs = ec; elif_tail = ec; }
        else { elif_tail->next = ec; elif_tail = ec; }
    }

    Stmt *else_b = NULL;
    if (match(p, Else)) {
        if (check(p, If)) {
            Stmt *nested_if = parse_if_stmt(p);
            StmtList *stmts = NULL;
            stmt_list_append(&stmts, nested_if);
            else_b = stmt_new_block(stmts, nested_if ? nested_if->loc : t->loc);
        } else {
            else_b = parse_block(p);
        }
    }

    return stmt_new_if(cond, then_b, elifs, else_b, t->loc);
}

static StmtList *parse_switch_stmt_list(Parser *p) {
    StmtList *stmts = NULL;
    while (!check(p, Case) && !check(p, Default) &&
           !check(p, RBrace) && !check(p, EndOfFile)) {
        if (p->panic_mode) {
            synchronize_statement(p);
            if (check(p, Case) || check(p, Default) ||
                check(p, RBrace) || check(p, EndOfFile)) break;
        }
        Stmt *s = parse_statement(p);
        if (s) stmt_list_append(&stmts, s);
    }
    return stmts;
}

static Stmt *parse_switch_stmt(Parser *p) {
    Token *t = advance(p);
    expect(p, LParen);
    Expr *expr = parse_expr(p);
    expect(p, RParen);
    expect(p, LBrace);

    SwitchCase *cases = NULL;
    SwitchCase *case_tail = NULL;
    StmtList *default_stmts = NULL;

    while (!check(p, RBrace) && !check(p, EndOfFile)) {
        if (p->panic_mode) {
            synchronize_statement(p);
            if (check(p, RBrace) || check(p, EndOfFile)) break;
        }
        if (match(p, Case)) {
            Expr *value = parse_expr(p);
            expect(p, Scope);
            StmtList *stmts = parse_switch_stmt_list(p);
            SwitchCase *sc = switch_case_new(value, stmts);
            if (!cases) { cases = sc; case_tail = sc; }
            else { case_tail->next = sc; case_tail = sc; }
            continue;
        }

        if (match(p, Default)) {
            if (default_stmts) {
                parser_error(p, "duplicate default in switch");
                break;
            }
            expect(p, Scope);
            default_stmts = parse_switch_stmt_list(p);
            continue;
        }

        parser_error(p, "expected case or default in switch");
        break;
    }

    expect(p, RBrace);
    return stmt_new_switch(expr, cases, default_stmts, t->loc);
}

static Stmt *parse_while_stmt(Parser *p) {
    Token *t = advance(p);
    expect(p, LParen);
    Expr *cond = parse_expr(p);
    expect(p, RParen);
    Stmt *body = parse_statement(p);
    return stmt_new_while(cond, body, t->loc);
}

static Stmt *parse_for_stmt(Parser *p) {
    Token *t = advance(p);
    expect(p, LParen);

    Stmt *init = NULL;
    if (!check(p, Semicolon)) {
        Token *tt = peek(p);
        if (token_is_primitive_type(tt) || tt->type == Identifier) {
            Token *saved = tt;
            if (token_is_primitive_type(tt)) {
                char *ty = parse_type_name(p);
                Token *name = expect(p, Identifier);
                Expr *val = NULL;
                if (match(p, Equals)) val = parse_expr(p);
                expect(p, Semicolon);
                init = stmt_new_var_decl(ty, name ? name->text : "", val, 0, saved->loc);
                free(ty);
            } else {
                init = stmt_new_expr(parse_expr(p), tt->loc);
                expect(p, Semicolon);
            }
        } else {
            init = stmt_new_expr(parse_expr(p), tt->loc);
            expect(p, Semicolon);
        }
    } else {
        advance(p);
    }

    Expr *cond = NULL;
    if (!check(p, Semicolon)) cond = parse_expr(p);
    expect(p, Semicolon);

    Stmt *incr = NULL;
    if (!check(p, RParen)) {
        incr = stmt_new_expr(parse_expr(p), loc(p));
    }
    expect(p, RParen);

    Stmt *body = parse_statement(p);
    return stmt_new_for(init, cond, incr, body, t->loc);
}

static Stmt *parse_do_while_stmt(Parser *p) {
    Token *t = advance(p);
    Stmt *body = parse_statement(p);
    expect(p, While);
    expect(p, LParen);
    Expr *cond = parse_expr(p);
    expect(p, RParen);
    expect(p, Semicolon);
    return stmt_new_do_while(body, cond, t->loc);
}

static Stmt *parse_block(Parser *p) {
    Token *t = expect(p, LBrace);
    StmtList *stmts = NULL;
    while (!check(p, RBrace) && !check(p, EndOfFile)) {
        if (p->panic_mode) {
            synchronize_statement(p);
            if (check(p, RBrace) || check(p, EndOfFile)) break;
        }
        Stmt *s = parse_statement(p);
        if (s) stmt_list_append(&stmts, s);
    }
    expect(p, RBrace);
    return stmt_new_block(stmts, t->loc);
}

static bool is_type_start(Token *t) {
    if (!t) return false;
    return token_is_primitive_type(t) || t->type == Void ||
           t->type == Identifier;
}

static void synchronize_top_level(Parser *p) {
    while (!check(p, EndOfFile)) {
        Token *t = peek(p);
        if (!t || t->type == Import || t->type == Type || t->type == Enum ||
            t->type == Struct || t->type == Interface ||
            t->type == Extern || is_type_start(t)) {
            p->panic_mode = false;
            return;
        }
        advance(p);
    }
    p->panic_mode = false;
}

static Stmt *parse_var_decl(Parser *p) {
    Token *t = peek(p);
    char *ty = parse_type_name(p);
    Token *name = expect(p, Identifier);
    Expr *init = NULL;
    if (match(p, Equals)) {
        init = parse_expr(p);
    }
    expect(p, Semicolon);
    int is_ptr = (strchr(ty, '*') != NULL);
    Stmt *s = stmt_new_var_decl(ty, name ? name->text : "", init, is_ptr, t->loc);
    free(ty);
    return s;
}

static Stmt *parse_statement(Parser *p) {
    Token *t = peek(p);
    if (!t) return NULL;

    switch (t->type) {
        case Return:   return parse_return_stmt(p);
        case If:       return parse_if_stmt(p);
        case Switch:   return parse_switch_stmt(p);
        case While:    return parse_while_stmt(p);
        case For:      return parse_for_stmt(p);
        case Do:       return parse_do_while_stmt(p);
        case Break:    advance(p); expect(p, Semicolon);
                       return stmt_new_break(t->loc);
        case Continue: advance(p); expect(p, Semicolon);
                       return stmt_new_continue(t->loc);
        case LBrace:   return parse_block(p);
        default:       break;
    }

    if (is_type_start(t) && p->pos + 1 < p->tokens->size) {
        Token *next = p->tokens->toks[p->pos + 1];
        if (next && (next->type == Identifier || next->type == Generic || next->type == Star)) {
            return parse_var_decl(p);
        }
    }

    Expr *e = parse_expr(p);
    match(p, Semicolon);
    return stmt_new_expr(e, t->loc);
}

/* ── Declaration parser ─────────────────────────────────────── */

static FuncParamList *parse_param_list(Parser *p, int *is_variadic) {
    FuncParamList *params = NULL;
    if (check(p, RParen)) return params;

    do {
        if (check(p, Ellipsis)) {
            advance(p);
            *is_variadic = 1;
            break;
        }
        if (check(p, Self)) {
            size_t saved = p->pos;
            advance(p);
            if (is_type_start(peek(p))) {
                char *ty = parse_type_name(p);
                Token *name = expect(p, Identifier);
                int is_ptr = (strchr(ty, '*') != NULL);
                FuncParam fp = func_param_new(ty, name ? name->text : "", is_ptr);
                func_param_list_append(&params, fp);
                free(ty);
            } else {
                p->pos = saved;
                char *ty = parse_type_name(p);
                Token *name = (check(p, Self) || check(p, Identifier))
                              ? advance(p) : NULL;
                int is_ptr = (strchr(ty, '*') != NULL);
                FuncParam fp = func_param_new(ty, name ? name->text : "", is_ptr);
                func_param_list_append(&params, fp);
                free(ty);
            }
        } else {
            char *ty = parse_type_name(p);
            Token *name = (check(p, Self) || check(p, Identifier))
                          ? advance(p) : NULL;
            int is_ptr = (strchr(ty, '*') != NULL);
            FuncParam fp = func_param_new(ty, name ? name->text : "", is_ptr);
            func_param_list_append(&params, fp);
            free(ty);
        }
    } while (match(p, Comma));

    return params;
}

static Decl *parse_func_decl(Parser *p, char *ret_type, char *name, GenericParamList *generic_params, int is_extern, SourceLocation loc) {
    expect(p, LParen);
    int is_variadic = 0;
    FuncParamList *params = parse_param_list(p, &is_variadic);
    expect(p, RParen);
    Stmt *body = NULL;
    if (!is_extern) {
        body = parse_block(p);
    } else {
        expect(p, Semicolon);
    }
    return decl_new_func(ret_type, name, params, generic_params, body, NULL, is_extern, is_variadic, loc);
}

static StructFieldList *parse_struct_fields(Parser *p) {
    StructFieldList *fields = NULL;
    while (!check(p, RBrace) && !check(p, EndOfFile)) {
        if (p->panic_mode) {
            synchronize_statement(p);
            if (check(p, RBrace) || check(p, EndOfFile)) break;
        }
        Token *t = peek(p);
        if (is_type_start(t) && p->pos + 1 < p->tokens->size) {
            Token *next = p->tokens->toks[p->pos + 1];
            if (next && (next->type == Identifier || next->type == Generic) && !check(p, RBrace)) {
                size_t saved = p->pos;
                char *ty = parse_type_name(p);
                Token *name = expect(p, Identifier);
                if (check(p, LParen)) {
                    p->pos = saved;
                    free(ty);
                    break;
                }
                int is_ptr = (strchr(ty, '*') != NULL);
                StructField f = struct_field_new(ty, name ? name->text : "", is_ptr);
                struct_field_list_append(&fields, f);
                free(ty);
                expect(p, Semicolon);
                continue;
            }
        }
        break;
    }
    return fields;
}

static Decl *parse_struct_decl(Parser *p) {
    Token *t = advance(p);
    Token *name = expect(p, Identifier);
    GenericParamList *generic_params = parse_generic_params(p);
    expect(p, LBrace);

    StructFieldList *fields = parse_struct_fields(p);

    DeclList *methods = NULL;
    while (!check(p, RBrace) && !check(p, EndOfFile)) {
        if (p->panic_mode) {
            synchronize_statement(p);
            if (check(p, RBrace) || check(p, EndOfFile)) break;
        }
        Token *mt = peek(p);
        if (is_type_start(mt)) {
            char *ty = parse_type_name(p);
            Token *mname = expect(p, Identifier);
            if (check(p, LParen)) {
                Decl *method = parse_func_decl(p, ty, mname->text, NULL, 0, mt->loc);
                decl_list_append(&methods, method);
            }
            free(ty);
        } else {
            parser_error(p, "unexpected token in struct body");
            advance(p);
        }
    }
    expect(p, RBrace);
    return decl_new_struct(name->text, fields, generic_params, methods, NULL, t->loc);
}

static Decl *parse_interface_decl(Parser *p) {
    Token *t = advance(p);
    Token *name = expect(p, Identifier);
    GenericParamList *generic_params = parse_generic_params(p);
    expect(p, LBrace);

    InterfaceMethodList *methods = NULL;
    while (!check(p, RBrace) && !check(p, EndOfFile)) {
        if (p->panic_mode) {
            synchronize_statement(p);
            if (check(p, RBrace) || check(p, EndOfFile)) break;
        }
        char *ret = parse_type_name(p);
        Token *mname = expect(p, Identifier);
        expect(p, LParen);
        FuncParamList *params = parse_param_list(p, NULL);
        expect(p, RParen);
        expect(p, Semicolon);
        InterfaceMethod m = interface_method_new(ret, mname->text, params);
        interface_method_list_append(&methods, m);
        free(ret);
    }
    expect(p, RBrace);
    return decl_new_interface(name->text, methods, generic_params, t->loc);
}

static Decl *parse_import_decl(Parser *p) {
    Token *t = advance(p);
    Token *mod = expect(p, Identifier);
    expect(p, Semicolon);
    return decl_new_import(mod ? mod->text : "", t->loc);
}

static Decl *parse_type_alias_decl(Parser *p) {
    Token *t = advance(p);
    Token *name = expect(p, Identifier);
    expect(p, Equals);
    char *target = parse_type_name(p);
    expect(p, Semicolon);
    Decl *decl = decl_new_type_alias(name ? name->text : "", target ? target : "", t->loc);
    free(target);
    return decl;
}

static Decl *parse_enum_decl(Parser *p) {
    Token *t = advance(p);
    Token *name = expect(p, Identifier);
    expect(p, LBrace);

    EnumVariantList *variants = NULL;
    int next_value = 0;
    while (!check(p, RBrace) && !check(p, EndOfFile)) {
        Token *variant = expect(p, Identifier);
        int value = next_value;
        if (match(p, Equals)) {
            Token *lit = expect(p, IntLiteral);
            if (lit) value = atoi(lit->text);
        }
        if (variant)
            enum_variant_list_append(&variants, enum_variant_list_new(variant->text, value));
        next_value = value + 1;
        if (!match(p, Comma))
            break;
    }

    expect(p, RBrace);
    return decl_new_enum(name ? name->text : "", variants, t->loc);
}

static Decl *parse_top_level(Parser *p) {
    Token *t = peek(p);
    if (!t) return NULL;

    if (t->type == Import)  return parse_import_decl(p);
    if (t->type == Type)    return parse_type_alias_decl(p);
    if (t->type == Enum)    return parse_enum_decl(p);
    if (t->type == Struct)  return parse_struct_decl(p);
    if (t->type == Interface) return parse_interface_decl(p);

    int is_extern = 0;
    if (t->type == Extern) {
        is_extern = 1;
        advance(p);
        t = peek(p);
    }

    if (is_type_start(t)) {
        char *ty = parse_type_name(p);
        Token *name = expect(p, Identifier);
        GenericParamList *generic_params = parse_generic_params(p);
        if (check(p, LParen)) {
            Decl *d = parse_func_decl(p, ty, name->text, generic_params, is_extern, t->loc);
            free(ty);
            return d;
        }
        generic_param_list_free(generic_params);
        free(ty);
        parser_error(p, "unexpected token after type name");
        return NULL;
    }

    parser_error(p, "unexpected token at top level");
    advance(p);
    return NULL;
}

/* ── Public API ─────────────────────────────────────────────── */

Parser *parser_new(Tokens *tokens) {
    Parser *p = malloc(sizeof(Parser));
    p->tokens = tokens;
    p->pos = 0;
    p->had_error = false;
    p->panic_mode = false;
    p->error_msg[0] = '\0';
    return p;
}

void parser_free(Parser *p) {
    free(p);
}

Program *parse_program(Parser *p, char *filename) {
    Program *prog = program_new(filename);

    while (!check(p, EndOfFile)) {
        if (p->panic_mode) {
            synchronize_top_level(p);
            if (check(p, EndOfFile)) break;
        }
        Decl *d = parse_top_level(p);
        if (d) program_add_decl(prog, d);
        if (p->panic_mode)
            synchronize_top_level(p);
    }

    if (p->had_error) {
        fprintf(stderr, "Parse error: %s\n", p->error_msg);
    }

    return prog;
}
