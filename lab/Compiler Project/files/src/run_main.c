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
    Instr *testCode = genTestProgramSecondary();
    run(testCode);
    dropDomain();

    return 0;
}
