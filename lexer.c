#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

Token *tokens;
Token *lastTk;

int line=1;

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

char *extract(const char *begin, const char *end){
	int len = end - begin;
	char *str = (char*)malloc(len+1);
	if(!str) err("not enough memory");
	memcpy(str, begin, len);
	str[len]='\0';
	return str;
}

Token *tokenize(const char *pch){
	const char *start;
	Token *tk;
	for(;;){
		switch(*pch){
			case ' ':case '\t':pch++;break;
			case '\r':
				if(pch[1]=='\n')pch++;
			case '\n':
				line++;
				pch++;
				break;
			case '\0':addTk(END);return tokens;
			case ',':addTk(COMMA);pch++;break;
			case ';':addTk(SEMICOLON);pch++;break;
			case '(':addTk(LPAR);pch++;break;
			case ')':addTk(RPAR);pch++;break;
			case '[':addTk(LBRACKET);pch++;break;
			case ']':addTk(RBRACKET);pch++;break;
			case '{':addTk(LACC);pch++;break;
			case '}':addTk(RACC);pch++;break;
			case '+':addTk(ADD);pch++;break;
			case '-':addTk(SUB);pch++;break;
			case '*':addTk(MUL);pch++;break;
			case '.':addTk(DOT);pch++;break;
			case '/':
				if(pch[1]=='/'){
					while(*pch&&*pch!='\n'&&*pch!='\r')pch++;
				}else{
					addTk(DIV);pch++;
				}
				break;
			case '=':
				if(pch[1]=='='){addTk(EQUAL);pch+=2;}
				else{addTk(ASSIGN);pch++;}
				break;
			case '!':
				if(pch[1]=='='){addTk(NOTEQ);pch+=2;}
				else{addTk(NOT);pch++;}
				break;
			case '<':
				if(pch[1]=='='){addTk(LESSEQ);pch+=2;}
				else{addTk(LESS);pch++;}
				break;
			case '>':
				if(pch[1]=='='){addTk(GREATEREQ);pch+=2;}
				else{addTk(GREATER);pch++;}
				break;
			case '&':
				if(pch[1]=='&'){addTk(AND);pch+=2;}
				else err("bitwise op '&' not supported");
				break;
			case '|':
				if(pch[1]=='|'){addTk(OR);pch+=2;}
				else err("bitwise op '|' not supported");
				break;
			case '\'':
				pch++;
				if(*pch=='\\'){
					pch++;
					tk=addTk(CHAR);
					switch(*pch){
						case 'a':tk->c='\a';break;
						case 'b':tk->c='\b';break;
						case 'f':tk->c='\f';break;
						case 'n':tk->c='\n';break;
						case 'r':tk->c='\r';break;
						case 't':tk->c='\t';break;
						case 'v':tk->c='\v';break;
						case '\\':tk->c='\\';break;
						case '\'':tk->c='\'';break;
						case '"':tk->c='"';break;
						case '0':tk->c='\0';break;
						default:err("invalid escape sequence in char");
					}
				}else{
					tk=addTk(CHAR);
					tk->c=*pch;
				}
				pch++;
				if(*pch!='\'')err("missing closing '");
				pch++;
				break;
			case '"':{
				pch++;
				int len=0;
				const char *tmp=pch;
				while(*tmp&&*tmp!='"'){
					if(*tmp=='\\')tmp++;
					tmp++;
					len++;
				}
				if(*tmp!='"')err("missing closing \"");
				char *str=(char*)malloc(len+1);
				if(!str)err("not enough memory");
				int idx=0;
				while(*pch&&*pch!='"'){
					if(*pch=='\\'){
						pch++;
						switch(*pch){
							case 'a':str[idx++]='\a';break;
							case 'b':str[idx++]='\b';break;
							case 'f':str[idx++]='\f';break;
							case 'n':str[idx++]='\n';break;
							case 'r':str[idx++]='\r';break;
							case 't':str[idx++]='\t';break;
							case 'v':str[idx++]='\v';break;
							case '\\':str[idx++]='\\';break;
							case '\'':str[idx++]='\'';break;
							case '"':str[idx++]='"';break;
							case '0':str[idx++]='\0';break;
							default:err("invalid escape sequence in string");
						}
					}else{
						str[idx++]=*pch;
					}
					pch++;
				}
				str[idx]='\0';
				tk=addTk(STRING);
				tk->text=str;
				pch++;
				break;
			}
			default:
				if(isalpha(*pch)||*pch=='_'){
					for(start=pch++;isalnum(*pch)||*pch=='_';pch++){}
					char *text=extract(start,pch);
					if(strcmp(text,"int")==0)addTk(TYPE_INT);
					else if(strcmp(text,"double")==0)addTk(TYPE_DOUBLE);
					else if(strcmp(text,"char")==0)addTk(TYPE_CHAR);
					else if(strcmp(text,"struct")==0)addTk(STRUCT);
					else if(strcmp(text,"void")==0)addTk(VOID);
					else if(strcmp(text,"if")==0)addTk(IF);
					else if(strcmp(text,"else")==0)addTk(ELSE);
					else if(strcmp(text,"while")==0)addTk(WHILE);
					else if(strcmp(text,"return")==0)addTk(RETURN);
					else{tk=addTk(ID);tk->text=text;}
				}else if(isdigit(*pch)){
					start=pch;
					while(isdigit(*pch))pch++;
					if(*pch=='.'||*pch=='e'||*pch=='E'){
						if(*pch=='.'){
							pch++;
							int aux = *pch;
							while(isdigit(*pch))pch++;
							if(aux==*pch){
								err("missing at least 1 number after point at line -> %d.\n",line);
							}
						}
						if(*pch=='e'||*pch=='E'){
							pch++;
							if(*pch=='+'||*pch=='-')pch++;
							if(!isdigit(*pch)){
								err("number missing after sign of exponent at line -> %d.\n",line);
							}
							while(isdigit(*pch))pch++;
						}
						tk=addTk(DOUBLE);
						tk->d=atof(start);
					}else{
						tk=addTk(INT);
						tk->i=atoi(start);
					}
				}else err("invalid char: %c (%d)",*pch,*pch);
		}
	}
}

void showTokens(const Token *tokens){
	for(const Token *tk=tokens;tk;tk=tk->next){
		printf("%d\n",tk->code);
	}
}

void printTokens(const Token *tokens){
	for(const Token *tk=tokens;tk;tk=tk->next){
		switch(tk->code){
			case ID:         printf("%d\tID:%s\n",        tk->line,tk->text);break;
			case TYPE_CHAR:  printf("%d\tTYPE_CHAR\n",    tk->line);break;
			case TYPE_DOUBLE:printf("%d\tTYPE_DOUBLE\n",  tk->line);break;
			case TYPE_INT:   printf("%d\tTYPE_INT\n",     tk->line);break;
			case STRUCT:     printf("%d\tSTRUCT\n",       tk->line);break;
			case VOID:       printf("%d\tVOID\n",         tk->line);break;
			case IF:         printf("%d\tIF\n",           tk->line);break;
			case ELSE:       printf("%d\tELSE\n",         tk->line);break;
			case WHILE:      printf("%d\tWHILE\n",        tk->line);break;
			case RETURN:     printf("%d\tRETURN\n",       tk->line);break;
			case COMMA:      printf("%d\tCOMMA\n",        tk->line);break;
			case SEMICOLON:  printf("%d\tSEMICOLON\n",    tk->line);break;
			case LPAR:       printf("%d\tLPAR\n",         tk->line);break;
			case RPAR:       printf("%d\tRPAR\n",         tk->line);break;
			case LBRACKET:   printf("%d\tLBRACKET\n",     tk->line);break;
			case RBRACKET:   printf("%d\tRBRACKET\n",     tk->line);break;
			case LACC:       printf("%d\tLACC\n",         tk->line);break;
			case RACC:       printf("%d\tRACC\n",         tk->line);break;
			case ADD:        printf("%d\tADD\n",          tk->line);break;
			case SUB:        printf("%d\tSUB\n",          tk->line);break;
			case MUL:        printf("%d\tMUL\n",          tk->line);break;
			case DIV:        printf("%d\tDIV\n",          tk->line);break;
			case DOT:        printf("%d\tDOT\n",          tk->line);break;
			case AND:        printf("%d\tAND\n",          tk->line);break;
			case OR:         printf("%d\tOR\n",           tk->line);break;
			case NOT:        printf("%d\tNOT\n",          tk->line);break;
			case ASSIGN:     printf("%d\tASSIGN\n",       tk->line);break;
			case EQUAL:      printf("%d\tEQUAL\n",        tk->line);break;
			case NOTEQ:      printf("%d\tNOTEQ\n",        tk->line);break;
			case LESS:       printf("%d\tLESS\n",         tk->line);break;
			case LESSEQ:     printf("%d\tLESSEQ\n",       tk->line);break;
			case GREATER:    printf("%d\tGREATER\n",      tk->line);break;
			case GREATEREQ:  printf("%d\tGREATEREQ\n",    tk->line);break;
			case INT:        printf("%d\tINT:%d\n",       tk->line,tk->i);break;
			case DOUBLE:     printf("%d\tDOUBLE:%.2f\n",    tk->line,tk->d);break;
			case CHAR:       printf("%d\tCHAR:%c\n",      tk->line,tk->c);break;
			case STRING:     printf("%d\tSTRING:%s\n",    tk->line,tk->text);break;
			case END:        printf("%d\tEND\n",          tk->line);break;
			default:         printf("%d\tUNKNOWN\n",      tk->line);break;
		}
	}
}

void writeTokens(const Token *tokens, FILE *fout){
	for(const Token *tk=tokens;tk;tk=tk->next){
		switch(tk->code){
			case ID:         fprintf(fout,"%d\tID:%s\n",        tk->line,tk->text);break;
			case TYPE_CHAR:  fprintf(fout,"%d\tTYPE_CHAR\n",    tk->line);break;
			case TYPE_DOUBLE:fprintf(fout,"%d\tTYPE_DOUBLE\n",  tk->line);break;
			case TYPE_INT:   fprintf(fout,"%d\tTYPE_INT\n",     tk->line);break;
			case STRUCT:     fprintf(fout,"%d\tSTRUCT\n",       tk->line);break;
			case VOID:       fprintf(fout,"%d\tVOID\n",         tk->line);break;
			case IF:         fprintf(fout,"%d\tIF\n",           tk->line);break;
			case ELSE:       fprintf(fout,"%d\tELSE\n",         tk->line);break;
			case WHILE:      fprintf(fout,"%d\tWHILE\n",        tk->line);break;
			case RETURN:     fprintf(fout,"%d\tRETURN\n",       tk->line);break;
			case COMMA:      fprintf(fout,"%d\tCOMMA\n",        tk->line);break;
			case SEMICOLON:  fprintf(fout,"%d\tSEMICOLON\n",    tk->line);break;
			case LPAR:       fprintf(fout,"%d\tLPAR\n",         tk->line);break;
			case RPAR:       fprintf(fout,"%d\tRPAR\n",         tk->line);break;
			case LBRACKET:   fprintf(fout,"%d\tLBRACKET\n",     tk->line);break;
			case RBRACKET:   fprintf(fout,"%d\tRBRACKET\n",     tk->line);break;
			case LACC:       fprintf(fout,"%d\tLACC\n",         tk->line);break;
			case RACC:       fprintf(fout,"%d\tRACC\n",         tk->line);break;
			case ADD:        fprintf(fout,"%d\tADD\n",          tk->line);break;
			case SUB:        fprintf(fout,"%d\tSUB\n",          tk->line);break;
			case MUL:        fprintf(fout,"%d\tMUL\n",          tk->line);break;
			case DIV:        fprintf(fout,"%d\tDIV\n",          tk->line);break;
			case DOT:        fprintf(fout,"%d\tDOT\n",          tk->line);break;
			case AND:        fprintf(fout,"%d\tAND\n",          tk->line);break;
			case OR:         fprintf(fout,"%d\tOR\n",           tk->line);break;
			case NOT:        fprintf(fout,"%d\tNOT\n",          tk->line);break;
			case ASSIGN:     fprintf(fout,"%d\tASSIGN\n",       tk->line);break;
			case EQUAL:      fprintf(fout,"%d\tEQUAL\n",        tk->line);break;
			case NOTEQ:      fprintf(fout,"%d\tNOTEQ\n",        tk->line);break;
			case LESS:       fprintf(fout,"%d\tLESS\n",         tk->line);break;
			case LESSEQ:     fprintf(fout,"%d\tLESSEQ\n",       tk->line);break;
			case GREATER:    fprintf(fout,"%d\tGREATER\n",      tk->line);break;
			case GREATEREQ:  fprintf(fout,"%d\tGREATEREQ\n",    tk->line);break;
			case INT:        fprintf(fout,"%d\tINT:%d\n",       tk->line,tk->i);break;
			case DOUBLE:     fprintf(fout,"%d\tDOUBLE:%.2f\n",    tk->line,tk->d);break;
			case CHAR:       fprintf(fout,"%d\tCHAR:%c\n",      tk->line,tk->c);break;
			case STRING:     fprintf(fout,"%d\tSTRING:%s\n",    tk->line,tk->text);break;
			case END:        fprintf(fout,"%d\tEND\n",          tk->line);break;
			default:         fprintf(fout,"%d\tUNKNOWN\n",      tk->line);break;
		}
	}
}