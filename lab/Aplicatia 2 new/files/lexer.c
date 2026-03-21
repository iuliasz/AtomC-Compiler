#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "utils.h"

Token *tokens;	// single linked list of tokens
Token *lastTk;		// the last token in list

int line=1;		// the current line in the input file

// adds a token to the end of the tokens list and returns it
// sets its code and line
Token *addTk(int code){
	Token *tk=safeAlloc(sizeof(Token));
	tk->code=code;
	tk->line=line;
	tk->next=NULL;
	if(lastTk){
		lastTk->next=tk;
		}else{
		tokens=tk;
		}
	lastTk=tk;
	return tk;
	}

char *extract(const char *begin,const char *end){
	err("extract needs to be implemented");
	}

Token *tokenize(const char *pch){
	const char *start;
	Token *tk;
	for(;;){
		switch(*pch){
			// space
			case ' ':case '\t':pch++;break;
			case '\r':		// handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
				if(pch[1]=='\n')pch++;
				// fallthrough to \n
			case '\n':
				line++;
				pch++;
				break;

			// delimiters
			case ',': addTk(COMMA);pch++;break;
			case ';': addTk(SEMICOLON);pch++;break;
			case '(': addTk(LPAR);pch++;break;
			case ')': addTk(RPAR);pch++;break;
			case '[': addTk(LBRACKET);pch++;break;
			case ']': addTk(RBRACKET);pch++;break;
			case '{': addTk(LACC);pch++;break;
			case '}': addTk(RACC);pch++;break;
			case '\0':addTk(END);return tokens;

			// operators
			case '+': addTk(ADD);pch++;break;
			case '-': addTk(SUB);pch++;break;
			case '*': addTk(MUL);pch++;break;
			case '/': 
				if(pch[1]=='/'){
					pch+=2;
					while(*pch!='\n' && *pch!='\r' && *pch!='\0') pch++; // linecomment
				}else{
					addTk(DIV);
					pch++;
				}
				break;
			case '.': addTk(DOT);pch++;break;
			case '&':
				if(pch[1]=='&'){
					addTk(AND);
					pch+=2;
				}else{
					err("invalid char: %c (%d)", *pch, *pch);
				}
				break;
			case '|':
				if(pch[1]=='|'){
					addTk(OR);
					pch+=2;
				}else{
					err("invalid char: %c (%d)", *pch, *pch);
				}
				break;
			case '!':
				if(pch[1]=='='){
					addTk(NOTEQ);
					pch+=2;
				}else{
					addTk(NOT);
					pch++;
				}
				break;
			case '=':
				if(pch[1]=='='){
					addTk(EQUAL);
					pch+=2;
				}else{
				addTk(ASSIGN);
				pch++;
				}
				break;
			case '<':
				if(pch[1]=='='){
					addTk(LESSEQ);
					pch+=2;
				}else{
					addTk(LESS);
					pch++;
				}
				break;
			case '>':
				if(pch[1]=='='){
					addTk(GREATEREQ);
					pch+=2;
				}else{
					addTk(GREATER);
					pch++;
				}
				break;
			
			// constants char
			case '\'':
				pch++;

				char c;

				if(*pch=='\'' || *pch=='\0' || *pch=='\r' || *pch=='\n') err("invalid char constant");
				
				if(*pch=='\\'){
					pch++;

					switch(*pch){
						case 'a':c='\a';break;
						case 'b':c='\b';break; 
						case 'f':c='\f';break;
						case 'n':c='\n';break;
						case 'r':c='\r';break;
						case 't':c='\t';break;
						case 'v':c='\v';break;
						case '\\':c='\\';break;
						case '\'':c='\'';break;
						case '"':c='\"';break;
						case '0':c='\0';break;
						default: err("invalid escape in char");
					}
					pch++;
				}else{
					c=*pch;
					pch++;
				}

				if(*pch!='\'') err("misssing closing '");
				pch++;

				tk=addTk(CHAR);
				tk->c=c;
				break;
			
			// constants string
			case '"':
				pch++;

				char *buffer=safeAlloc(100);
				int length=0;

				while(*pch!='"'){
					if(*pch=='\0' || *pch=='r' || *pch=='\n') err("unterminated string constant");
		
					if(*pch=='\\'){
						pch++;

						switch(*pch){
						case 'a':buffer[length++]='\a';break;
						case 'b':buffer[length++]='\b';break; 
						case 'f':buffer[length++]='\f';break;
						case 'n':buffer[length++]='\n';break;
						case 'r':buffer[length++]='\r';break;
						case 't':buffer[length++]='\t';break;
						case 'v':buffer[length++]='\v';break;
						case '\\':buffer[length++]='\\';break;
						case '\'':buffer[length++]='\'';break;
						case '"':buffer[length++]='\"';break;
						case '0':buffer[length++]='\0';break;
						default: err("invalid escape in string");
						}
						pch++;
					}else{
						buffer[length++]=*pch;
						pch++;
					}
				}

				buffer[length]='\0';
				pch++;

				tk=addTk(STRING);
				tk->text=buffer;
				break;
			default: 
				// keywords
				if(isalpha(*pch)||*pch=='_'){
					for(start=pch++;isalnum(*pch)||*pch=='_';pch++){}
					char *text=extract(start,pch);
					if(strcmp(text,"char")==0)addTk(TYPE_CHAR);
					else if(strcmp(text,"double")==0)addTk(TYPE_DOUBLE);
					else if(strcmp(text,"else")==0)addTk(ELSE);
					else if(strcmp(text,"if"))addTk(IF);
					else if(strcmp(text,"int"))addTk(TYPE_INT);
					else if(strcmp(text,"return"))addTk(RETURN);
					else if(strcmp(text,"struct"))addTk(STRUCT);
					else if(strcmp(text,"void"))addTk(VOID);
					else if(strcmp(text,"while"))addTk(WHILE);
					else{
						tk=addTk(ID);
						tk->text=text;
						}
						
					//  constants
					}else if(isdigit(*pch)){
						for(start=pch++;isdigit(*pch);pch++);

						int isDouble=0;

						// double
						if(*pch=='.'){
							isDouble=1;
							pch++;

							if(!isdigit(*pch)) err("invalid double"); // at least one digit after .

							for(start=pch++;isdigit(*pch);pch++);
						}

						// with exponent
						if(*pch=='e' || *pch=='E'){
							isDouble=1;
							pch++;

							if(*pch=='+' || *pch=='-') pch++;

							if(!isdigit(*pch)) err("invalid exponent");

							for(start=pch++;isdigit(*pch);pch++);
						}

						tk=addTk(isDouble ? DOUBLE:INT);
						char *text=extract(start,pch);
						tk->text=text;
					}

				else err("invalid char: %c (%d)",*pch,*pch);
			}
		}
	}

void showTokens(const Token *tokens){
	for(const Token *tk=tokens;tk;tk=tk->next){
		printf("%d\n",tk->code);
		}
	}
