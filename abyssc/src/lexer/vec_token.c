#include "../../include/lexer/vec_token.h"
#include <stdio.h>

Tokens *tokens_vec_new(){
	Tokens* tokens = (Tokens*)malloc(sizeof(Tokens));
	if (!tokens) return NULL;
 	tokens->size = 0;
  	tokens->capacity = INITIAL_TOKEN_VECTOR_CAPACITY;
   	tokens->toks = (Token**)malloc(sizeof(Token*) * tokens->capacity);
   	if (!tokens->toks) { free(tokens); return NULL; }
    return tokens;
}

void tokens_vec_append(Tokens* tokens,Token* token){
	if(tokens->size >= tokens->capacity - 2){
 		size_t new_capacity = tokens->capacity + INITIAL_TOKEN_VECTOR_CAPACITY / 2;
 		Token** new_toks = (Token**)realloc(tokens->toks, sizeof(Token*) * new_capacity);
 		if (!new_toks) return;
 		tokens->toks = new_toks;
 		tokens->capacity = new_capacity;
  	}
   	tokens->toks[tokens->size++] = token;
}

void tokens_vec_free(Tokens* tokens){
	for(int i=0;i<tokens->size;i++){
 		token_free(tokens->toks[i]);
   		tokens->toks[i]=NULL;
  	}
   	free(tokens->toks);
    free(tokens);
}

void tokens_vec_clear(Tokens* tokens){
	for(int i=0;i<tokens->size;i++){
 		token_free(tokens->toks[i]);
   		tokens->toks[i]=NULL;
  	}
   	tokens->size = 0;
}
static void sourceLocPrint(SourceLocation* loc){
	printf("%s => %d:%d",loc->filename,loc->line,loc->col);
}
char* tokens_types[Identifier+1]={
  "EndOfFile"
  ,"Invalid "
  ,"CharLiteral"
  ,"StringLiteral"
  ,"IntLiteral"
  ,"FloatLiteral"
  ,"I8"
  ,"I16"
  ,"I32"
  ,"I64"
  ,"U8"
  ,"U16"
  ,"U32"
  ,"U64"
  ,"F32"
  ,"F64"
  ,"Char"
  ,"Str"
  ,"Void"
  ,"Bool"
  ,"If"
  ,"Else"
  ,"Elif"
  ,"Switch"
  ,"While"
  ,"For"
  ,"Do"
  ,"Static"
  ,"Struct"
  ,"Interface"
  ,"Import"
  ,"Return"
  ,"Break"
  ,"Continue"
  ,"Self"
  ,"Extern"
  ,"New"
  ,"Delete"
  ,"Generic"
  ,"Arrow"
  ,"Plus"
  ,"PlusPlus"
  ,"PlusEquals"
  ,"Minus"
  ,"MinusMinus"
  ,"MinusEquals"
  ,"Star"
  ,"StarEquals"
  ,"Slash"
  ,"SlashEquals"
  ,"Percent"
  ,"Ampersand"
  ,"AmpersandEquals"
  ,"AmpersandAmpersand"
  ,"Caret"
  ,"Less"
  ,"LessEquals"
  ,"LeftShift"
  ,"Greater"
  ,"GreaterEquals"
  ,"RightShift"
  ,"Equals"
  ,"EqualsEquals"
  ,"BangEquals"
  ,"Bang"
  ,"PipePipe"
  ,"Pipe"
  ,"PipeEquals"
  ,"PercentEquals"
  ,"CaretEquals"
  ,"Tilde"
  ,"Comma"
  ,"Semicolon"
  ,"Dot"
  ,"Ellipsis"
  ,"Scope"
  ,"At"
  ,"LParen"
  ,"RParen"
  ,"LBrace"
  ,"RBrace"
  ,"LBracket"
  ,"RBracket"
  ,"Identifier"
};

void tokens_vec_print(Tokens* tokens){
	for(int i=0;i<tokens->size;i++){
 		sourceLocPrint(&(tokens->toks[i]->loc));
   		Token* current = tokens->toks[i];
 		printf("   %s (%s)\n",current->text,tokens_types[current-> type]);
  	}
}