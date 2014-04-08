#include "lispy.h"

/* Get the type of a value */
char* ltype_name(int t) {
    switch(t) {
        case LVAL_FUN: return "Function";
        case LVAL_NUM: return "Number";
        case LVAL_ERR: return "Error";
        case LVAL_SYM: return "Symbol";
        case LVAL_SEXPR: return "S-Expression";
        case LVAL_QEXPR: return "Q-Expression";
        default: return "Unknown type";
    }
}

/* Create a new lval number */
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num  = x;
    return v;
}

/* Create a new lval error */
lval* lval_err(char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type  = LVAL_ERR;

    /* Create a va list and init it */
    va_list va;
    va_start(va, fmt);

    /* Allocate 512 bytes of space */
    v->err = malloc(512);

    /* Printf into the error string with a max of 511 chars */
    vsnprintf(v->err, 511, fmt, va);

    /* Reallocate to actual number of bytes used */
    v->err = realloc(v->err, strlen(v->err)+1);

    /* Cleanup our va list */
    va_end(va);

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

/* Create a new lval qexpr */
lval* lval_qexpr(void) {
    lval* v  = malloc(sizeof(lval));
    v->type  = LVAL_QEXPR;
    v->count = 0;
    v->cell  = NULL;
    return v;
}

/* Create a new lval function */
lval* lval_fun(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->fun  = func;
    return v;
}

/* Create a new lenv */
lenv* lenv_new(void) {
    lenv* e  = malloc(sizeof(lenv));
    e->count = 0;
    e->syms  = NULL;
    e->vals  = NULL;
    return e;
}

/* Delete an lenv */
void lenv_del(lenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }

    free(e->syms);
    free(e->vals);
    free(e);
}

/* Delete an lval */
void lval_del(lval* v) {
    switch (v->type) {
        /* Nothing to do for numbers or functions */
        case LVAL_NUM:
        case LVAL_FUN:
            break;

        /* Free the string data for errs and syms */
        case LVAL_ERR:
            free(v->err);
            break;
        case LVAL_SYM:
            free(v->sym);
            break;

        /* Delete all exprs in an sexpr/qexpr */
        case LVAL_QEXPR:
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

/* Copys an lval into a new lval */
lval* lval_copy(lval* v) {
    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
        /* Copy functions and numbers directly */
        case LVAL_FUN:
            x->fun = v->fun;
            break;
        case LVAL_NUM:
            x->num = v->num;
            break;

        /* Copy strings with malloc and strcpy */
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;
        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->err, v->err);
            break;

        /* Copy expressions by copying each sub-expression */
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell  = malloc(sizeof(lval*) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }
    return x;
}

/* Gets an lval from an lenv, or an error if it isn't there */
lval* lenv_get(lenv* e, lval* k) {
    /* Iterate over each item in the lenv */
    for (int i = 0; i < e->count; i++) {
        /* If the stored string matches the symbol string
        *  then return a copy of the value */
        if (strcmp(e->syms[i], k->sym) == 0) { return lval_copy(e->vals[i]); }
    }
    /* Otherwise, return an error */
    return lval_err("Unbound symbol '%s'", k->sym);
}

/* Puts an lval into the lenv, replacing the value if it already exists */
void lenv_put(lenv* e, lval* k, lval* v) {
    /* See if the variable already exists */
    for (int i = 0; i < e->count; i++) {
        /* If variable is found, delete it and replace
        *  with the variable supplied by the user */
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            e->syms[i] = realloc(e->syms[i], strlen(k->sym) + 1);
            strcpy(e->syms[i], k->sym);
            return;
        }
    }

    /* If it's not found, allocate space for it */
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    /* Copy the provided lval and symbol string */
    e->vals[e->count-1] = lval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);
}

/* Add a builtin function to an lenv */
void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

/* Add our builtin functions */
void lenv_add_builtins(lenv* e) {
    /* List functions */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);

    /* Math functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);

    /* Variable functions */
    lenv_add_builtin(e, "def", builtin_def);
}

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

        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}');
            break;

        case LVAL_FUN:
            printf("<function>");
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

    /* If root (>), sexpr, or qexpr then create an empty list */
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
    if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

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

/* Evaluate an lval */
lval* lval_eval(lenv* e, lval* v) {
    /* Get symbols from the environment */
    if (v->type == LVAL_SYM)   { return lenv_get(e, v); }
    /* Eval s-exprs */
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
    /* Other types stay the same, so just give them back */
    return v;
}

/* Evaluate an s-expr */
lval* lval_eval_sexpr(lenv* e, lval* v) {
    /* Evaluate children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    /* Error checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* Empty expressions */
    if (v->count == 0) { return v; }

    /* Single expression */
    if (v->count == 1) { return lval_take(v, 0); }

    /* Ensure first element is a function */
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval* err = lval_err(
            "S-Expression starts with incorrect type. Got %s, expected %s",
            ltype_name(f->type), ltype_name(LVAL_FUN)
            );
        lval_del(f);
        lval_del(v);
        return err;
    }

    /* Call function */
    lval* result = f->fun(e, v);
    lval_del(f);
    return result;
}

/* Performs arithmetic operations */
lval* builtin_op(lenv* e, lval* a, char* op) {
    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE(op, a, i, LVAL_NUM);
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

lval* builtin_add(lenv* e, lval* a) { return builtin_op(e, a, "+"); }
lval* builtin_sub(lenv* e, lval* a) { return builtin_op(e, a, "-"); }
lval* builtin_mul(lenv* e, lval* a) { return builtin_op(e, a, "*"); }
lval* builtin_div(lenv* e, lval* a) { return builtin_op(e, a, "/"); }

/* Builtin function for `head` */
lval* builtin_head(lenv* e, lval* a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

    lval* v = lval_take(a, 0);
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;
}

/* Builtin function for `tail` */
lval* builtin_tail(lenv* e, lval* a) {
    LASSERT_NUM("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", a, 0);

    lval* v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

/* Builtin function for `list` */
lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}

/* Builtin function for `eval` */
lval* builtin_eval(lenv* e, lval* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

/* Builtin function for `join` */
lval* builtin_join(lenv* e, lval* a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }

    lval* x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;
}

/* Join two lvals together */
lval* lval_join(lval* x, lval* y) {
    /* For each cell in `y` add it to `x` */
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    /* Delete the empty `y` and return `x` */
    lval_del(y);
    return x;
}

/* Builtin function for `def` */
lval* builtin_def(lenv* e, lval* a) {
    LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

    /* First argument is symbol list */
    lval* syms = a->cell[0];

    /* Ensure our list of symbols is all symbols */
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM), 
            "Function 'def' cannot define non-symbol, Got %s, expected %s",
            ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }

    /* Assign copies of values to symbols */
    for (int i = 0; i < syms->count; i++) {
        lenv_put(e, syms->cell[i], a->cell[i+1]);
    }

    lval_del(a);
    return lval_sexpr();
}

int main(int argc, char** argv) {
	/* Create parsers */
	mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr  = mpc_new("sexpr");
    mpc_parser_t* Qexpr  = mpc_new("qexpr");
	mpc_parser_t* Expr   = mpc_new("expr");
	mpc_parser_t* Lispy  = mpc_new("lispy");

	/* Define parsers */
	mpca_lang(MPC_LANG_DEFAULT,
		"                                                     \
          number : /-?[0-9]+/ ;                               \
          symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;         \
          sexpr  : '(' <expr>* ')' ;                          \
          qexpr  : '{' <expr>* '}' ;                          \
          expr   : <number> | <symbol> | <sexpr> | <qexpr> ;  \
          lispy  : /^/ <expr>* /$/ ;                          \
        ",
		Number, Symbol, Sexpr, Qexpr, Expr, Lispy);


	/* Print Version and Exit Information */
	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl+c to Exit\n");

    /* Create a new environment */
    lenv* e = lenv_new();
    lenv_add_builtins(e);

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
            lval* x = lval_eval(e, lval_read(r.output));
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

    lenv_del(e);

	/* Undefine and delete parsers */
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

	return 0;
}