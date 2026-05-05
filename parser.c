#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;
Token *consumedTk;

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
    if(consume(TYPE_INT))    return true;
    if(consume(TYPE_DOUBLE)) return true;
    if(consume(TYPE_CHAR))   return true;
    if(consume(STRUCT)){
        if(consume(ID)) return true;
        tkerr("identifier missing after struct");
    }
    return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef() {
    Token *start = iTk;
    if (typeBase()) {
        if (consume(ID)) {
            arrayDecl();
            if (consume(SEMICOLON)) {
                return true;
            } else tkerr("Missing ; after variable definition");
        }
		else tkerr("Missing ID after type or invalid struct/function declaration");
    }
    iTk = start;
    return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl() {
    Token *start = iTk;
    if (consume(LBRACKET)) {
        consume(INT);
        if (consume(RBRACKET)) {
            return true;
        } else tkerr("Missing ] in array declaration");
    }
    iTk = start;
    return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef() {
    Token *start = iTk;
    if (consume(STRUCT)) {
        if (consume(ID)) {
            if (consume(LACC)) {
                while (varDef()) {} 
                if (consume(RACC)) {
                    if (consume(SEMICOLON)) {
                        return true;
                    } else tkerr("Missing ; after struct definition");
                } else tkerr("Missing } in struct definition");
			} 
		} 
		else tkerr("Missing ID after struct");
    }
    iTk = start;
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
		tkerr("identifier missing in function parameter");
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
					if(!expr()) tkerr("expression missing after ,");
					}
				}
			if(consume(RPAR)) return true;
			tkerr(") missing in function call");
		}
		return true;
	}
	if(consume(INT))    return true;
	if(consume(DOUBLE)) return true;
	if(consume(CHAR))   return true;
	if(consume(STRING)) return true;
	if(consume(LPAR)){
		if(expr()){
			if(consume(RPAR)) return true;
			tkerr(") missing after expression");
		}
		tkerr("invalid expression after (");
	}
	iTk=start;
	return false;
}

// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | epsilon
bool exprPostfixPrim(){
	if(consume(LBRACKET)){
		if(expr()){
			if(consume(RBRACKET)) return exprPostfixPrim();
			tkerr("] missing in indexing");
		}
		tkerr("invalid expression in indexing");
	}
	if(consume(DOT)){
		if(consume(ID)) return exprPostfixPrim();
		tkerr("field name missing after .");
	}
	return true;
}

// exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix(){
	if(exprPrimary()) return exprPostfixPrim();
	return false;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(){
	if(consume(SUB)||consume(NOT)){
		if(exprUnary()) return true;
		tkerr("expression missing after unary operator");
	}
	return exprPostfix();
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(){
	Token *start=iTk;
	if(consume(LPAR)){
		if(typeBase()){
			arrayDecl();
			if(consume(RPAR)){
				if(exprCast()) return true;
				tkerr("expression missing after cast");
			}
			tkerr(") missing in cast expression");
		}
	}
	iTk=start;
	return exprUnary();
}

// exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | epsilon
bool exprMulPrim(){
	if(consume(MUL)||consume(DIV)){
		if(exprCast()) return exprMulPrim();
		tkerr("expression missing after * or /");
	}
	return true;
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
		tkerr("expression missing after + or -");
	}
	return true;
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
		tkerr("expression missing after relational operator");
	}
	return true;
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
		tkerr("expression missing after == or !=");
	}
	return true;
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
		tkerr("expression missing after &&");
		}
	return true;
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
		tkerr("expression missing after ||");
	}
	return true;
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
			tkerr("expression missing after =");
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

// stm: stmCompound | IF LPAR expr RPAR stm ( ELSE stm )? | WHILE LPAR expr RPAR stm | RETURN expr? SEMICOLON | expr? SEMICOLON
bool stm(){
	Token *start=iTk;

	if(stmCompound()) return true;

	if(consume(IF)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						if(consume(ELSE)){
							if(!stm()) tkerr("statement missing after else");
						}
						return true;
					}
					tkerr("statement missing for if body");
				}
				tkerr(") missing after if condition");
			}
			tkerr("invalid or missing condition in if");
		}
		tkerr("( missing after if");
	}

	if(consume(WHILE)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()) return true;
					tkerr("statement missing for while body");
				}
				tkerr(") missing after while condition");
			}
			tkerr("invalid or missing condition in while");
		}
		tkerr("( missing after while");
	}

	if(consume(RETURN)){
		expr();
		if(consume(SEMICOLON)) return true;
		tkerr("; missing after return");
	}

	expr();
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
					if(!fnParam()) tkerr("parameter missing after ,");
				}
			}
			if(consume(RPAR)){
				if(stmCompound()) return true;
				tkerr("function body { } missing");
			}
			tkerr(") missing in function definition");
		}
		iTk=start;
		return false;
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
	if(consume(END)) return true;
	tkerr("unexpected token at end of file");
	return false;
}

void parse(Token *tokens){
	iTk=tokens;
	if(!unit()) tkerr("syntax error");
}