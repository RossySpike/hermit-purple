.PHONY:
make:
	gcc -march=znver3 -O2 main.c -o ./test/main.out
debug:
	gcc -O0 fsanitize=address,undefined,leak -g main.c -o ./test/main.debug.out
test:
	gcc -Wno-unused-parameter -Wpedantic -Wall -Wextra -fhardened -fstack-protector-strong -fcf-protection -Os -std=c23 main.c -o ./test/main.test.out
