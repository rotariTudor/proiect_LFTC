#include <stdio.h>
#include "lexer.h"
#include "utils.h"
#include <stdlib.h>

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

    free(buffer);
    fclose(fout);
    
    return 0;
}