#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "../ast/ast.h"
#include <stdbool.h>

typedef struct SemanticAnalyzer SemanticAnalyzer;

SemanticAnalyzer *semantic_analyzer_new(void);
void semantic_analyzer_free(SemanticAnalyzer *a);

bool semantic_analyze(SemanticAnalyzer *a, Program *program);
void semantic_print_errors(SemanticAnalyzer *a);

#endif
