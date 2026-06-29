#ifndef TOKENS_VEC_H
#define TOKENS_VEC_H
#include "token.h"
#include <stdlib.h>

#define INITIAL_TOKEN_VECTOR_CAPACITY 256

typedef struct{
	Token** toks;
	size_t size;
 	size_t capacity;
}Tokens;

Tokens *tokens_vec_new();
void tokens_vec_append(Tokens* tokens,Token* token);
void tokens_vec_free(Tokens* tokens);
void tokens_vec_clear(Tokens* tokens);
void tokens_vec_print(Tokens* tokens);

#endif