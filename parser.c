#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;// the iterator in the tokens list
Token *consumedTk;// the last consumed token

bool structDef();
bool fnDef();
bool varDef();
bool stmCompound();
bool stm();
bool expr();
bool exprAssign();
bool exprOr();
bool exprOrPrim();
bool exprAnd();
bool exprAndPrim();
bool exprEq();
bool exprEqPrim();
bool exprRel();
bool exprRelPrim();
bool exprAdd();
bool exprAddPrim();
bool exprMul();
bool exprMulPrim();
bool exprCast();
bool exprUnary();
bool exprPostfix();
bool exprPostfixPrim();
bool exprPrimary();
bool typeBase();
bool arrayDecl();
bool fnParam();

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
	if(consume(TYPE_INT)) return true;
	if(consume(TYPE_DOUBLE)) return true;
	if(consume(TYPE_CHAR)) return true;
	if(consume(STRUCT)){
		if(consume(ID)) return true;
		tkerr("Identifier missing after struct.\n");}
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

//arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl(){
	Token *start=iTk;
	if(consume(LBRACKET)){
		consume(INT);
		if(consume(RBRACKET)) return true;
		tkerr("] missing.\n");
	}
	iTk = start;
	return false;
}

//var declaration typeBase ID arrayDecl? SEMICOLON
bool varDef(){
	Token *start = iTk;
	if(typeBase()){
		if(consume(ID)){
			arrayDecl();
			if(consume(SEMICOLON)) return true;
			tkerr("; missing after variable declaration.\n");
		}
	}
	iTk=start;
	return false;
}

//struct def: struct id lacc varDef* RACC SEMICOLON
bool structDef() {
    Token *start = iTk;
    if (consume(STRUCT)) {
        if (consume(ID)) {
            if (consume(LACC)) {
                while (varDef()) {}
                if (consume(RACC)) {
                    if (consume(SEMICOLON)) return true;
                    tkerr("; missing after }");
                }
                tkerr("} missing in struct.\n");
            }
        }
    }
    iTk = start;
    return false;
}

//// fnParam: typeBase ID arrayDecl?
bool fnParam(){
	Token *start = iTk;
	if(typeBase()){
		if(consume(ID)){
			arrayDecl();
			return true;
		}
	}
	iTk=start;
	return false;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
//            | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(){
	Token *start=iTk;
	if(consume(ID)){
		if(consume(LPAR)){
			if(expr()){
				while(consume(COMMA)){
					if(!expr()) tkerr("invalid expression after ','.\n");
				}
			}
			if(consume(RPAR)) return true;
			tkerr("')' missing in function call.\n");
		}
		return true;
	}
	if(consume(INT)) return true;
	if(consume(DOUBLE)) return true;
	if(consume(CHAR)) return true;
	if(consume(STRING)) return true;
	if(consume(LPAR)){
		if(expr()){
			if(consume(RPAR)) return true;
			tkerr("')' missing after expression.\n");
		}
		tkerr("invalid expression after '('.\n");
	}
	iTk=start;
	return false;
}

// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim
//               | DOT ID exprPostfixPrim
//               | epsilon
bool exprPostfixPrim(){
	if(consume(LBRACKET)){
		if(expr()){
			if(consume(RBRACKET)) return exprPostfixPrim();
			tkerr("']' missing in indexing.\n");
		}
		tkerr("invalid expression in indexing.\n");
	}
	if(consume(DOT)){
		if(consume(ID)) return exprPostfixPrim();
		tkerr("field name missing after '.'.\n");
	}
	return true; 
}

// exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix(){
    if(exprPrimary()) return exprPostfixPrim();
    return false;
}


bool exprUnary(){
	if(consume(SUB)||consume(NOT)){
		if(exprUnary()) return true;
		tkerr("invalid expression after unary operator");
	}
	return exprPostfix();
}
 
// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(){
	Token *start=iTk;
	if(consume(LPAR)){
		if(typeBase()){
			arrayDecl(); // optional
			if(consume(RPAR)){
				if(exprCast()) return true;
				tkerr("invalid expression after cast");
			}
		}
	}
	iTk=start;
	return exprUnary();
}
 
// exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | epsilon
bool exprMulPrim(){
	if(consume(MUL)||consume(DIV)){
		if(exprCast()) return exprMulPrim();
		tkerr("invalid expression after * or /");
	}
	return true; // epsilon
	}
 
// exprMul: exprCast exprMulPrim
bool exprMul(){
	if(exprCast()) return exprMulPrim();
	return false;
}
 
// exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | epsilon
bool exprAddPrim(){
	if(consume(ADD)||consume(SUB)){
		if(exprMul()) return exprAddPrim();
		tkerr("invalid expression after + or -");
	}
	return true; // epsilon
	}
 
// exprAdd: exprMul exprAddPrim
bool exprAdd(){
	if(exprMul()) return exprAddPrim();
	return false;
}
 
// exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | epsilon
bool exprRelPrim(){
	if(consume(LESS)||consume(LESSEQ)||consume(GREATER)||consume(GREATEREQ)){
		if(exprAdd()) return exprRelPrim();
		tkerr("invalid expression after relational operator");
	}
	return true; // epsilon
}
 
// exprRel: exprAdd exprRelPrim
bool exprRel(){
	if(exprAdd()) return exprRelPrim();
	return false;
}
 
// exprEqPrim: ( EQUAL | NOTEQ ) exprRel exprEqPrim | epsilon
bool exprEqPrim(){
	if(consume(EQUAL)||consume(NOTEQ)){
		if(exprRel()) return exprEqPrim();
		tkerr("invalid expression after == or !=");
	}
	return true; // epsilon
}
 
// exprEq: exprRel exprEqPrim
bool exprEq(){
	if(exprRel()) return exprEqPrim();
	return false;
}
 
// exprAndPrim: AND exprEq exprAndPrim | epsilon
bool exprAndPrim(){
	if(consume(AND)){
		if(exprEq()) return exprAndPrim();
		tkerr("invalid expression after &&");
	}
	return true; // epsilon
}
 
// exprAnd: exprEq exprAndPrim
bool exprAnd(){
	if(exprEq()) return exprAndPrim();
	return false;
}
 
// exprOrPrim: OR exprAnd exprOrPrim | epsilon
bool exprOrPrim(){
	if(consume(OR)){
		if(exprAnd()) return exprOrPrim();
		tkerr("invalid expression after ||");
	}
	return true; // epsilon
}
 
// exprOr: exprAnd exprOrPrim
bool exprOr(){
	if(exprAnd()) return exprOrPrim();
	return false;
}
 
// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign(){
	Token *start=iTk;
	if(exprUnary()){
		if(consume(ASSIGN)){
			if(exprAssign()) return true;
			tkerr("invalid expression after =");
		}
	}
	iTk=start;
	return exprOr();
}
 
// expr: exprAssign
bool expr(){
	return exprAssign();
}
 
// stmCompound: LACC ( varDef | stm )* RACC
bool stmCompound(){
	Token *start=iTk;
	if(consume(LACC)){
		for(;;){
			if(varDef()){}
			else if(stm()){}
			else break;
		}
		if(consume(RACC)) return true;
		tkerr("} missing at end of block");
	}
	iTk=start;
	return false;
}
 
// stm: stmCompound
//    | IF LPAR expr RPAR stm ( ELSE stm )?
//    | WHILE LPAR expr RPAR stm
//    | RETURN expr? SEMICOLON
//    | expr? SEMICOLON
bool stm(){
	Token *start=iTk;
 
	if(stmCompound()) return true;
 
	if(consume(IF)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						if(consume(ELSE)){
							if(!stm()) tkerr("invalid statement after else");
						}
						return true;
					}
					tkerr("invalid statement for if");
				}
				tkerr(") missing after if condition");
			}
			tkerr("invalid condition for if");
		}
		tkerr("( missing after if");
	}
 
	if(consume(WHILE)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()) return true;
					tkerr("invalid statement for while.\n");
				}
				tkerr("')' missing after while condition.\n");
			}
			tkerr("invalid condition for while.\n");
		}
		tkerr("'(' missing after while.\n");
	}
 
	if(consume(RETURN)){
		expr(); // optional
		if(consume(SEMICOLON)) return true;
		tkerr("';' missing after return.\n");
	}
 
	// expr? SEMICOLON
	expr(); // optional
	if(consume(SEMICOLON)) return true;
 
	iTk=start;
	return false;
}
 
// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
bool fnDef(){
	Token *start=iTk;
	bool hasType=typeBase();
	if(!hasType&&!consume(VOID)){
		iTk=start;
		return false;
	}
	if(consume(ID)){
		if(consume(LPAR)){
			if(fnParam()){
				while(consume(COMMA)){
					if(!fnParam()) tkerr("invalid parameter after ','.\n");
				}
			}
			if(consume(RPAR)){
				if(stmCompound()) return true;
				tkerr("function body missing.\n");
			}
			tkerr("')' missing in function definition.\n");
		}
	}
	iTk=start;
	return false;
}
 
void parse(Token *tokens){
	iTk=tokens;
	if(!unit())tkerr("syntax error.\n");
}
