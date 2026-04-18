#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "utils.h"
#include "parser.h"

int main(){
    FILE *fout = fopen("textLexTokens.txt","w");
    if(!fout){
        printf("Unable to open file to write.\n");
        return 1;
    }

    char *buffer = loadFile("tests/testlex.c");
    Token *tks = tokenize(buffer);
    //printTokens(tks);
    writeTokens(tks, fout);

    //parse(tks);
    printf("Tokenize process is done!\n");

    free(buffer);
    fclose(fout);
    
    return 0;
}