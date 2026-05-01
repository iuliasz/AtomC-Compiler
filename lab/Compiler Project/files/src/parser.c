#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "ad.h"
#include "utils.h"

Token *iTk;		// the iterator in the tokens list
Token *consumedTk;		// the last consumed token

Symbol *owner = NULL;

bool stmCompound(bool newDomain);
bool expr(void);
bool exprAssign(void);
bool exprOr(void);
bool exprOrPrim(void);
bool exprAnd(void);
bool exprAndPrim(void);
bool exprEq(void);
bool exprEqPrim(void);
bool exprRel(void);
bool exprRelPrim(void);
bool exprAdd(void);
bool exprAddPrim(void);
bool exprMul(void);
bool exprMulPrim(void);
bool exprCast(void);
bool exprUnary(void);
bool exprPostfix(void);
bool exprPostfixPrim(void);
bool exprPrimary(void);



void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

const char *tkCodeName(int code){
    switch(code){
        case ID: return "ID";
        case TYPE_INT: return "TYPE_INT";
        case TYPE_DOUBLE: return "TYPE_DOUBLE";
        case TYPE_CHAR: return "TYPE_CHAR";
        case STRUCT: return "STRUCT";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case RETURN: return "RETURN";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case ASSIGN: return "ASSIGN";
        case EQUAL: return "EQUAL";
        case NOTEQ: return "NOTEQ";
        case LESS: return "LESS";
        case LESSEQ: return "LESSEQ";
        case GREATER: return "GREATER";
        case GREATEREQ: return "GREATEREQ";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        case LPAR: return "LPAR";
        case RPAR: return "RPAR";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case LACC: return "LACC";
        case RACC: return "RACC";
        case COMMA: return "COMMA";
        case SEMICOLON: return "SEMICOLON";
		case DOT: return "DOT";
        case INT: return "INT";
        case DOUBLE: return "DOUBLE";
        case CHAR: return "CHAR";
        case STRING: return "STRING";
        case VOID: return "VOID";
        case END: return "END";
        default: return "UNKNOWN";
    }
}

bool consume(int code){
    printf("consume(%s)", tkCodeName(code));

    if(iTk->code == code){
        consumedTk = iTk;
        iTk = iTk->next;
        printf(" => consumed\n");
        return true;
    }

    printf(" => found %s\n", tkCodeName(iTk->code));
    return false;
}

void rule(const char* name){
	printf("# %s\n", name);
}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
// domain analysis =>
// typeBase[out Type *t]: {t->n=-1;}
// (
// INT {t->tb=TB_INT;}
// | DOUBLE {t->tb=TB_DOUBLE;}
// | CHAR {t->tb=TB_CHAR;}
// | STRUCT ID[tkName]
// {
// t->tb=TB_STRUCT;
// t->s=findSymbol(tkName->text);
// if(!t->s)tkerr("structura nedefinita: %s",tkName->text);
// }
// )
bool typeBase(Type *t){
	rule("typeBase");
	Token *start=iTk;

	t->n=-1;
	if(consume(TYPE_INT)){
		t->tb=TB_INT;
		return true;
		}
	if(consume(TYPE_DOUBLE)){
		t->tb=TB_DOUBLE;
		return true;
		}
	if(consume(TYPE_CHAR)){
		t->tb=TB_CHAR;
		return true;
		}
	if(consume(STRUCT)){
		if(consume(ID)){
			Token *tkName=consumedTk;

			t->tb=TB_STRUCT;
			t->s=findSymbol(tkName->text);

			if(!t->s)tkerr("undefined structure: %s",tkName->text);
			
			return true;
			}
		}
		iTk=start;
		return false;
	}

// arrayDecl: LBRACKET INT? RBRACKET
// domain analysis =>
// arrayDecl[inout Type *t]: LBRACKET
// ( INT[tkSize] {t->n=tkSize->i;} | {t->n=0;} )
// RBRACKET
bool arrayDecl(Type *t){
	rule("arrayDecl");
	Token *start=iTk;
	if(consume(LBRACKET)){
		if(consume(INT)){
			Token *tkSize=consumedTk;
			t->n=tkSize->i;
		}else{
			t->n=0;
		}
		if(consume(RBRACKET)) return true;
		tkerr("missing ] after array declaration");
	}
	iTk=start;
	return false;
}

// fnParam: typeBase ID arrayDecl?
// domain analysis =>
// nParam: {Type t;} typeBase[&t] ID[tkName]
// (arrayDecl[&t] {t.n=0;} )?
// {
// Symbol *param=findSymbolInDomain(symTable,tkName->text);
// if(param)tkerr("symbol redefinition: %s",tkName->text);
// param=newSymbol(tkName->text,SK_PARAM);
// param->type=t;
// param->owner=owner;
// param->paramIdx=symbolsLen(owner->fn.params);
// // parametrul este adaugat atat la domeniul curent, cat si la parametrii fn
// addSymbolToDomain(symTable,param);
// addSymbolToList(&owner->fn.params,dupSymbol(param));
// }
bool fnParam(){
	rule("fnParam");
	Token *start=iTk;

	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(arrayDecl(&t)){
				t.n=0;
			}
			Symbol *param=findSymbolInDomain(symTable,tkName->text);

			if(param)tkerr("symbol redefinition: %s",tkName->text);

			param=newSymbol(tkName->text,SK_PARAM);
			param->type=t;
			param->owner=owner;
			param->paramIdx=symbolsLen(owner->fn.params);

			addSymbolToDomain(symTable,param);
			addSymbolToList(&owner->fn.params,dupSymbol(param));

			return true;
		}
		tkerr("missing parameter name");
	}
	iTk=start;
	return false;
}


// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
// domain analysis =>
// fnDef: {Type t;}
// ( typeBase[&t] | VOID {t.tb=TB_VOID;} ) ID[tkName] LPAR
// {
// Symbol *fn=findSymbolInDomain(symTable,tkName->text);
// if(fn)tkerr("symbol redefinition: %s",tkName->text);
// fn=newSymbol(tkName->text,SK_FN);
// fn->type=t;
// addSymbolToDomain(symTable,fn);
// owner=fn;
// pushDomain();
// }
// ( fnParam ( COMMA fnParam )* )? RPAR stmCompound[false]
// {
// dropDomain();
// owner=NULL;
// }
bool fnDef(){
	rule("fnDef");
	Token *start=iTk;
	
	Type t;
	if(iTk->code == ID && iTk->next && iTk->next->code == ID && iTk->next->next && iTk->next->next->code == LPAR){
		tkerr("invalid return type");
	}
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(consume(LPAR)){
				Symbol *fn=findSymbolInDomain(symTable,tkName->text);
				if(fn)tkerr("symbol redefinition: %s",tkName->text);

				fn=newSymbol(tkName->text,SK_FN);
				fn->type=t;
				addSymbolToDomain(symTable,fn);
				owner=fn;
				pushDomain();
				if(iTk->code != RPAR){
					if(fnParam()){
						while(consume(COMMA)){
							if(fnParam()){}
							else{
								tkerr("invalid or missing parameter after ,");
							}
						}
					}else{
						if(iTk->code == ID){
							tkerr("invalid or missing parameter type");
						}
						tkerr("invalid function parameter");
					}
				}
				if(consume(RPAR)){
					if(stmCompound(false)){
						dropDomain();
						owner=NULL;
						return true;
					}
					tkerr("missing function body");
				}
				tkerr("missing ) after function parameters");
			}
		}
	}
	iTk=start;
	if(consume(VOID)){
		t.tb=TB_VOID;
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(consume(LPAR)){
				Symbol *fn=findSymbolInDomain(symTable,tkName->text);
				if(fn)tkerr("symbol redefinition: %s",tkName->text);

				fn=newSymbol(tkName->text,SK_FN);
				fn->type=t;
				addSymbolToDomain(symTable,fn);
				owner=fn;
				pushDomain();
				if(iTk->code != RPAR){
					if(fnParam()){
						while(consume(COMMA)){
							if(fnParam()){}
							else{
								tkerr("missing parameter after ,");
							}
						}
					}else{
						if(iTk->code == ID){
							tkerr("invalid parameter type");
						}
						tkerr("invalid function parameter");
					}
				}
				if(consume(RPAR)){
					if(stmCompound(false)){
						dropDomain();
						owner=NULL;
						return true;
					}
					tkerr("missing function body");
				}
				tkerr("missing ) after function parameters");
			}
			tkerr("missing ( after function name");
		}
		tkerr("missing function name after void");
	}
	iTk=start;
	return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
// domain analysis =>
// varDef: {Type t;} typeBase[&t] ID[tkName]
// ( arrayDecl[&t]
// {if(t.n==0)tkerr("a vector variable must have a specified dimension");}
// )? SEMICOLON
bool varDef(){
	rule("varDef");
	Token *start=iTk;

	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(arrayDecl(&t)){
				if(t.n==0){
					tkerr("a vector variable must have a specified dimension");
				}
			}
			if(consume(SEMICOLON)){
				Symbol *var=findSymbolInDomain(symTable,tkName->text);
				if(var)tkerr("symbol redefinition: %s",tkName->text);

				var=newSymbol(tkName->text,SK_VAR);
				var->type=t;
				var->owner=owner;
				addSymbolToDomain(symTable,var);

				if(owner){
				switch(owner->kind){
					case SK_FN:
						var->varIdx=symbolsLen(owner->fn.locals);
						addSymbolToList(&owner->fn.locals,dupSymbol(var));
						break;
					case SK_STRUCT:
						var->varIdx=typeSize(&owner->type);
						addSymbolToList(&owner->structMembers,dupSymbol(var));
						break;
					}
				}else{
				var->varMem=safeAlloc(typeSize(&t));
				}
				return true;
			}
			tkerr("missing ; after variable definition");
		}
		tkerr("missing variable name after type");
	}
	if(iTk->code == ID && iTk->next && iTk->next->code == ID && iTk->next->next && iTk->next->next->code == ID){
		tkerr("invalid variable type");
	}
	 if(iTk->code == ID && iTk->next && iTk->next->code == ID){
        tkerr("invalid variable type");
    }
	iTk=start;
	return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
// domain analysis =>
// structDef: STRUCT ID[tkName] LACC
bool structDef(){
	rule("structDef");
	Token *start=iTk;
	if(consume(STRUCT)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(consume(LACC)){
				Symbol *s=findSymbolInDomain(symTable,tkName->text);

				if(s)tkerr("symbol redefinition: %s",tkName->text);
				s=addSymbolToDomain(symTable,newSymbol(tkName->text,SK_STRUCT));

				s->type.tb=TB_STRUCT;
				s->type.s=s;
				s->type.n=-1;

				pushDomain();
				owner=s;

				while(varDef()){}

				if(consume(RACC)){
					if(consume(SEMICOLON)){
						owner=NULL;
						dropDomain();

						return true;
					}
					tkerr("missing ; after struct definition");
				}
				tkerr("missing } in struct definition");
			}
			if(iTk->code == TYPE_INT || iTk->code == TYPE_DOUBLE || iTk->code == TYPE_CHAR || iTk->code == STRUCT || iTk->code == RACC){
				tkerr("missing { after struct name");
			}
			iTk=start;
			return false;
		}
		tkerr("missing struct name");
	}
	iTk=start;
	return false;
}

// unit: ( structDef | fnDef | varDef )* END
bool unit(){
	rule("unit");
	for(;;){
		if(structDef()){}
		else if(fnDef()){}
		else if(varDef()){}
		else break;
		}
	if(consume(END)){
		return true;
		}
	tkerr("syntax error");
	return false;
	}

// stm: stmCompound
// | IF LPAR expr RPAR stm ( ELSE stm )?
// | WHILE LPAR expr RPAR stm
// | RETURN expr? SEMICOLON
// | expr? SEMICOLON	
// domain analysis =>
// stm: stmCompound[true] ...
bool stm(){
	rule("stm");
	Token *start=iTk;
	if(stmCompound(true)){
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
							else{
								tkerr("missing statement after else");
							}
						}
						return true;
					}
					tkerr("missing statement after if condition");
				}
				tkerr("invalid if condition or missing )");
			}
			tkerr("missing condition after if(");
		}
		tkerr("missing ( after if");
	}
	iTk=start;
	if(consume(WHILE)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						return true;
					}
					tkerr("missing statement after while condition");
				}
				tkerr("invalid while condition or missing )");
			}
			tkerr("missing condition after while(");
		}
		tkerr("missing ( after while");
	}
	iTk=start;
	if(consume(RETURN)){
		expr();
		if(consume(SEMICOLON)){
			return true;
		}
		tkerr("missing ; after return");
	}
	iTk=start;
	if(expr()){
		if(consume(SEMICOLON)){
			return true;
		}
		tkerr("invalid statement or missing ;");
	}
	if(consume(SEMICOLON)){
		return true;
	}
	iTk=start;
	return false;
}

// stmCompound: LACC ( varDef | stm )* RACC
// domain analysis =>
// stmCompound[in bool newDomain]: LACC
// {if(newDomain)pushDomain();}
// ( varDef | stm )* RACC
// {if(newDomain)dropDomain();}
bool stmCompound(bool newDomain){
	rule("stmCompound");
	Token *start=iTk;
	
	if(consume(LACC)){
		if(newDomain)pushDomain();
		for(;;){
			if(varDef()){}
			else if(stm()){}
			else break;
		}
		if(consume(RACC)){
			if(newDomain)dropDomain();
			return true;
		}
		tkerr("missing } in compound statement");
	}
	iTk=start;
	return false;
}

//////////
// expr: exprAssign
bool expr(){
	rule("expr");
	return exprAssign();
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign(){
	rule("exprAssign");
	Token *start=iTk;
	if(exprUnary()){
		if(consume(ASSIGN)){
			if(exprAssign()){
				return true;
			}
			tkerr("missing expression after =");
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
	rule("exprOrPrim");
	if(consume(OR)){
		if(exprAnd()){
			if(exprOrPrim()){
				return true;
			}
		}
		tkerr("missing expression after ||");
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
	rule("exprAndPrim");
	if(consume(AND)){
		if(exprEq()){
			if(exprAndPrim()){
				return true;
			}
		}
		tkerr("missing expression after &&");
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
	rule("exprEqPrim");
	if(consume(EQUAL)){
		if(exprRel()){
			if(exprEqPrim()){
				return true;
			}	
		}
		tkerr("missing expression after ==");
	}
	if(consume(NOTEQ)){
		if(exprRel()){
			if(exprEqPrim()){
				return true;
			}	
		}
		tkerr("missing expression after !=");
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
	rule("exprRelPrim");
	if(consume(LESS)){
		if(exprAdd()){
			if(exprRelPrim()){
				return true;
			}
		}
		tkerr("missing expression after <");
	}
	if(consume(LESSEQ)){
		if(exprAdd()){
			if(exprRelPrim()){
				return true;
			}
		}
		tkerr("missing expression after <=");
	}
	if(consume(GREATER)){
		if(exprAdd()){
			if(exprRelPrim()){
				return true;
			}
		}
		tkerr("missing expression after >");
	}
	if(consume(GREATEREQ)){
		if(exprAdd()){
			if(exprRelPrim()){
				return true;
			}
		}
		tkerr("missing expression after >=");
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
	rule("exprAddPrim");
	if(consume(ADD)){
		if(exprMul()){
			if(exprAddPrim()){
				return true;
			}
			
		}
		tkerr("missing expression after +");
	}
	if(consume(SUB)){
		if(exprMul()){
			if(exprAddPrim()){
				return true;
			}
			
		}
		tkerr("missing expression after -");
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
	rule("exprMulPrim");
	if(consume(MUL)){
		if(exprCast()){
			if(exprMulPrim()){
				return true;
			}
		
		}
		tkerr("missing expression after *");
	}
	if(consume(DIV)){
		if(exprCast()){
			if(exprMulPrim()){
				return true;
			}
			
		}
		tkerr("missing expression after /");
	}
	return true;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
// domain analysis =>
// exprCast: LPAR {Type t;} typeBase[&t] arrayDecl[&t]? RPAR exprCast | exprUnary
bool exprCast(){
	rule("exprCast");
	Token *start=iTk;
	if(consume(LPAR)){
		 Type t;
		 if(iTk->code == STRUCT){
            if(!iTk->next || iTk->next->code != ID){
                tkerr("missing struct type name");
            }
        }
		if(typeBase(&t)){
			arrayDecl(&t);
			if(consume(RPAR)){
				if(exprCast()){
					return true;
				}
				tkerr("missing expression after cast");
			}
			tkerr("missing ) after cast type");
		}
		iTk=start;
	}
	if(exprUnary()){
		return true;
	}
	iTk=start;
	return false;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(){
	rule("exprUnary");
	Token *start=iTk;
	if(consume(SUB)){
		if(exprUnary()){
			return true;
		}
		tkerr("missing expression after unary -");
	}
	iTk=start;
	if(consume(NOT)){
		if(exprUnary()){
			return true;
		}
		tkerr("missing expression after unary !");
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
	rule("exprPostfixPrim");
	if(consume(LBRACKET)){
		if(expr()){
			if(consume(RBRACKET)){
				if(exprPostfixPrim()){
					return true;
				}
			}
			tkerr("missing ] after index expression");
		}
		tkerr("missing expression after [");
	}
	if(consume(DOT)){
		if(consume(ID)){
			if(exprPostfixPrim()){
				return true;
			}
		}
		tkerr("missing field name after .");
	}
	return true;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
// | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(){
	rule("exprPrimary");
	Token *start=iTk;
	if(consume(ID)){
		if(consume(LPAR)){
			if(expr()){
				while(consume(COMMA)){
					if(expr()){}
					else{
						tkerr("missing expression after ,");
					}
				}
			}
			if(consume(RPAR)){
				return true;
			}
			tkerr("missing ) after function call");
		}
		return true;
	}
	iTk=start;
	if(consume(INT)){
		return true;
	}
	iTk=start;
	if(consume(DOUBLE)){
		return true;
	}
	iTk=start;
	if(consume(CHAR)){
		return true;
	}
	iTk=start;
	if(consume(STRING)){
		return true;
	}
	iTk=start;
	if(consume(LPAR)){
		if(expr()){
			if(consume(RPAR)){
				return true;
			}
			tkerr("missing ) after expression");
		}
		tkerr("missing expression after (");
	}
	iTk=start;
	return false;
}

void parse(Token *tokens){
	iTk=tokens;
	if(!unit())tkerr("syntax error");
	}
