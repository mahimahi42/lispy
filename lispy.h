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

/* If we're on a Mac, include the correct library file */
#elif __APPLE__

#include <editline/readline.h>

/* Otherwise, we're on Linux! */
#else

#include <editline/readline.h>
#include <editline/history.h>

#endif

/***************************
* Macros, Enums, and Structs
***************************/
#define LASSERT(args, cond, fmt, ...) \
	if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
	LASSERT(args, args->cell[index]->type == expect, \
		"Function '%s' passed incorrect type for argument %i. Got %s, expected %s.", \
		func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);

/* Forward declarations for the compiler */
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Lispy;

/* lval possible types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_STR, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN };

/* Builtin function type */
typedef lval*(*lbuiltin)(lenv*, lval*);

/* Lisp Value struct */
struct lval {
    int type;

    /* Basic */
    long num;
    char* err;
    char* sym;
    char* str;

    /* Function */
    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;

    /* Expression */
    int count;
    lval** cell;
};

/* Lisp Environment struct */
struct lenv {
	lenv* par;
	int count;
	char** syms;
	lval** vals;
};

/**********************
* Function declarations
**********************/

char* ltype_name(int);

lval* lval_num(long);
lval* lval_err(char*, ...);
lval* lval_sym(char*);
lval* lval_sexpr(void);
lval* lval_qepxr(void);
lval* lval_fun(lbuiltin);
lval* lval_str(char*);

lenv* lenv_new(void);
void  lenv_del(lenv*);
lenv* lenv_copy(lenv*);

void  lval_del(lval*);
lval* lval_add(lval*, lval*);
lval* lval_copy(lval*);

lval* lenv_get(lenv*, lval*);
void  lenv_def(lenv*, lval*, lval*);
void  lenv_put(lenv*, lval*, lval*);
void  lenv_add_builtin(lenv*, char*, lbuiltin);
void  lenv_add_builtins(lenv*);

void lval_expr_print(lval*, char, char);
void lval_print_str(lval*);
void lval_print(lval*);
void lval_println(lval*);

lval* lval_read_num(mpc_ast_t*);
lval* lval_read_str(mpc_ast_t*);
lval* lval_read(mpc_ast_t*);

lval* lval_pop(lval*, int);
lval* lval_take(lval*, int);
lval* lval_eval(lenv*, lval*);
lval* lval_eval_sexpr(lenv*, lval*);
lval* lval_call(lenv*, lval*, lval*);
int   lval_eq(lval*, lval*);

lval* builtin_op(lenv*, lval*, char*);
lval* builtin_add(lenv*, lval*);
lval* builtin_sub(lenv*, lval*);
lval* builtin_mul(lenv*, lval*);
lval* builtin_div(lenv*, lval*);

lval* builtin_head(lenv*, lval*);
lval* builtin_tail(lenv*, lval*);
lval* builtin_list(lenv*, lval*);
lval* builtin_eval(lenv*, lval*);
lval* builtin_join(lenv*, lval*);
lval* lval_join(lval*, lval*);
lval* builtin_var(lenv*, lval*, char*);
lval* builtin_def(lenv*, lval*);
lval* builtin_put(lenv*, lval*);
lval* builtin_lambda(lenv*, lval*);

lval* builtin_ord(lenv*, lval*, char*);
lval* builtin_gt(lenv*, lval*);
lval* builtin_lt(lenv*, lval*);
lval* builtin_ge(lenv*, lval*);
lval* builtin_le(lenv*, lval*);

lval* builtin_cmp(lenv*, lval*, char*);
lval* builtin_eq(lenv*, lval*);
lval* builtin_ne(lenv*, lval*);
lval* builtin_if(lenv*, lval*);

lval* builtin_load(lenv*, lval*);
lval* builtin_print(lenv*, lval*);
lval* builtin_error(lenv*, lval*);

#endif