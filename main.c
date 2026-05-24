#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"
#include "parser.h"
#include "vm.h"

int main(){
    // FILE *fout = fopen("testParser.txt","w");
    // if(!fout){
    //     printf("[ERR ] Unable to open file to write.\n");
    //     return 1;
    // }

    char *buffer = loadFile("tests/fct_tr.c");
    Token *tks = tokenize(buffer);
    // writeTokens(tks, fout);
    // fclose(fout);

    printf("\n[INFO] Tokenize process is done!\n");

    pushDomain();
    vmInit();
    parse(tks);
    // printf("Parsing and domain analysis successful!\n");

    printf("\n[INFO] Running genTestProgram (int)\n");
    Instr *testCode = genTestProgram();
    run(testCode);

    printf("\n[INFO] Running genTestProgram (int)\n");
    Instr *testCode2 = genTestProgram2();
    run(testCode2);

    printf("\n[INFO] Succes!\n");

    dropDomain();
    free(buffer);
    return 0;
}