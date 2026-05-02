#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "utils.h"
#include "parser.h"

int main(){
    FILE *fout = fopen("testParser.txt","w");
    if(!fout){
        printf("Unable to open file to write.\n");
        return 1;
    }

    char *buffer = loadFile("tests/testparser.c");
    Token *tks = tokenize(buffer);
    writeTokens(tks, fout);
    fclose(fout);

    printf("Tokenize process is done!\n");

    parse(tks);
    printf("Parsing successful!\n");

    free(buffer);
    return 0;
}