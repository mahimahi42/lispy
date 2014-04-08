all: evaluation

evaluation: evaluation.c lib/mpc.c
	gcc -std=c99 -ledit -lm -o evaluation evaluation.c lib/mpc.c
