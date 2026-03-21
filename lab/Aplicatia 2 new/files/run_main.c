#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "utils.h"

int main(){
	char *buffer=loadFile("tests/testlex.c");

    Token *tokens=tokenize(buffer);

    showTokens(tokens);

	return 0;
}