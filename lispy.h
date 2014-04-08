#ifndef _LISPY_H
#define _LISPY_H

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

/***************************
* Macros, Enums, and Structs
***************************/
#define LASSERT(args, cond, err) if (!(cond)) { lval_del(args); return lval_err(err); }

/* lval possible types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

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

/**********************
* Function declarations
**********************/

lval* lval_num(long);
lval* lval_err(char*);
lval* lval_sym(char*);
lval* lval_sexpr(void);
lval* lval_qepxr(void);

void  lval_del(lval*);
lval* lval_add(lval*, lval*);

void lval_expr_print(lval*, char, char);
void lval_print(lval*);
void lval_println(lval*);

lval* lval_read_num(mpc_ast_t*);
lval* lval_read(mpc_ast_t*);

lval* lval_pop(lval*, int);
lval* lval_take(lval*, int);
lval* lval_eval(lval*);
lval* lval_eval_sexpr(lval*);

lval* builtin(lval*, char*);
lval* builtin_op(lval*, char*);
lval* builtin_head(lval*);
lval* builtin_tail(lval*);
lval* builtin_list(lval*);
lval* builtin_eval(lval*);
lval* builtin_join(lval*);
lval* lval_join(lval*, lval*);

#endif