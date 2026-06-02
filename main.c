#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "ad.h"
#include "vm.h"

int main(){
    // FILE *fout = fopen("testGenCod.txt","w");
    // if(!fout){
    //     printf("[ERR ] Unable to open file to write.\n");
    //     return 1;
    // }

    char *buffer = loadFile("tests/testgc.c");
    Token *tks = tokenize(buffer);
    // writeTokens(tks, fout);
    // fclose(fout);

    printf("\n[INFO] Tokenize process is done!\n");

    pushDomain();
    vmInit();
    parse(tks);
    printf("Parsing and domain analysis successful!\n");

    Symbol *symMain=findSymbolInDomain(symTable,"main");
    if(!symMain)err("missing main function");
    Instr *entryCode=NULL;
    addInstr(&entryCode,OP_CALL)->arg.instr=symMain->fn.instr;
    addInstr(&entryCode,OP_HALT);
    run(entryCode);
    
    dropDomain();
    free(buffer);
    return 0;
}