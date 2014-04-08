#include "lib/mpc.h"

/* Compile these functions if we're on Windows */
#ifdef _WIN32

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

/* lval possible types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

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
    v->type  = LVAL_ERR;
    v->err   = malloc(strlen(m) + 1);
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

void lval_print(lval*);

/* Print an lval expression */
void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        /* Print the value */
        lval_print(v->cell[i]);
        /* Don't print trailing space if last element */
        if (i != (v->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

/* Print an lval */
void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_NUM:
            printf("%li", v->num);
            break;

        case LVAL_ERR:
            printf("Error: %s", v->err);
            break;

        case LVAL_SYM:
            printf("%s", v->sym);
            break;

        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;
    }
}

/* Print an lval and a newline */
void lval_println(lval* v) {
    lval_print(v);
    putchar('\n');
}

/* Read in a number */
lval* lval_read_num(mpc_ast_t* t) {
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("Invalid number");
}

/* Read in the AST */
lval* lval_read(mpc_ast_t* t) {
    /* If sym or num, convert to that type */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    /* If root (>) or sexpr then create an empty list */
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }

    /* Fill the list with valid expressions within */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->tag, "regex") == 0)  { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

/* Pops an element at index i from an s-expr, moving the later elements up */
lval* lval_pop(lval* v, int i) {
    /* Find the item at `i` */
    lval* x = v->cell[i];

    /* Shift the memory following the item at `i` over top it */
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

    /* Decrease count of items */
    v->count--;

    /* Reallocate memory used */
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

/* Pops an element at index i from an s-expr, then deletes the s-expr */
lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* lval_eval_sexpr(lval*);
lval* builtin_op(lval*, char*);

/* Evaluate an lval */
lval* lval_eval(lval* v) {
    /* Eval s-exprs */
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
    /* Other types stay the same, so just give them back */
    return v;
}

/* Evaluate an s-expr */
lval* lval_eval_sexpr(lval* v) {
    /* Evaluate children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    /* Error checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* Empty expressions */
    if (v->count == 0) { return v; }

    /* Single expression */
    if (v->count == 1) { return lval_take(v, 0); }

    /* Ensure first element is a symbol */
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f);
        lval_del(v);
        return lval_err("S-expression does not start with symbol");
    }

    /* Call builtin with operator */
    lval* result = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}

/* Performs arithmetic operations */
lval* builtin_op(lval* a, char* op) {
    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operator on non-number!");
        }
    }

    /* Pop the first element */
    lval* x = lval_pop(a, 0);

    /* If no arguments and sub then perform unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) { x->num = -x->num; }

    /* While there are still elements remaining */
    while (a->count > 0) {
        /* Pop next element */
        lval* y = lval_pop(a, 0);

        /* Perform operation */
        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x);
                lval_del(y);
                lval_del(a);
                x = lval_err("Division by zero");
                break;
            } else {
                x->num /= y->num;
            }
        }

        /* Delete the element now finished with */
        lval_del(y);
    }

    /* Delete input expression and return result */
    lval_del(a);
    return x;
}

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
            lval* x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
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