#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#define INITIAL_SB_CAPACITY 100

#include <stdbool.h>
#include <string.h>
typedef struct {
	char* string;
 	size_t size;
  	size_t capacity;
}StringBuilder;

StringBuilder *string_builder_new();
void string_builder_add(StringBuilder *builder,char c);
void string_builder_add_string(StringBuilder *builder,char *str);
char* string_builder_to_string(StringBuilder *builder);
void string_builder_raise_capacity(StringBuilder *builder,size_t increment);
bool string_builder_is_full(StringBuilder *builder);
void string_builder_free(StringBuilder *builder);

#endif