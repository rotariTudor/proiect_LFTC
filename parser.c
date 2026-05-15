#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "utils.h"
#include "at.h"
#include "ad.c"

Token *iTk;
Token *consumedTk;

// variabila globala owner: functia sau structura in interiorul careia suntem
Symbol *owner = NULL;

bool structDef();
bool fnDef();
bool varDef();
bool stmCompound(bool newDomain);
bool stm();
bool expr(Ret *r);
bool exprAssign(Ret *r);
bool exprOr(Ret *r);
bool exprOrPrim(Ret *r);
bool exprAnd(Ret *r);
bool exprAndPrim(Ret *r);
bool exprEq(Ret *r);
bool exprEqPrim(Ret *r);
bool exprRel(Ret *r);
bool exprRelPrim(Ret *r);
bool exprAdd(Ret *r);
bool exprAddPrim(Ret *r);
bool exprMul(Ret *r);
bool exprMulPrim(Ret *r);
bool exprCast(Ret *r);
bool exprUnary(Ret *r);
bool exprPostfix(Ret *r);
bool exprPostfixPrim(Ret *r);
bool exprPrimary(Ret *r);
bool typeBase(Type *t);
bool arrayDecl(Type *t);
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
// AD: seteaza campurile din *t
bool typeBase(Type *t){
	t->n=-1;
	if(consume(TYPE_INT))    { t->tb=TB_INT; return true; }
	if(consume(TYPE_DOUBLE)) { t->tb=TB_DOUBLE; return true; }
	if(consume(TYPE_CHAR))   { t->tb=TB_CHAR; return true; }
	if(consume(STRUCT)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			t->tb=TB_STRUCT;
			t->s=findSymbol(tkName->text);
			if(!t->s) tkerr("undefined struct: %s",tkName->text);
			return true;
		}
		tkerr("identifier missing after struct");
	}
	return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
// AD: seteaza t->n (dimensiunea sau 0 daca lipseste)
bool arrayDecl(Type *t){
	Token *start=iTk;
	if(consume(LBRACKET)){
		if(consume(INT)){
			Token *tkSize=consumedTk;
			t->n=tkSize->i;
		} else {
			t->n=0;
		}
		if(consume(RBRACKET)){
			return true;
		} else tkerr("Missing ] in array declaration");
	}
	iTk=start;
	return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
// AD: verifica redefinire, adauga simbol in TS
bool varDef(){
	Token *start=iTk;
	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(arrayDecl(&t)){
				if(t.n==0) tkerr("a vector variable must have a specified dimension");
			}
			if(consume(SEMICOLON)){
				Symbol *var=findSymbolInDomain(symTable,tkName->text);
				if(var) tkerr("symbol already exists: %s",tkName->text);
				var=newSymbol(tkName->text,SK_VAR);
				var->type=t;
				var->owner=owner;
				addSymbolToDomain(symTable,var);
				if(owner){
					switch(owner->kind){
						case SK_FN:
							var->varIdx=symbolsLen(owner->fn.locals);
							//calculeaza indexu
							addSymbolToList(&owner->fn.locals,dupSymbol(var));
							break;
						case SK_STRUCT:
							var->varIdx=typeSize(&owner->type);
							//calculeaza offsetu
							addSymbolToList(&owner->structMembers,dupSymbol(var));
							break;
						default: break;
					}
				} else {
					var->varMem=safeAlloc(typeSize(&t));
				}
				return true;
			} else tkerr("Missing ; after variable definition");
		}
		else tkerr("Missing Identificator after type or invalid struct/function declaration");
	}
	iTk=start;
	return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
// AD: verifica redefinire, adauga SK_STRUCT, pushDomain, owner=s
bool structDef(){
	Token *start=iTk;
	if(consume(STRUCT)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(consume(LACC)){
				Symbol *s=findSymbolInDomain(symTable,tkName->text);
				if(s) tkerr("symbol redefinition: %s",tkName->text);
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
					} else tkerr("Missing ; after struct definition");
				} else tkerr("Missing } in struct definition");
			}
		}
		else tkerr("Missing ID after struct");
	}
	iTk=start;
	return false;
}

// fnParam: typeBase ID arrayDecl?
// AD: verifica redefinire, adauga SK_PARAM in domeniu si in fn.params
bool fnParam(){
	Token *start=iTk;
	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(arrayDecl(&t)){
				t.n=0;	// parametrii vectori devin fara dimensiune specificata
			}
			Symbol *param=findSymbolInDomain(symTable,tkName->text);
			if(param) tkerr("symbol redefinition: %s",tkName->text);
			param=newSymbol(tkName->text,SK_PARAM);
			param->type=t;
			param->owner=owner;
			param->paramIdx=symbolsLen(owner->fn.params);
			addSymbolToDomain(symTable,param);
			addSymbolToList(&owner->fn.params,dupSymbol(param));
			return true;
		}
		tkerr("identifier missing in function parameter");
	}
	iTk=start;
	return false;
}


bool exprPrimary(Ret *r){
	Token *start=iTk;
	if(consume(ID)){
		Token *tkName=consumedTk;
		Symbol *s=findSymbol(tkName->text);
		if(!s) tkerr("undefined id: %s",tkName->text);
		if(consume(LPAR)){
			if(s->kind!=SK_FN) tkerr("only a function can be called");
			Ret rArg;
			Symbol *param=s->fn.params;
			if(expr(&rArg)){
				if(!param) tkerr("too many arguments in function call");
				if(!convTo(&rArg.type,&param->type))
					tkerr("in call, cannot convert the argument type to the parameter type");
				param=param->next;
				while(consume(COMMA)){
					if(!expr(&rArg)) tkerr("expression missing after ,");
					if(!param) tkerr("too many arguments in function call");
					if(!convTo(&rArg.type,&param->type))
						tkerr("in call, cannot convert the argument type to the parameter type");
					param=param->next;
				}
			}
			if(param) tkerr("too few arguments in function call");
			if(consume(RPAR)){
				*r=(Ret){s->type,false,true};
				return true;
			}
			tkerr(") missing in function call");
		}
		// nu e apel de functie - e variabila/parametru
		if(s->kind==SK_FN) tkerr("a function can only be called");
		*r=(Ret){s->type,true,s->type.n>=0};
		return true;
	}
	if(consume(INT)){
		*r=(Ret){{TB_INT,NULL,-1},false,true};
		return true;
	}
	if(consume(DOUBLE)){
		*r=(Ret){{TB_DOUBLE,NULL,-1},false,true};
		return true;
	}
	if(consume(CHAR)){
		*r=(Ret){{TB_CHAR,NULL,-1},false,true};
		return true;
	}
	if(consume(STRING)){
		*r=(Ret){{TB_CHAR,NULL,0},false,true};
		return true;
	}
	if(consume(LPAR)){
		if(expr(r)){
			if(consume(RPAR)) return true;
			tkerr(") missing after expression");
		}
		tkerr("invalid expression after (");
	}
	iTk=start;
	return false;
}


bool exprPostfixPrim(Ret *r){
	if(consume(LBRACKET)){
		Ret idx;
		if(expr(&idx)){
			if(r->type.n<0) tkerr("only an array can be indexed");
			Type tInt={TB_INT,NULL,-1};
			if(!convTo(&idx.type,&tInt)) tkerr("the index is not convertible to int");
			r->type.n=-1;
			r->lval=true;
			r->ct=false;
			if(consume(RBRACKET)) return exprPostfixPrim(r);
			tkerr("] missing in indexing");
		}
		tkerr("invalid expression in indexing");
	}
	if(consume(DOT)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(r->type.tb!=TB_STRUCT) tkerr("a field can only be selected from a struct");
			Symbol *s=findSymbolInList(r->type.s->structMembers,tkName->text);
			if(!s) tkerr("the structure %s does not have a field %s",r->type.s->name,tkName->text);
			*r=(Ret){s->type,true,s->type.n>=0};
			return exprPostfixPrim(r);
		}
		tkerr("field name missing after .");
	}
	return true;
}

// exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix(Ret *r){
	if(exprPrimary(r)) return exprPostfixPrim(r);
	return false;
}


bool exprUnary(Ret *r){
	if(consume(SUB)){
		if(exprUnary(r)){
			if(!canBeScalar(r)) tkerr("unary - or ! must have a scalar operand");
			r->lval=false;
			r->ct=true;
			return true;
		}
		tkerr("expression missing after unary operator (SUB)");
	}else if(consume(NOT)){
		if(exprUnary(r)){
			if(!canBeScalar(r)) tkerr("unary - or ! must have a scalar operand");
			r->lval=false;
			r->ct=true;
			return true;
		}
		tkerr("expression missing after unary operator(NOT)");
	}
	return exprPostfix(r);
}

// AT: exprCast[out Ret *r]
// LPAR typeBase[&t] arrayDecl[&t]? RPAR exprCast[&op] {
//   if(t.tb==TB_STRUCT) tkerr("cannot convert to a struct type");
//   if(op.type.tb==TB_STRUCT) tkerr("cannot convert a struct");
//   if(op.type.n>=0&&t.n<0) tkerr("an array can be converted only to another array");
//   if(op.type.n<0&&t.n>=0) tkerr("a scalar can be converted only to another scalar");
//   *r={t,false,true};
// }
// | exprUnary[r]
bool exprCast(Ret *r){
	Token *start=iTk;
	if(consume(LPAR)){
		Type t;
		Ret op;
		if(typeBase(&t)){
			arrayDecl(&t);
			if(consume(RPAR)){
				if(exprCast(&op)){
					if(t.tb==TB_STRUCT) tkerr("cannot convert to a struct type");
					if(op.type.tb==TB_STRUCT) tkerr("cannot convert a struct");
					if(op.type.n>=0&&t.n<0) tkerr("an array can be converted only to another array");
					if(op.type.n<0&&t.n>=0) tkerr("a scalar can be converted only to another scalar");
					*r=(Ret){t,false,true};
					return true;
				}
				tkerr("expression missing after cast");
			}
			tkerr(") missing in cast expression");
		}
	}
	iTk=start;
	return exprUnary(r);
}

bool exprMulPrim(Ret *r){
	if(consume(MUL)){
		Ret right;
		if(exprCast(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for * or /");
			*r=(Ret){tDst,false,true};
			return exprMulPrim(r);
		}
		tkerr("expression missing after *");
	}
	else if(consume(DIV)){
		Ret right;
		if(exprCast(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for * or /");
			*r=(Ret){tDst,false,true};
			return exprMulPrim(r);
		}
		tkerr("expression missing after /");
	}
	return true;
}


bool exprMul(Ret *r){
	if(exprCast(r)) return exprMulPrim(r);
	return false;
}


bool exprAddPrim(Ret *r){
	if(consume(ADD)){
		Ret right;
		if(exprMul(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for + or -");
			*r=(Ret){tDst,false,true};
			return exprAddPrim(r);
		}
		tkerr("expression missing after +");
	}else if(consume(SUB)){
		Ret right;
		if(exprMul(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for + or -");
			*r=(Ret){tDst,false,true};
			return exprAddPrim(r);
		}
		tkerr("expression missing after -");
	}
	return true;
}

// exprAdd: exprMul exprAddPrim
bool exprAdd(Ret *r){
	if(exprMul(r)) return exprAddPrim(r);
	return false;
}


bool exprRelPrim(Ret *r){
	if(consume(LESS)||consume(LESSEQ)||consume(GREATER)||consume(GREATEREQ)){
		Ret right;
		if(exprAdd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for <, <=, >, >=");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			return exprRelPrim(r);
		}
		tkerr("expression missing after relational operator");
	}
	return true;
}

// exprRel: exprAdd exprRelPrim
bool exprRel(Ret *r){
	if(exprAdd(r)) return exprRelPrim(r);
	return false;
}


bool exprEqPrim(Ret *r){
	if(consume(EQUAL)||consume(NOTEQ)){
		Ret right;
		if(exprRel(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for == or !=");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			return exprEqPrim(r);
		}
		tkerr("expression missing after == or !=");
	}
	return true;
}

// exprEq: exprRel exprEqPrim
bool exprEq(Ret *r){
	if(exprRel(r)) return exprEqPrim(r);
	return false;
}


bool exprAndPrim(Ret *r){
	if(consume(AND)){
		Ret right;
		if(exprEq(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for &&");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			return exprAndPrim(r);
		}
		tkerr("expression missing after &&");
		}
	return true;
}

// exprAnd: exprEq exprAndPrim
bool exprAnd(Ret *r){
	if(exprEq(r)) return exprAndPrim(r);
	return false;
}


bool exprOrPrim(Ret *r){
	if(consume(OR)){
		Ret right;
		if(exprAnd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for ||");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			return exprOrPrim(r);
		}
		tkerr("expression missing after ||");
	}
	return true;
}

// exprOr: exprAnd exprOrPrim
bool exprOr(Ret *r){
	if(exprAnd(r)) return exprOrPrim(r);
	return false;
}


bool exprAssign(Ret *r){
	Token *start=iTk;
	Ret rDst;
	if(exprUnary(&rDst)){
		if(consume(ASSIGN)){
			if(exprAssign(r)){
				if(!rDst.lval) tkerr("the assign destination must be a left-value");
				if(rDst.ct) tkerr("the assign destination cannot be constant");
				if(!canBeScalar(&rDst)) tkerr("the assign destination must be scalar");
				if(!canBeScalar(r)) tkerr("the assign source must be scalar");
				if(!convTo(&r->type,&rDst.type)) tkerr("the assign source cannot be converted to destination");
				r->lval=false;
				r->ct=true;
				return true;
			}
			tkerr("expression missing after =");
		}
	}
	iTk=start;
	return exprOr(r);
}

// expr: exprAssign
bool expr(Ret *r){
	return exprAssign(r);
}

// stmCompound: LACC ( varDef | stm )* RACC
// AD: parametru newDomain controleaza daca se creeaza un domeniu nou
bool stmCompound(bool newDomain){
	Token *start=iTk;
	if(consume(LACC)){
		if(newDomain) pushDomain();
		for(;;){
			if(varDef()){}
			else if(stm()){}
			else break;
		}
		if(consume(RACC)){
			if(newDomain) dropDomain();
			return true;
		}
		tkerr("} missing at end of block");
	}
	iTk=start;
	return false;
}


bool stm(){
	Token *start=iTk;
	Ret rCond,rExpr;

	if(stmCompound(true)) return true;

	if(consume(IF)){
		if(consume(LPAR)){
			if(expr(&rCond)){
				if(!canBeScalar(&rCond)) tkerr("the if condition must be a scalar value");
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
			if(expr(&rCond)){
				if(!canBeScalar(&rCond)) tkerr("the while condition must be a scalar value");
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
		if(expr(&rExpr)){
			if(owner->type.tb==TB_VOID) tkerr("a void function cannot return a value");
			if(!canBeScalar(&rExpr)) tkerr("the return value must be a scalar value");
			if(!convTo(&rExpr.type,&owner->type)) tkerr("cannot convert the return expression type to the function return type");
		} else {
			if(owner->type.tb!=TB_VOID) tkerr("a non-void function must return a value");
		}
		if(consume(SEMICOLON)) return true;
		tkerr("; missing after return");
	}

	expr(&rExpr);
	if(consume(SEMICOLON)) return true;

	iTk=start;
	return false;
}

// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
// AD: verifica redefinire, adauga SK_FN, owner=fn, pushDomain
//     stmCompound apelat cu false (corpul fn nu adauga domeniu nou)
bool fnDef(){
	Token *start=iTk;
	Type t;
	bool hasType=typeBase(&t);
	if(!hasType){
		if(consume(VOID)){
			t.tb=TB_VOID;
			t.n=-1;
			t.s=NULL;
		} else {
			iTk=start;
			return false;
		}
	}
	if(consume(ID)){
		Token *tkName=consumedTk;
		if(consume(LPAR)){
			Symbol *fn=findSymbolInDomain(symTable,tkName->text);
			if(fn) tkerr("symbol redefinition: %s",tkName->text);
			fn=newSymbol(tkName->text,SK_FN);
			fn->type=t;
			addSymbolToDomain(symTable,fn);
			owner=fn;
			pushDomain();

			if(fnParam()){
				while(consume(COMMA)){
					if(!fnParam()) tkerr("parameter missing or invalid after ,");
				}
			}
			// else{
			// 	tkerr("invalid or missing parameter");
			// }
			if(consume(RPAR)){
				if(stmCompound(false)) {
					dropDomain();
					owner=NULL;
					return true;
				}
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
	pushDomain();
	if(!unit()) tkerr("syntax error");
	showDomain(symTable,"global");
	dropDomain();
}