#!/usr/bin/env bash
set -e

gcc src/run_main.c src/lexer.c src/parser.c src/utils.c -o run_main
./run_main "$1" > outputs/output_test.txt