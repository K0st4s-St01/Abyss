#include "../../include/lexer/lexer.h"
#include <string.h>
#include <ctype.h>
#include "../../include/utils/string_builder.h"

static bool token_is_primitive_type_raw(TokenType type) {
    return type >= I8 && type <= Bool;
}

Lexer *lexer_new(char *filename, char *source_code) {
  Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
  if (!lexer) return NULL;
  lexer->filename = filename;
  lexer->source_code = source_code;
  lexer->line = 1;
  lexer->col = 1;
  lexer->index = 0;
  lexer-> source_code_len=  strlen(source_code);
  lexer->prev_token_type = -1;
  return lexer;
}

char lexer_peek(Lexer *lexer, int offset){
    if(lexer->index + offset >  lexer->source_code_len - 1){
        return '\0';
    }
    return lexer-> source_code[lexer->index+offset];
}
char lexer_advance(Lexer *lexer){
	char c = lexer_peek(lexer,0);
 	if(c!='\0'){
  		lexer->col++;
 		if(c == '\n'){
   			lexer->line++;
      		lexer->col=1;
     	}   	
     	lexer->index++;
   	}
    return c;
}
bool lexer_match(Lexer *lexer,char to_match){
	if(lexer_peek(lexer,0) == to_match){
 		lexer_advance(lexer);
   		return true;
  	}
   return false;
   }
   
void lexer_skip_whitespace_and_comments(Lexer *lexer){
    for (;;) {
	    char c = lexer_peek(lexer,0);
        while(c == ' ' || c == '\t' || c == '\r' || c=='\n'){
            lexer_advance(lexer);
            c = lexer_peek(lexer,0);
        }

        if (c == '#') {
            while(c != '\n' && c != '\0'){
                lexer_advance(lexer);
                c = lexer_peek(lexer,0);
            }
            continue;
        }

        if (c == '/' && lexer_peek(lexer, 1) == '/') {
            while(c != '\n' && c != '\0'){
                lexer_advance(lexer);
                c = lexer_peek(lexer,0);
            }
            continue;
        }

        if (c == '/' && lexer_peek(lexer, 1) == '*') {
            lexer_advance(lexer);
            lexer_advance(lexer);
            while (lexer_peek(lexer, 0) != '\0') {
                if (lexer_peek(lexer, 0) == '*' && lexer_peek(lexer, 1) == '/') {
                    lexer_advance(lexer);
                    lexer_advance(lexer);
                    break;
                }
                lexer_advance(lexer);
            }
            continue;
        }

        break;
    }
}
SourceLocation lexer_current_location(Lexer *lexer){
	SourceLocation loc;
 	loc.line = lexer->line;
  	loc.col = lexer->col;
   	loc.filename = lexer->filename;
    return loc;
}

Token *lexer_next_token(Lexer *lexer){
	lexer_skip_whitespace_and_comments(lexer);
	char c = lexer_peek(lexer,0);
	if(isalpha(c) || c == '_'){
 		Token *t = lexer_make_identifier_or_keyword(lexer);
 		lexer->prev_token_type = t->type;
 		return t;
  	}
   	if(isdigit(c) || c == '\'' || c=='"'){
    	Token *t = lexer_make_literal(lexer);
    	lexer->prev_token_type = t->type;
    	return t;
    }
       
    switch(c){
    	case '-':
     	case '+':
      	case '*':      	
       	case '/':
       	case '%':
        case '&':
        case '=':
        case '!':
        case '|':
        case '^':
        case '~':
        	{ Token *t = lexer_make_operator(lexer); lexer->prev_token_type = t->type; return t; }
        case '<':
        	case '>':
        	{
        	    TokenType prev = lexer->prev_token_type;
        	    bool prev_is_type = (prev == Generic ||
        	                        token_is_primitive_type_raw(prev) || prev == Void);
        	    bool is_generic_open  = (c == '<' && prev_is_type);
        	    bool is_generic_close = (c == '>' && prev_is_type);
        	    if (is_generic_open || is_generic_close) {
        	        lexer_advance(lexer);
        	        Token *t = token_new(lexer_current_location(lexer),
        	                           c == '<' ? "<" : ">",
        	                           Generic);
        	        lexer->prev_token_type = Generic;
        	        return t;
        	    }
        	    { Token *t = lexer_make_operator(lexer); lexer->prev_token_type = t->type; return t; }
        	}
        case ',':
        case ';':
        case '.':
            if (c == '.' && lexer_peek(lexer, 1) == '.' && lexer_peek(lexer, 2) == '.') {
                lexer_advance(lexer); lexer_advance(lexer); lexer_advance(lexer);
                Token *t = token_new(lexer_current_location(lexer), "...", Ellipsis);
                lexer->prev_token_type = Ellipsis;
                return t;
            }
            { Token *t = lexer_make_punctuation(lexer); lexer->prev_token_type = t->type; return t; }
        case ':':
        case '?':
        case '@':
        case '(':
        case ')':
        case '{':
        case '}':
        case '[':
        case ']':
        	{ Token *t = lexer_make_punctuation(lexer); lexer->prev_token_type = t->type; return t; }
        case '\0':
        	{ Token *t = token_new(lexer_current_location(lexer),"<EOF>",EndOfFile);
        	  lexer->prev_token_type = t->type; return t; }
     	default:
       		lexer_advance(lexer);
       		{ Token *t = token_new(lexer_current_location(lexer),"<INVALID>",Invalid);
       		  lexer->prev_token_type = t->type; return t; }
    }
 	
}

static char* keywords[26] = {
  "if"
  ,"else"
  ,"elif"
  ,"switch"
  ,"case"
  ,"default"
  ,"while"
  ,"for"
  ,"do"
  ,"static"
  ,"type"
  ,"enum"
  ,"struct"
  ,"interface"
  ,"import"
  ,"return"
  ,"break"
  ,"continue"
  ,"self"
  ,"extern"
  ,"new"
  ,"delete"
  ,"null"
  ,"true"
  ,"false"
  ,"sizeof"
};

static char* primitives[13] ={
  "i8"
  ,"i16"
  ,"i32"
  ,"i64"
  ,"u8"
  ,"u16"
  ,"u32"
  ,"u64"
  ,"f32"
  ,"f64"
  ,"char"
  ,"str"
  ,"bool"
};

Token *lexer_make_identifier_or_keyword(Lexer *lexer){
	char c = lexer_advance(lexer);
 	StringBuilder* sb = string_builder_new();
  	string_builder_add(sb,c);
   	c = lexer_peek(lexer,0);
   	while(isalnum(c) || c == '_'){
    	string_builder_add(sb,c);
     	lexer_advance(lexer);
      	c = lexer_peek(lexer,0);
    }
    char* current_word = string_builder_to_string(sb);
    string_builder_free(sb);
  	for(int i =0;i<26;i++){
   		if(i < 26 && strcmp(current_word,keywords[i]) == 0){
     		Token *tok = token_new(lexer_current_location(lexer),current_word,i + 20);
  			free(current_word);
  			return tok;
       }
       if(i<13 && strcmp(current_word,primitives[i]) == 0 ){
     		Token *tok = token_new(lexer_current_location(lexer),current_word,i + 6);
  			free(current_word);
  			return tok;
       }
       
   	}
    Token *tok = token_new(lexer_current_location(lexer),current_word,Identifier);
   	free(current_word);
   	return  tok;
  	
}
static Token* lex_char_literal(Lexer *lexer){
	char c = lexer_advance(lexer);
 	StringBuilder *sb = string_builder_new();
  	string_builder_add(sb,c);
 	while(lexer_peek(lexer,0) != '\'' && lexer_peek(lexer,0) != '\0'){
  		c = lexer_advance(lexer);
    	string_builder_add(sb,c);
        if (c == '\\' && lexer_peek(lexer,0) != '\0') {
            c = lexer_advance(lexer);
            string_builder_add(sb,c);
        }
   	}
    if(lexer_peek(lexer,0) == '\''){
        c =lexer_advance(lexer);
        string_builder_add(sb,c);
    }
    char* current_word = string_builder_to_string(sb);
    string_builder_free(sb);
    Token *tok = token_new(lexer_current_location(lexer),current_word,CharLiteral);
    free(current_word);
    return tok;
}
static Token* lex_str_literal(Lexer *lexer){
	char c = lexer_advance(lexer);
 	StringBuilder *sb = string_builder_new();
  	string_builder_add(sb,c);
 	while(lexer_peek(lexer,0) != '"' && lexer_peek(lexer,0) != '\0'){
  		c = lexer_advance(lexer);
        if (c == '\\' && lexer_peek(lexer, 0) != '\0') {
            char next = lexer_advance(lexer);
            switch (next) {
                case 'n':  string_builder_add(sb, '\n'); break;
                case 't':  string_builder_add(sb, '\t'); break;
                case 'r':  string_builder_add(sb, '\r'); break;
                case '\\': string_builder_add(sb, '\\'); break;
                case '"':  string_builder_add(sb, '"'); break;
                case '0':  string_builder_add(sb, '\0'); break;
                default:   string_builder_add(sb, '\\'); string_builder_add(sb, next); break;
            }
        } else {
    	    string_builder_add(sb,c);
        }
   	}
    if(lexer_peek(lexer,0) == '"'){
        c =lexer_advance(lexer);
        string_builder_add(sb,c);
    }
    char* current_word = string_builder_to_string(sb);
    string_builder_free(sb);
    Token *tok = token_new(lexer_current_location(lexer),current_word,StringLiteral);
    free(current_word);
    return tok;
}
static Token* lex_number_literal(Lexer *lexer){
	char c = lexer_advance(lexer);
 	StringBuilder *sb = string_builder_new();
  	TokenType tt = IntLiteral;
 	string_builder_add(sb,c);

    if (c == '0' && (lexer_peek(lexer,0) == 'x' || lexer_peek(lexer,0) == 'X' ||
                     lexer_peek(lexer,0) == 'b' || lexer_peek(lexer,0) == 'B' ||
                     lexer_peek(lexer,0) == 'o' || lexer_peek(lexer,0) == 'O')) {
        c = lexer_advance(lexer);
        string_builder_add(sb,c);
        while(isalnum(lexer_peek(lexer,0)) || lexer_peek(lexer,0) == '_'){
            c = lexer_advance(lexer);
            string_builder_add(sb,c);
        }
        char* current_word = string_builder_to_string(sb);
        string_builder_free(sb);
        Token *tok = token_new(lexer_current_location(lexer),current_word,tt);
        free(current_word);
        return tok;
    }

  	while(isdigit(lexer_peek(lexer,0)) || lexer_peek(lexer,0) == '_'){
   		c = lexer_advance(lexer);
     	string_builder_add(sb,c);
    }
    if(lexer_peek(lexer,0) == '.'){
    	c = lexer_advance(lexer);
     	string_builder_add(sb,c);
       	while(isdigit(lexer_peek(lexer,0)) || lexer_peek(lexer,0) == '_'){
	   		c = lexer_advance(lexer);
	     	string_builder_add(sb,c);
	    }
     	tt = FloatLiteral; 	
    }
    char* current_word = string_builder_to_string(sb);
    string_builder_free(sb);
    Token *tok = token_new(lexer_current_location(lexer),current_word,tt);
    free(current_word);
    return tok;
}
Token *lexer_make_literal(Lexer *lexer){
	char c = lexer_peek(lexer,0);
 	switch(c){
  		case '\'':
    		return lex_char_literal(lexer);
    	case '"':
     		return lex_str_literal(lexer);
     	default:
      		return lex_number_literal(lexer);
   	}
}
Token *lexer_make_punctuation(Lexer *lexer){
	char c = lexer_advance(lexer);
 	switch(c){
		case ',':return token_new(lexer_current_location(lexer),",",Comma);
  		case ';':return token_new(lexer_current_location(lexer),";",Semicolon);
		case '.':return token_new(lexer_current_location(lexer),".",Dot);
		case '?':return token_new(lexer_current_location(lexer),"?",Question);
		case ':':
  			if(lexer_match(lexer,':')){
  				return token_new(lexer_current_location(lexer),"::",Scope);
      		}else{
	       		return token_new(lexer_current_location(lexer),":",Scope);
        	}
		case '@':return token_new(lexer_current_location(lexer),"@",At);
		case '(':return token_new(lexer_current_location(lexer),"(",LParen);
		case ')':return token_new(lexer_current_location(lexer),")",RParen);
		case '{':return token_new(lexer_current_location(lexer),"{",LBrace);
		case '}':return token_new(lexer_current_location(lexer),"}",RBrace);
		case '[':return token_new(lexer_current_location(lexer),"[",LBracket);
		case ']':return token_new(lexer_current_location(lexer),"]",RBracket);

      	default:
       		return token_new(lexer_current_location(lexer),"<INVALID>",Invalid);
    }
    
 	
}
Token *lexer_make_operator(Lexer *lexer){
	char c = lexer_advance(lexer);
 	SourceLocation loc = lexer_current_location(lexer);
  	switch(c){
   		case '-':
     		if(lexer_match(lexer,'>'))
       			return token_new(loc,"->",Arrow);
          	if(lexer_match(lexer,'-'))
      			return token_new(loc,"--",MinusMinus);
         	if(lexer_match(lexer,'='))
          		return token_new(loc,"-=",MinusEquals);
         	return token_new(loc,"-",Minus);
      	case '+':
       		if(lexer_match(lexer,'+'))
         		return token_new(loc,"++",PlusPlus);
       		if(lexer_match(lexer,'='))
         		return token_new(loc,"+=",PlusEquals);
          return token_new(loc,"+",Plus);
       	case '*':
        	if(lexer_match(lexer,'='))
         		return token_new(loc,"*=",StarEquals);
           return token_new(loc,"*",Star);
        case '/':
        	if(lexer_match(lexer,'='))
         		return token_new(loc,"/=",SlashEquals);
           return token_new(loc,"/",Slash);
        case '%':
        	if(lexer_match(lexer,'='))
         		return token_new(loc,"%=",PercentEquals);
        	return token_new(loc,"%",Percent);
        case '&':
        	if(lexer_match(lexer,'='))
         		return token_new(loc,"&=",AmpersandEquals);
           	if(lexer_match(lexer,'&'))
            	return token_new(loc,"&&",AmpersandAmpersand);
        	return token_new(loc,"&",Ampersand); 
        case '|':
        	if(lexer_match(lexer,'='))
         		return token_new(loc,"|=",PipeEquals);
           	if(lexer_match(lexer,'|'))
            	return token_new(loc,"||",PipePipe);
        	return token_new(loc,"|",Pipe); 
        case '^':
        	if(lexer_match(lexer,'='))
         		return token_new(loc,"^=",CaretEquals);
        	return token_new(loc,"^",Caret); 
        case '~':
        	return token_new(loc,"~",Tilde);
        case '<':
        	if(lexer_match(lexer,'<'))
         		return token_new(loc,"<<",LeftShift);
           if(lexer_match(lexer,'='))
           		return token_new(loc,"<=",LessEquals);
        	return token_new(loc,"<",Less);
        case '>':
        	if(lexer_match(lexer,'>'))
         		return token_new(loc,">>",RightShift);
           if(lexer_match(lexer,'='))
           		return token_new(loc,">=",GreaterEquals);
        	return token_new(loc,">",Greater);
        case '=':
        	if(lexer_match(lexer,'='))
         		return token_new(loc,"==",EqualsEquals);
           return token_new(loc,"=",Equals);
        case '!':
        	if(lexer_match(lexer,'='))
         		return token_new(loc,"!=",BangEquals);
           return token_new(loc,"!",Bang);
        default:
           return token_new(loc,"<INVALID>",Invalid);
        
   	}
}

bool lexer_has_next(Lexer* lexer){
	return lexer->index < lexer->source_code_len;
}
