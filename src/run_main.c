#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "utils.h"
#include "ad.h"
#include "vm.h"

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "Needs input file");
        return 1;
    }

    char *buffer = loadFile(argv[1]);
    Token *tokens = tokenize(buffer);

    showTokens(tokens);

    pushDomain();
    vmInit();
    parse(tokens);
    printf("\n\n");
    // showDomain(symTable, "global");
    // Instr *testCode = genTestProgram();
    // Instr *testCode = genTestProgramSecondary();
    // run(testCode);

    Symbol *symMain = findSymbolInDomain(symTable, "main");
    if (!symMain) err("missing main function");
    Instr *entryCode = NULL;
    addInstr(&entryCode, OP_CALL)->arg.instr = symMain->fn.instr;
    addInstr(&entryCode, OP_HALT);
    run(entryCode);

    dropDomain();

    return 0;
}
