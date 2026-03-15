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
			
			// spaces ignored
			case ' ':case '\t':pch++;break;

			case '\r':		// handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
				if(pch[1]=='\n')pch++;

			// fallthrough to \n
			case '\n':
				line++;
				pch++;
				break;
			
			
			// delimiters
			case ',':addTk(COMMA);pch++;break;
			case ';':addTk(SEMICOLON);pch++;break;
			case '(':addTk(LPAR);pch++;break;
			case ')':addTk(RPAR);pch++;break;
			case '[':addTk(LBRACKET);pch++;break;
			case ']':addTk(RBRACKET);pch++;break;
			case '{':addTk(LACC);pch++;break;
			case '}':addTk(RACC);pch++;break;
			case '\0':addTk(END);return tokens;

			// operators
			case '+':addTk(ADD);pch++;break;
			case '-':addTk(SUB);pch++;break;
			case '*':addTk(MUL);pch++;break;
			case '/':
				if (pch[1]=='/'){
					pch+=2;
					while(*pch!='\n' && *pch!='\r'&&*pch!='\0')pch++;
					}else{

					addTk(DIV);
					pch++;
					}
				break;
			case '.':addTk(DOT);pch++;break;
			case '&':
					if(pch[1]=='&'){
						addTk(AND);
						pch+=2;
						}else{

						err("invalid char: %c (%d)",*pch,*pch);
						}
					break;
			case '|':
				if(pch[1]=='|'){
					addTk(OR);
					pch+=2;
					}else{

					err("invalid char: %c (%d)",*pch,*pch);
					}
				break;
			case '!':
				if(pch[1]=='='){
					addTk(NOTEQ);
					pch+=2;
					}
					
				else{

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
			
		    // char
			case '\'':
				pch++;
				tk=addTk(CHAR);

				if(*pch=='\\'){
					pch++;

					switch(*pch){
						case 'a':tk->i='\a';break;
						case 'b':tk->i='\b';break;
						case 'f':tk->i='\f';break;
						case 'n':tk->i='\n';break;
						case 'r':tk->i='\r';break;
						case 't':tk->i='\t';break;
						case 'v':tk->i='\v';break;
						case '\\':tk->i='\\';break;
						case '\'':tk->i='\'';break;
						case '"':tk->i='\"';break;
						case '0':tk->i='\0';break;

						default:
							err("invalid escape sequence");
						}
					

					pch++;
				}else{

					if(*pch=='\'' || *pch=='\0' || *pch=='\n' || *pch=='\r')
						err("invalid char constant");
					
					tk->i=*pch;
					pch++;
				}

				if(*pch!='\'')
					err("missing closing ' in char constant");
				
				pch++;
				break;

			// string
			case '"':{
				char buffer[1024];
				int n=0;

				pch++; // skip first "

				while(*pch!='"'){
					if(*pch=='\0' || *pch=='\n' || *pch=='\r')
						err("missing closing \" in string");

					if(*pch=='\\'){
						pch++;

						if(n>=1023) err("string too long");

						switch(*pch){
							case 'a': buffer[n++]='\a'; break;
							case 'b': buffer[n++]='\b'; break;
							case 'f': buffer[n++]='\f'; break;
							case 'n': buffer[n++]='\n'; break;
							case 'r': buffer[n++]='\r'; break;
							case 't': buffer[n++]='\t'; break;
							case 'v': buffer[n++]='\v'; break;
							case '\\': buffer[n++]='\\'; break;
							case '\'': buffer[n++]='\''; break;
							case '"': buffer[n++]='\"'; break;
							case '0': buffer[n++]='\0'; break;

							default:
								err("invalid escape sequence");
						}

						pch++;
					}else{
						if(n>=1023) err("string too long");
						buffer[n++]=*pch;
						pch++;
					}
				}

				buffer[n]='\0';

				tk=addTk(STRING);
				tk->text=safeAlloc(n+1);
				memcpy(tk->text,buffer,n+1);

				pch++; // skip closing "
				break;
			}
						
		
			default:
				if(isalpha(*pch)||*pch=='_'){
					for(start=pch++;isalnum(*pch)||*pch=='_';pch++){}
					char *text=extract(start,pch);

					if(strcmp(text,"char")==0){addTk(TYPE_CHAR); free(text);}
					else if(strcmp(text,"double")==0){addTk(TYPE_DOUBLE); free(text);}
					else if(strcmp(text,"else")==0){addTk(ELSE); free(text);}
					else if(strcmp(text,"if")==0){addTk(IF); free(text);}
					else if(strcmp(text,"int")==0){addTk(TYPE_INT); free(text);}
					else if(strcmp(text,"return")==0){addTk(RETURN); free(text);}
					else if(strcmp(text,"struct")==0){addTk(STRUCT); free(text);}
					else if(strcmp(text,"void")==0){addTk(VOID); free(text);}
					else if(strcmp(text,"while")==0){addTk(WHILE); free(text);}
					else{
						tk=addTk(ID);
						tk->text=text;
						}
					}else if(isdigit(*pch)){
						int isDouble=0;
						start=pch;
						while(isdigit(*pch))pch++;

						if(*pch=='.'){
							isDouble=1;
							pch++;
							if(!isdigit(*pch))err("invalid double number");
							while(isdigit(*pch))pch++;
						}

						if(*pch=='e'||*pch=='E'){
							isDouble=1;
							pch++;
							if(*pch=='+'||*pch=='-')pch++;
							if(!isdigit(*pch))err("invalid exponent");
							while(isdigit(*pch))pch++;
						}

						if(isDouble){

							char *text = extract(start,pch);
							tk = addTk(DOUBLE);
							tk->d = atof(text);
							free(text);
							}else{
							
							char *text=extract(start,pch);
							tk=addTk(INT);
							tk->i=atoi(text);
							free(text);
							}
					}
						
				else err("invalid char");
			}
		}
	}

void showTokens(const Token *tokens){
    for(const Token *tk=tokens; tk; tk=tk->next){
        printf("%d\t", tk->line);

        switch(tk->code){
            case ID: printf("ID:%s", tk->text); break;

            case TYPE_CHAR: printf("TYPE_CHAR"); break;
            case TYPE_DOUBLE: printf("TYPE_DOUBLE"); break;
            case ELSE: printf("ELSE"); break;
            case IF: printf("IF"); break;
            case TYPE_INT: printf("TYPE_INT"); break;
            case RETURN: printf("RETURN"); break;
            case STRUCT: printf("STRUCT"); break;
            case VOID: printf("VOID"); break;
            case WHILE: printf("WHILE"); break;

            case INT: printf("INT:%d", tk->i); break;
            case DOUBLE: printf("DOUBLE:%g", tk->d); break;
            case CHAR: printf("CHAR:%c", tk->i); break;
            case STRING: printf("STRING:%s", tk->text); break;

            case COMMA: printf("COMMA"); break;
            case SEMICOLON: printf("SEMICOLON"); break;
            case LPAR: printf("LPAR"); break;
            case RPAR: printf("RPAR"); break;
            case LBRACKET: printf("LBRACKET"); break;
            case RBRACKET: printf("RBRACKET"); break;
            case LACC: printf("LACC"); break;
            case RACC: printf("RACC"); break;
            case END: printf("END"); break;

            case ADD: printf("ADD"); break;
            case SUB: printf("SUB"); break;
            case MUL: printf("MUL"); break;
            case DIV: printf("DIV"); break;
            case DOT: printf("DOT"); break;
            case AND: printf("AND"); break;
            case OR: printf("OR"); break;
            case NOT: printf("NOT"); break;
            case ASSIGN: printf("ASSIGN"); break;
            case EQUAL: printf("EQUAL"); break;
            case NOTEQ: printf("NOTEQ"); break;
            case LESS: printf("LESS"); break;
            case LESSEQ: printf("LESSEQ"); break;
            case GREATER: printf("GREATER"); break;
            case GREATEREQ: printf("GREATEREQ"); break;

            default: printf("UNKNOWN");
        }

        printf("\n");
    }
}