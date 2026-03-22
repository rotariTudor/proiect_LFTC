#pragma once
typedef enum {
    ID,

	INT, DOUBLE, CHAR, STRING,

    TYPE_CHAR,TYPE_DOUBLE,TYPE_INT,STRUCT,VOID,

	IF,ELSE,WHILE,RETURN,

    COMMA,SEMICOLON,LPAR,RPAR,LBRACKET,RBRACKET,LACC,RACC,

    ADD,SUB,MUL,DIV,DOT,

    AND,OR,NOT,

	ASSIGN,EQUAL,NOTEQ,

    LESS,LESSEQ,GREATER,GREATEREQ,

    END

} TokenCode;


typedef struct Token{
	int code;		// ID, TYPE_CHAR, ...
	int line;		// the line from the input file
	union{
		char *text;		// the chars for ID, STRING (dynamically allocated)
		int i;		// the value for INT
		char c;		// the value for CHAR
		double d;		// the value for DOUBLE
		};
	struct Token *next;		// next token in a simple linked list
	}Token;

Token *tokenize(const char *pch);
void showTokens(const Token *tokens);
void printTokens(const Token *tk);
void writeTokens(const Token *tk, FILE *outFile);
