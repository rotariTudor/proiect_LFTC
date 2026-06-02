#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "utils.h"
#include "at.h"
#include "ad.c"
#include "gc.h"

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
bool typeBase(Type *t){
	t->n=-1;
    Token *start=iTk;
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
    iTk=start;
	return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
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
							addSymbolToList(&owner->fn.locals,dupSymbol(var));
							break;
						case SK_STRUCT:
							var->varIdx=typeSize(&owner->type);
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

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
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
bool fnParam(){
	Token *start=iTk;
	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName=consumedTk;
			if(arrayDecl(&t)){
				t.n=0;
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
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
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
				if(!convTo(&rArg.type,&param->type)) tkerr("in call, cannot convert the argument type to the parameter type");
                
                addRVal(&owner->fn.instr, rArg.lval, &rArg.type);
                insertConvIfNeeded(lastInstr(owner->fn.instr), &rArg.type, &param->type);
				
                param=param->next;
				while(consume(COMMA)){
					if(!expr(&rArg)) tkerr("expression missing after ,");
					if(!param) tkerr("too many arguments in function call");
					if(!convTo(&rArg.type,&param->type)) tkerr("in call, cannot convert the argument type to the parameter type");
					
                    addRVal(&owner->fn.instr, rArg.lval, &rArg.type);
                    insertConvIfNeeded(lastInstr(owner->fn.instr), &rArg.type, &param->type);
                    
                    param=param->next;
				}
			}
			if(param) tkerr("too few arguments in function call");
			if(consume(RPAR)){
				*r=(Ret){s->type,false,true};
                if (s->fn.extFnPtr) {
                    addInstr(&owner->fn.instr, OP_CALL_EXT)->arg.extFnPtr = s->fn.extFnPtr;
                } else {
                    addInstr(&owner->fn.instr, OP_CALL)->arg.instr = s->fn.instr;
                }
				return true;
			}
			tkerr(") missing in function call");
		}
		if(s->kind==SK_FN) tkerr("a function can only be called");
		*r=(Ret){s->type,true,s->type.n>=0};

        if (s->kind == SK_VAR) {
            if (s->owner == NULL) { 
                addInstr(&owner->fn.instr, OP_ADDR)->arg.p = s->varMem;
            } else { 
                switch (s->type.tb) {
                    case TB_INT: addInstrWithInt(&owner->fn.instr, OP_FPADDR_I, s->varIdx + 1); break;
                    case TB_DOUBLE: addInstrWithInt(&owner->fn.instr, OP_FPADDR_F, s->varIdx + 1); break;
                }
            }
        }
        if (s->kind == SK_PARAM) {
            switch (s->type.tb) {
                case TB_INT:
                    addInstrWithInt(&owner->fn.instr, OP_FPADDR_I, s->paramIdx - symbolsLen(s->owner->fn.params) - 1); break;
                case TB_DOUBLE:
                    addInstrWithInt(&owner->fn.instr, OP_FPADDR_F, s->paramIdx - symbolsLen(s->owner->fn.params) - 1); break;
            }
        }
		return true;
	}
	if(consume(INT)){
		*r=(Ret){{TB_INT,NULL,-1},false,true};
        addInstrWithInt(&owner->fn.instr, OP_PUSH_I, consumedTk->i);
		return true;
	}
	if(consume(DOUBLE)){
		*r=(Ret){{TB_DOUBLE,NULL,-1},false,true};
        addInstrWithDouble(&owner->fn.instr, OP_PUSH_F, consumedTk->d);
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
    }
    iTk=start;
    if(owner) delInstrAfter(startInstr);
    return false;
}

bool exprPostfixPrim(Ret *r){
    Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
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

bool exprPostfix(Ret *r){
    Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	if(exprPrimary(r)) {
        if(exprPostfixPrim(r)) return true;
    }
    iTk = start;
    if (owner) delInstrAfter(startInstr);
	return false;
}

bool exprUnary(Ret *r){
    Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	if(consume(SUB)){
		if(exprUnary(r)){
			if(!canBeScalar(r)) tkerr("unary - must have a scalar operand");
			r->lval=false;
			r->ct=true;
			return true;
		}
		tkerr("expression missing after unary operator (SUB)");
	}else if(consume(NOT)){
		if(exprUnary(r)){
			if(!canBeScalar(r)) tkerr("unary ! must have a scalar operand");
			r->lval=false;
			r->ct=true;
			return true;
		}
		tkerr("expression missing after unary operator(NOT)");
	}
    iTk = start;
    if (owner) delInstrAfter(startInstr);
	if (exprPostfix(r)) return true;

    iTk = start;
    if (owner) delInstrAfter(startInstr);
    return false;
}

bool exprCast(Ret *r){
	Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	if(consume(LPAR)){
		Type t;
		Ret op;
		if(typeBase(&t)){
			arrayDecl(&t);
			if(consume(RPAR)){
				if(exprCast(&op)){
					if(t.tb==TB_STRUCT) tkerr("cannot convert to a struct type");
					if(op.type.n>=0&&t.n<0) tkerr("an array can be converted only to another array");
					if(op.type.n<0&&t.n>=0) tkerr("a scalar can be converted only to another scalar");
					if(op.type.tb==TB_STRUCT) tkerr("cannot convert a struct");
					*r=(Ret){t,false,true};
					return true;
				}
				tkerr("expression missing after cast");
			}
			tkerr(") missing in cast expression");
		}
	}
	iTk=start;
    if(owner) delInstrAfter(startInstr);
	if (exprUnary(r)) return true;

    iTk=start;
    if(owner) delInstrAfter(startInstr);
    return false;
}

bool exprMulPrim(Ret *r){
	if(consume(MUL) || consume(DIV)){
        Token *op = consumedTk;
        Instr *lastLeft = owner ? lastInstr(owner->fn.instr) : NULL;
        addRVal(&owner->fn.instr, r->lval, &r->type);

		Ret right;
		if(exprCast(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) {
                if (op->code == MUL) tkerr("invalid operand type for *");
                else tkerr("invalid operand type for /");
            }

            addRVal(&owner->fn.instr, right.lval, &right.type);
            insertConvIfNeeded(lastLeft, &r->type, &tDst);
            insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);

            if(op->code == MUL) {
                switch(tDst.tb){
                    case TB_INT: addInstr(&owner->fn.instr, OP_MUL_I); break;
                    case TB_DOUBLE: addInstr(&owner->fn.instr, OP_MUL_F); break;
                }
            } else {
                switch(tDst.tb){
                    case TB_INT: addInstr(&owner->fn.instr, OP_DIV_I); break;
                    case TB_DOUBLE: addInstr(&owner->fn.instr, OP_DIV_F); break;
                }
            }

			*r=(Ret){tDst,false,true};
			if(exprMulPrim(r)) return true;
		}
		if (op->code == MUL) tkerr("expression missing after *");
        else tkerr("expression missing after /");
	}
	return true;
}

bool exprMul(Ret *r){
    Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	if(exprCast(r)) {
        if(exprMulPrim(r)) return true;
    }
    iTk=start;
    if(owner) delInstrAfter(startInstr);
	return false;
}

bool exprAddPrim(Ret *r){
	if(consume(ADD) || consume(SUB)){
        Token *op = consumedTk;
        Instr *lastLeft = owner ? lastInstr(owner->fn.instr) : NULL;
        addRVal(&owner->fn.instr, r->lval, &r->type);

		Ret right;
		if(exprMul(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) {
                if(op->code == ADD) tkerr("invalid operand type for +");
                else tkerr("invalid operand type for -");
            }

            addRVal(&owner->fn.instr, right.lval, &right.type);
            insertConvIfNeeded(lastLeft, &r->type, &tDst);
            insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);

            if(op->code == ADD) {
                switch(tDst.tb){
                    case TB_INT: addInstr(&owner->fn.instr, OP_ADD_I); break;
                    case TB_DOUBLE: addInstr(&owner->fn.instr, OP_ADD_F); break;
                }
            } else {
                switch(tDst.tb){
                    case TB_INT: addInstr(&owner->fn.instr, OP_SUB_I); break;
                    case TB_DOUBLE: addInstr(&owner->fn.instr, OP_SUB_F); break;
                }
            }

			*r=(Ret){tDst,false,true};
			if (exprAddPrim(r)) return true;
		}
		if(op->code == ADD) tkerr("expression missing after +");
        else tkerr("expression missing after -");
	}
	return true;
}

bool exprAdd(Ret *r){
    Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	if(exprMul(r)) {
        if(exprAddPrim(r)) return true;
    }
    iTk=start;
    if(owner) delInstrAfter(startInstr);
	return false;
}

bool exprRelPrim(Ret *r){
	if(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)){
        Token *op = consumedTk;
        Instr *lastLeft = owner ? lastInstr(owner->fn.instr) : NULL;
        addRVal(&owner->fn.instr, r->lval, &r->type);

		Ret right;
		if(exprAdd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for relational operator");
			
            addRVal(&owner->fn.instr, right.lval, &right.type);
            insertConvIfNeeded(lastLeft, &r->type, &tDst);
            insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);

            if(op->code == LESS) {
                switch(tDst.tb){
                    case TB_INT: addInstr(&owner->fn.instr, OP_LESS_I); break;
                    case TB_DOUBLE: addInstr(&owner->fn.instr, OP_LESS_F); break;
                }
            }

            *r=(Ret){{TB_INT,NULL,-1},false,true};
			if (exprRelPrim(r)) return true;
		}
		tkerr("expression missing after relational operator");
	}
	return true;
}

bool exprRel(Ret *r){
    Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	if(exprAdd(r)) {
        if(exprRelPrim(r)) return true;
    }
    iTk=start;
    if(owner) delInstrAfter(startInstr);
	return false;
}

bool exprEqPrim(Ret *r){
	if(consume(EQUAL) || consume(NOTEQ)){
        Token *op = consumedTk;
		Ret right;
		if(exprRel(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) {
                if(op->code == EQUAL) tkerr("invalid operand type for ==");
                else tkerr("invalid operand type for !=");
            }
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if (exprEqPrim(r)) return true;
		}
		if(op->code == EQUAL) tkerr("expression missing after ==");
        else tkerr("expression missing after !=");
	}
	return true;
}

bool exprEq(Ret *r){
    Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	if(exprRel(r)) {
        if(exprEqPrim(r)) return true;
    }
    iTk=start;
    if(owner) delInstrAfter(startInstr);
	return false;
}

bool exprAndPrim(Ret *r){
	if(consume(AND)){
		Ret right;
		if(exprEq(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for &&");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if (exprAndPrim(r)) return true;
		}
		tkerr("expression missing after &&");
	}
	return true;
}

bool exprAnd(Ret *r){
    Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	if(exprEq(r)) {
        if(exprAndPrim(r)) return true;
    }
    iTk=start;
    if(owner) delInstrAfter(startInstr);
	return false;
}

bool exprOrPrim(Ret *r){
	if(consume(OR)){
		Ret right;
		if(exprAnd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst)) tkerr("invalid operand type for ||");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if (exprOrPrim(r)) return true;
		}
		tkerr("expression missing after ||");
	}
	return true;
}

bool exprOr(Ret *r){
    Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	if(exprAnd(r)) {
        if(exprOrPrim(r)) return true;
    }
    iTk=start;
    if(owner) delInstrAfter(startInstr);
	return false;
}

bool exprAssign(Ret *r){
	Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
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

                addRVal(&owner->fn.instr, r->lval, &r->type);
                insertConvIfNeeded(lastInstr(owner->fn.instr), &r->type, &rDst.type);
                switch(rDst.type.tb){
                    case TB_INT: addInstr(&owner->fn.instr, OP_STORE_I); break;
                    case TB_DOUBLE: addInstr(&owner->fn.instr, OP_STORE_F); break;
                }
				return true;
			}
			tkerr("expression missing after =");
		}
	}
	iTk=start;
    if(owner) delInstrAfter(startInstr);

	if (exprOr(r)) return true;

    iTk=start;
    if(owner) delInstrAfter(startInstr);
    return false;
}

bool expr(Ret *r){
	return exprAssign(r);
}

// stmCompound: LACC ( varDef | stm )* RACC
bool stmCompound(bool newDomain){
	Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
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
    if(owner) delInstrAfter(startInstr);
	return false;
}

bool stm(){
	Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	Ret rCond,rExpr;

	if(stmCompound(true)) return true;

	if(consume(IF)){
		if(consume(LPAR)){
			if(expr(&rCond)){
				if(!canBeScalar(&rCond)) tkerr("the if condition must be a scalar value");
				
                addRVal(&owner->fn.instr, rCond.lval, &rCond.type);
                Type intType = {TB_INT, NULL, -1};
                insertConvIfNeeded(lastInstr(owner->fn.instr), &rCond.type, &intType);
                Instr *ifJF = addInstr(&owner->fn.instr, OP_JF);

                if(consume(RPAR)){
					if(stm()){
						if(consume(ELSE)){
                            Instr *ifJMP = addInstr(&owner->fn.instr, OP_JMP);
                            ifJF->arg.instr = addInstr(&owner->fn.instr, OP_NOP);

							if(!stm()) tkerr("statement missing after else");

                            ifJMP->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
						} else {
                            ifJF->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
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
    iTk=start;
    if(owner) delInstrAfter(startInstr);

	if(consume(WHILE)){
        Instr *beforeWhileCond = lastInstr(owner->fn.instr);
		if(consume(LPAR)){
			if(expr(&rCond)){
				if(!canBeScalar(&rCond)) tkerr("the while condition must be a scalar value");
				
                addRVal(&owner->fn.instr, rCond.lval, &rCond.type);
                Type intType = {TB_INT, NULL, -1};
                insertConvIfNeeded(lastInstr(owner->fn.instr), &rCond.type, &intType);
                Instr *whileJF = addInstr(&owner->fn.instr, OP_JF);

                if(consume(RPAR)){
					if(stm()){
                        addInstr(&owner->fn.instr, OP_JMP)->arg.instr = beforeWhileCond ? beforeWhileCond->next : owner->fn.instr;
                        whileJF->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
                        return true;
                    }
					tkerr("statement missing for while body");
				}
				tkerr(") missing after while condition");
			}
			tkerr("invalid or missing condition in while");
		}
		tkerr("( missing after while");
	}
    iTk=start;
    if(owner) delInstrAfter(startInstr);

	if(consume(RETURN)){
		if(expr(&rExpr)){
			if(owner->type.tb==TB_VOID) tkerr("a void function cannot return a value");
			if(!canBeScalar(&rExpr)) tkerr("the return value must be a scalar value");
			if(!convTo(&rExpr.type,&owner->type)) tkerr("cannot convert the return expression type to the function return type");
		    
            addRVal(&owner->fn.instr, rExpr.lval, &rExpr.type);
            insertConvIfNeeded(lastInstr(owner->fn.instr), &rExpr.type, &owner->type);
            addInstrWithInt(&owner->fn.instr, OP_RET, symbolsLen(owner->fn.params));
        } else {
			if(owner->type.tb!=TB_VOID) tkerr("a non-void function must return a value");
		    addInstrWithInt(&owner->fn.instr, OP_RET_VOID, symbolsLen(owner->fn.params));
        }
		if(consume(SEMICOLON)) return true;
		tkerr("; missing after return");
	}
    iTk=start;
    if(owner) delInstrAfter(startInstr);

	if(expr(&rExpr)) {
        if (rExpr.type.tb != TB_VOID) addInstr(&owner->fn.instr, OP_DROP);
        if(consume(SEMICOLON)) return true;
        tkerr("missing ;");
    }
	iTk=start;
    if(owner) delInstrAfter(startInstr);

    if (consume(SEMICOLON)) return true;

	return false;
}

// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
bool fnDef(){
	Token *start=iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	Type t;
	bool hasType=typeBase(&t);
	if(!hasType){
		if(consume(VOID)){
			t.tb=TB_VOID;
			t.n=-1;
			t.s=NULL;
		} else {
			iTk=start;
            if(owner) delInstrAfter(startInstr);
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

            addInstr(&fn->fn.instr, OP_ENTER);

			if(fnParam()){
				while(consume(COMMA)){
					if(!fnParam()) tkerr("parameter missing or invalid after ,");
				}
			}
			if(consume(RPAR)){
				if(stmCompound(false)) {
                    fn->fn.instr->arg.i = symbolsLen(fn->fn.locals);
                    if (fn->type.tb == TB_VOID) {
                        addInstrWithInt(&fn->fn.instr, OP_RET_VOID, symbolsLen(fn->fn.params));
                    }
					dropDomain();
					owner=NULL;
					return true;
				}
				tkerr("function body { } missing");
			}
			tkerr(") missing in function definition");
		}
		iTk=start;
        if(owner) delInstrAfter(startInstr);
		return false;
	}
	iTk=start;
    if(owner) delInstrAfter(startInstr);
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