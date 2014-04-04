all: parsing

parsing: parsing.c lib/mpc.c
	gcc -std=c99 -ledit -lm -o parsing parsing.c lib/mpc.c