#include "../../include/utils/string_builder.h"
#include <stdlib.h>


StringBuilder *string_builder_new(){
	StringBuilder *sb = (StringBuilder*)malloc(sizeof(StringBuilder));
	if (!sb) return NULL;
 	sb->capacity = INITIAL_SB_CAPACITY;
  	sb->size=0;
 	sb->string = (char*)malloc(sizeof(char) * sb->capacity);
 	if (!sb->string) { free(sb); return NULL; }
  	sb->string[sb->size]='\0';
  	return sb;
}

void string_builder_add(StringBuilder *builder,char c){
	if(string_builder_is_full(builder)){
 		string_builder_raise_capacity(builder,INITIAL_SB_CAPACITY/2);
  	}
   	builder->string[builder->size] = c;
    builder->string[builder->size + 1] = '\0';
    builder->size++;
}

void string_builder_add_string(StringBuilder *builder,char *string){
	size_t length =  strlen(string);

    for(int i=0;i<length;i++){
    	if(string[i] != '\0'){
     		string_builder_add(builder,string[i]);
      	}
    }
}

char* string_builder_to_string(StringBuilder *builder){
	size_t size = strlen(builder->string) + 1;
	char *str = malloc(size*sizeof(char));
 	if(str){
  		memcpy(str,builder->string,size);
    	return str;
   	} 
    return NULL;
}

void string_builder_raise_capacity(StringBuilder *builder,size_t increment){
	char *new_str = (char*)realloc(builder->string, builder->capacity + increment);
	if (!new_str) return;
	builder->string = new_str;
 	builder->capacity+=increment;
}

bool string_builder_is_full(StringBuilder *builder){
	return builder-> size >= builder->capacity -2;
}
void string_builder_free(StringBuilder *builder){
	free(builder->string);
 	builder->string = NULL;
  	free(builder);
}


