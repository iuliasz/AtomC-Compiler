#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;		// the iterator in the tokens list
Token *consumedTk;		// the last consumed token

void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

bool consume(int code){
	if(iTk->code==code){
		consumedTk=iTk;
		iTk=iTk->next;
		return true;
		}
	return false;
	}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(){
	if(consume(TYPE_INT)){
		return true;
		}
	if(consume(TYPE_DOUBLE)){
		return true;
		}
	if(consume(TYPE_CHAR)){
		return true;
		}
	if(consume(STRUCT)){
		if(consume(ID)){
			return true;
			}
		}
	return false;
	}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl(){
	Token *start=iTk;
	if(consume(LBRACKET)){
		cosnume(INT);
		if(consume(RBRACKET)) return true;
	}
	iTk=start;

	return false;
}

// fnParam: typeBase ID arrayDecl?
bool fnParam(){
	Token *start=iTk;
	if(typeBase()){
		if(consume(ID)){
			arrayDecl();
			return true;
		}
	}
	iTk=start;

	return false;
}

// fnDef: ( typeBase | VOID ) ID
// LPAR ( fnParam ( COMMA fnParam )* )? RPAR
// stmCompound
bool fnDef(){
	Token *start=iTk;
	if(typeBase() || consume(VOID)){
		if(consume(ID)){
			if(consume(LPAR)){
				if(fnParam()){
					for(;;){
						if(consume(COMMA)){
							if(fnParam()){}
							else break;
						}else break;
					}
				}
				if(consume(RPAR)){
					if(stmCompound()){
						return true;
					}
				}
			}
		}
	}
	iTk=start;

	return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef(){
	Token *start=iTk;
	if(typeBase()){
		if(consume(ID)){
			arrayDecl();

			if(consume(SEMICOLON)) return true;
		}
	}
	iTk=start;

	return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef(){
		Token *start=iTk;
		if(consume(STRUCT)){
			if(consume(ID)){
				if(consume(LACC)){
					for(;;){
						if(varDef()){}
						else break;
					}
				}
				if(consume(RACC)){
					if(consume(SEMICOLON)){
						return true;
					}
				}
			}
		}
	iTk=start;

	return false;
}

// unit: ( structDef | fnDef | varDef )* END
bool unit(){
	for(;;){
		if(structDef()){}
		else if(fnDef()){}
		else if(varDef()){}
		else break;
		}
	if(consume(END)){
		return true;
		}
	return false;
	}

// stm: stmCompound
// | IF LPAR expr RPAR stm ( ELSE stm )?
// | WHILE LPAR expr RPAR stm
// | RETURN expr? SEMICOLON
// | expr? SEMICOLON	
bool stm(){
	Token *start=iTk;
	if(stmCompound()){
		return true;
	}
	iTk=start;
	if(consume(IF)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						if(consume(ELSE)){
							if(stm()){}
							else return false;
						}
						return true;
					}
				}
			}
		}
	}
	iTk=start;
	if(consume(WHILE)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						return true;
					}
				}
			}
		}
	}
	iTk=start;
	if(consume(RETURN)){
		expr();
		if(consume(SEMICOLON)){
			return true;
		}
	}
	iTk=start;
	expr();
	if(consume(SEMICOLON)){
		return true;
	}
	iTk=start;

	return false;
}

// stmCompound: LACC ( varDef | stm )* RACC
bool stmCompound(){
	Token *start=iTk;
	if(consume(LACC)){
		for(;;){
			if(varDef() || stm()){
				if(consume(RACC)){
					return true;
				}
			}
		}
	}
	iTk=start;

	return false;
}

//////////
// expr: exprAssign
bool expr(){
	return exprAssign();
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAsssign(){
	Token *start=iTk;
	if(exprUnary()){
		if(consume(ASSIGN)){
			if(exprAsssign()){
				return true;
			}
		}
	}
	iTk=start;
	if(exprOr()){
		return true;
	}
	iTk=start;

	return false;
}

// exprOr: exprOr OR exprAnd | exprAnd
// =>
// exprOr: exprAnd exprOrPrim
// exprOrPrim: OR exprAnd exprOrPrim | epsilon
bool exprOr(){
	if(exprAnd()){
		if(exprOrPrim()){
			return true;
		}
	}
	return false;
}

bool exprOrPrim(){
	Token *start=iTk;
	if(consume(OR)){
		if(exprAnd()){
			if(exprOrPrim()){
				return true;
			}
		}
		iTk=start;
		return false;
	}
	return true;
}

// exprAnd: exprAnd AND exprEq | exprEq
// =>
// exprAnd: exprEq exprAndPrim
// exprAndPrim: AND exprEq exprAndPrim | epsilon
bool exprAnd(){
	if(exprEq()){
		if(exprAndPrim()){
			return true;
		}
	}
	return false;
}

bool exprAndPrim(){
	Token *start=iTk;
	if(consume(AND)){
		if(exprEq()){
			if(exprAndPrim()){
				return true;
			}
		}
		iTk=start;
		return false;
	}
	return true;
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
// =>
// exprEq: exprRel exprEqPrim
// exprEqPrim: (EQUAL | NOTEQ) exprRel exprEqPrim | epsilon
bool exprEq(){
	if(exprRel()){
		if(exprEqPrim()){
			return true;
		}
	}
	return false;
}

bool exprEqPrim(){
	Token *start=iTk;
	if(consume(EQUAL) || consume(NOTEQ)){
		if(exprRel()){
			if(exprEqPrim()){
				return true;
			}
		}
		iTk=start;
		return false;
	}
	return true;
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
// =>
// exprRel: exprAdd exprRelPrim
// exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | epsilon
bool exprRel(){
	if(exprAdd()){
		if(exprRelPrim()){
			return true;
		}
	}
	return false;
}

bool exprRelPrim(){
	Token *start=iTk;
	if(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)){
		if(exprAdd()){
			if(exprRelPrim()){
				return true;
			}
		}
		iTk=start;
		return false;
	}
	return true;
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul
// =>
// exprAdd: exprMul exprAddPrim
// exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | epsilon
bool exprAdd(){
	if(exprMul()){
		if(exprAddPrim()){
			return true;
		}
	}
	return false;
}

bool exprAddPrim(){
	Token *start=iTk;
	if(consume(ADD) || consume(SUB)){
		if(exprMul()){
			if(exprAddPrim()){
				return true;
			}
		}
		iTk=start;
		return false;
	}
	return true;
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast
// =>
// exprMul: exprCast exprMulPrim
// exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | epsilon
bool exprMul(){
	if(exprCast()){
		if(exprMulPrim()){
			return true;
		}
	}
	return false;
}

bool exprMulPrim(){
	Token *start=iTk;
	if(consume(MUL) || consume(DIV)){
		if(exprCast()){
			if(exprMulPrim()){
				return true;
			}
		}
		iTk=start;
		return false;
	}
	return true;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(){
	Token *start=iTk;
	if(consume(LPAR)){
		if(typeBase()){
			arrayDecl();
			if(consume(RPAR)){
				if(exprCast()){
					return true;
				}
			}
		}
	}
	iTk=start;
	if(exprUnary()){
		return true;
	}
	iTk=start;
	return false;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(){
	Token *start=iTk;
	if(consume(SUB) || consume(NOT)){
		if(exprUnary()){
			return true;
		}
	}
	iTk=start;
	if(exprPostfix()){
		return true;
	}
	iTk=start;
	return false;
}

// exprPostfix: exprPostfix LBRACKET expr RBRACKET
// | exprPostfix DOT ID
// | exprPrimary

// =>
// exprPostfix: exprPrimary exprPostfixPrim
// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | epsilon 		
bool exprPostfix(){
	if(exprPrimary()){
		if(exprPostfixPrim()){
			return true;
		}
	}
	return false;
}

bool exprPostfixPrim(){
	Token *start=iTk;
	if(consume(LBRACKET)){
		if(expr()){
			if(consume(RBRACKET)){
				if(exprPostfixPrim()){
					return true;
				}
			}
		}
		iTk=start;
		return false;
	}
	iTk=start;
	if(consume(DOT)){
		if(consume(ID)){
			if(exprPostfixPrim()){
				return true;
			}
		}
		iTk=start;
		return false;
	}
	return true;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
// | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(){
	Token *start=iTk;
	if(consume(ID)){
		if(consume(LPAR)){
			if(expr()){
				for(;;){
					if(consume(COMMA)){
						if(expr){}
						else return false;
					}
				}
			}
			if(consume(RPAR)){
				return true;
			}
		}
	}
	iTk=start;
	if(consume(INT) | consume(DOUBLE) | consume(CHAR) | consume(STRING)){
		return true
	}
	iTk=start;
	if(consume(LPAR)){
		if(expr()){
			if(consume(RPAR)){
				return true;
			}
		}
	}
	iTk=start;
	return false;
}

void parse(Token *tokens){
	iTk=tokens;
	if(!unit())tkerr("syntax error");
	}
