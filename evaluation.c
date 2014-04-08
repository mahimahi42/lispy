#include <stdio.h>
#include <stdlib.h>

#include "lib/mpc.h"

/* Compile these functions if we're on Windows */
#ifdef _WIN32

#include <string.h>

static char buffer[2048];

char* readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer) + 1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy) - 1] = "\0";
	return cpy;
}

void add_history(char* unused) {}

#elif __APPLE__

#include <editline/readline.h>

#else

#include <editline/readline.h>
#include <editline/history.h>

#endif

long eval(mpc_ast_t*);
long eval_op(long, char*, long);

int main(int argc, char** argv) {
	/* Create parsers */
	mpc_parser_t* Number   = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr     = mpc_new("expr");
	mpc_parser_t* Lispy    = mpc_new("lispy");

	/* Define parsers */
	mpca_lang(MPC_LANG_DEFAULT,
		"                                                     \
		number   : /-?[0-9]+/ ;                             \
		operator : '+' | '-' | '*' | '/' ;                  \
		expr     : <number> | '(' <operator> <expr>+ ')' ;  \
		lispy    : /^/ <operator> <expr>+ /$/ ;             \
		",
		Number, Operator, Expr, Lispy);


	/* Print Version and Exit Information */
	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl+c to Exit\n");

	/* In a never ending loop */
	while (1) {
		/* Output our prompt and get input */
	    char* input = readline("lispy> ");
	    
	    /* Add input to history */
	    add_history(input);
	    
	    /* Attempt to parse input */
	    mpc_result_t r;
	    if (mpc_parse("<stdin>", input, Lispy, &r)) {
	    	/* Print AST on success */
                long result = eval(r.output);
                printf("%li\n", result);
	    	mpc_ast_delete(r.output);
	    } else {
	    	/* Otherwise, print the error */
	    	mpc_err_print(r.error);
	    	mpc_err_delete(r.error);
	    }

	    /* Free retrived input */
	    free(input);
	}

	/* Undefine and delete parsers */
	mpc_cleanup(4, Number, Operator, Expr, Lispy);

	return 0;
}

long eval(mpc_ast_t* t) {
    /* If tagged as a number, return it, otherwise it's an expr */
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    /* Operator is always second child */
    char* op = t->children[1]->contents;

    /* Store the third child in `x` */
    long x = eval(t->children[2]);

    /* Iterate the remaining children, combining using our operator */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }
    return x;
}

long eval_op(long x, char* op, long y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    return 0;
}
