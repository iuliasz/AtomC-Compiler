#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

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
	int n=(int)(end-begin);

	char *text=safeAlloc(n+1);
	memcpy(text,begin,n);
	text[n]='\0';

	return text;
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
					if(*pch=='\0' || *pch=='\r' || *pch=='\n') err("unterminated string constant");
		
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
					else if(strcmp(text,"if")==0)addTk(IF);
					else if(strcmp(text,"int")==0)addTk(TYPE_INT);
					else if(strcmp(text,"return")==0)addTk(RETURN);
					else if(strcmp(text,"struct")==0)addTk(STRUCT);
					else if(strcmp(text,"void")==0)addTk(VOID);
					else if(strcmp(text,"while")==0)addTk(WHILE);
					else{
						tk=addTk(ID);
						tk->text=text;
						}
						
					//  constants
					}else if(isdigit(*pch)){
						start=pch;
						while(isdigit(*pch)) pch++;

						int isDouble=0;

						// double
						if(*pch=='.'){
							isDouble=1;
							pch++;

							if(!isdigit(*pch)) err("invalid double"); // at least one digit after .

							while(isdigit(*pch)) pch++;
						}

						// with exponent
						if(*pch=='e' || *pch=='E'){
							isDouble=1;
							pch++;

							if(*pch=='+' || *pch=='-') pch++;

							if(!isdigit(*pch)) err("invalid exponent");

							while(isdigit(*pch)) pch++;
						}

						if(isalpha(*pch) || *pch=='_') err("invalid id");

						tk=addTk(isDouble ? DOUBLE:INT);
						char *text=extract(start,pch);
						
						if(isDouble==0) tk->i=atoi(text);
						else {
							tk->originalText=strdup(text);
							tk->d=atof(text);
						}

						free(text);
					}

				else err("invalid char: %c (%d)",*pch,*pch);
			}
		}
	}

void showTokens(const Token *tokens){
	for(const Token *tk=tokens;tk;tk=tk->next){
		switch(tk->code){
            case ID: printf("%d\tID:%s", tk->line, tk->text); break;

			// keywords
			case TYPE_CHAR: printf("%d\tTYPE_CHAR", tk->line); break;
			case TYPE_DOUBLE: printf("%d\tTYPE_DOUBLE", tk->line); break;
			case ELSE: printf("%d\tELSE", tk->line); break;
			case IF: printf("%d\tIF", tk->line); break;
			case TYPE_INT: printf("%d\tTYPE_INT", tk->line); break;
			case RETURN: printf("%d\tRETURN", tk->line); break;
			case STRUCT: printf("%d\tSTRUCT", tk->line); break;
			case VOID: printf("%d\tVOID", tk->line); break;
			case WHILE: printf("%d\tWHILE", tk->line); break;

			// constants
			case INT: printf("%d\tINT:%d", tk->line, tk->i); break;
			case DOUBLE: printf("%d\tDOUBLE:%.2f", tk->line, tk->d); break;
			case CHAR: printf("%d\tCHAR:%c", tk->line, tk->c); break;
			case STRING: printf("%d\tSTRING:%s", tk->line, tk->text); break;

			// delimiters
			case COMMA: printf("%d\tCOMMA", tk->line); break;
			case SEMICOLON: printf("%d\tSEMICOLON", tk->line); break;
			case LPAR: printf("%d\tLPAR", tk->line); break;
			case RPAR: printf("%d\tRPAR", tk->line); break;
			case LBRACKET: printf("%d\tLBRACKET", tk->line); break;
			case RBRACKET: printf("%d\tRBRACKET", tk->line); break;
			case LACC: printf("%d\tLACC", tk->line); break;
			case RACC: printf("%d\tRACC", tk->line); break;
			case END: printf("%d\tEND", tk->line); break;

			// operators
			case ADD: printf("%d\tADD", tk->line); break;
			case SUB: printf("%d\tSUB", tk->line); break;
			case MUL: printf("%d\tMUL", tk->line); break;
			case DIV: printf("%d\tDIV", tk->line); break;
			case DOT: printf("%d\tDOT", tk->line); break;
			case AND: printf("%d\tAND", tk->line); break;
			case OR: printf("%d\tOR", tk->line); break;
			case NOT: printf("%d\tNOT", tk->line); break;
			case ASSIGN: printf("%d\tASSIGN", tk->line); break;
			case EQUAL: printf("%d\tEQUAL", tk->line); break;
			case NOTEQ: printf("%d\tNOTEQ", tk->line); break;
			case LESS: printf("%d\tLESS", tk->line); break;
			case LESSEQ: printf("%d\tLESSEQ", tk->line); break;
			case GREATER: printf("%d\tGREATER", tk->line); break;
			case GREATEREQ: printf("%d\tGREATEREQ", tk->line); break;

			default: printf("%d\tUNKNOWN", tk->line);
        }
			printf("\n");
		}
	}
