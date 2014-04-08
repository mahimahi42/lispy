all: lispy

lispy: lispy.c lib/mpc.c
	gcc -std=c99 -ledit -lm -o lispy lispy.c lib/mpc.c
