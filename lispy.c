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

/* Lisp Value struct */
typedef struct lval {
    int type;
    long num;
    /* String data for err and sym types */
    char* err;
    char* sym;
    /* Count and pointer to a list of lvals */
    int count;
    struct lval** cell;
} lval;

/* lval possible types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

/* lval possible error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval eval(mpc_ast_t*);
lval eval_op(lval*, char*, lval*);

lval* lval_num(long);
lval* lval_err(char*);
lval* lval_sym(char*);
lval* lval_sexpr(void);

void lval_print(lval*);
void lval_println(lval*);
void lval_del(lval*);
lval* lval_add(lval*, lval*);
lval* lval_read_num(mpc_ast_t*);
lval* lval_read(mpc_ast_t*);

int main(int argc, char** argv) {
	/* Create parsers */
	mpc_parser_t* Number = mpc_new("number");
        mpc_parser_t* Symbol = mpc_new("symbol");
        mpc_parser_t* Sexpr  = mpc_new("sexpr");
	mpc_parser_t* Expr   = mpc_new("expr");
	mpc_parser_t* Lispy  = mpc_new("lispy");

	/* Define parsers */
	mpca_lang(MPC_LANG_DEFAULT,
		"                                                     \
		number   : /-?[0-9]+/ ;                             \
		symbol   : '+' | '-' | '*' | '/' ;                  \
                sexpr    : '(' <expr>* ')' ;                        \
		expr     : <number> | <symbol> | <sexpr> ;          \
		lispy    : /^/ <expr>* /$/ ;             \
		",
		Number, Symbol, Sexpr, Expr, Lispy);


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
                lval result = eval(r.output);
                lval_println(result);
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
	mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

	return 0;
}

lval eval(mpc_ast_t* t) {
    /* If tagged as a number, return it, otherwise it's an expr */
    if (strstr(t->tag, "number")) {
        /* Check for conversion errors */
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    /* Operator is always second child */
    char* op = t->children[1]->contents;

    /* Store the third child in `x` */
    lval x = eval(t->children[2]);

    /* Iterate the remaining children, combining using our operator */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }
    return x;
}

lval eval_op(lval x, char* op, lval y) {
    /* Return if error */
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    /* Otherwise, do the maths */
    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if (strcmp(op, "/") == 0) { 
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }
    return lval_err(LERR_BAD_OP);
}

/* Create a new lval number */
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num  = x;
    return v;
}

/* Create a new lval error */
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v.type  = LVAL_ERR;
    v.err   = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

/* Create a new lval symbol */
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym  = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

/* Create a new lval sexpr */
lval* lval_sexpr(void) {
    lval* v  = malloc(sizeof(lval));
    v->type  = LVAL_SEXPR;
    v->count = 0;
    v->cell  = NULL;
    return v;
}

/* Print an lval */
void lval_print(lval v) {
    switch (v.type) {
        /* If a number, print it */
        case LVAL_NUM:
            printf("%li\n", v.num);
            break;

        /* If there's an error... */
        case LVAL_ERR:
            /* Print the error based on type */
            if (v.err == LERR_DIV_ZERO) { printf("Error: Division by zero!\n"); }
            if (v.err == LERR_BAD_OP)   { printf("Error: Invalid operator!\n"); }
            if (v.err == LERR_BAD_NUM)  { printf("Error: Invalid number!\n"); }
            break;
    }
}

/* Print an lval and a newline */
void lval_println(lval v) {
    lval_print(v);
    putchar("\n");
}

/* Delete an lval */
void lval_del(lval* v) {
    switch (v->type) {
        /* Nothing to do for numbers */
        case LVAL_NUM:
            break;

        /* Free the string data for errs and syms */
        case LVAL_ERR:
            free(v->err);
            break;
        case LVAL_SYM:
            free(v->sym);
            break;

        /* Delete all exprs in an sexpr */
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            /* Free the list */
            free(v->cell);
            break;
    }
    /* Free the memory for v */
    free(v);
}

/* Add an element to an sexpr */
lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}


