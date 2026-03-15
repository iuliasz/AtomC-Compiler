#include <stdio.h>
#include <stdlib.h>
#include "../lexer.h"
#include "../utils.h"

int main(){
	char *buf = loadFile("tests/input.c");

	Token *tokens = tokenize(buf);

	showTokens(tokens);

	return 0;
}