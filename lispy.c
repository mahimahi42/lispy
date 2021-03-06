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
        case LVAL_STR: return "String";
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
    v->builtin  = func;
    return v;
}

/* Create a new lval lambda */
lval* lval_lambda(lval* formals, lval* body) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    
    /* Set Builtin to Null */
    v->builtin = NULL;
    
    /* Built new environment */
    v->env = lenv_new();
    
    /* Set Formals and Body */
    v->formals = formals;
    v->body = body;
    return v;
}

/* Create a new lval string */
lval* lval_str(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str  = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}

/* Create a new lenv */
lenv* lenv_new(void) {
    lenv* e  = malloc(sizeof(lenv));
    e->par   = NULL;
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

/* Copy an lenv */
lenv* lenv_copy(lenv* e) {
    lenv* n = malloc(sizeof(lenv));
    n->par = e->par;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);
    for (int i = 0; i < e->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}

/* Delete an lval */
void lval_del(lval* v) {
    switch (v->type) {
      case LVAL_NUM: break;
      case LVAL_FUN: 
        if (!v->builtin) {
          lenv_del(v->env);
          lval_del(v->formals);
          lval_del(v->body);
        }
      break;
      case LVAL_ERR: free(v->err); break;
      case LVAL_SYM: free(v->sym); break;
      case LVAL_QEXPR:
      case LVAL_SEXPR:
        for (int i = 0; i < v->count; i++) {
          lval_del(v->cell[i]);
        }
        free(v->cell);
      break;
      case LVAL_STR: free(v->str); break;
    }
    
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
      case LVAL_FUN:
        if (v->builtin) {
          x->builtin = v->builtin;
        } else {
          x->builtin = NULL;
          x->env = lenv_copy(v->env);
          x->formals = lval_copy(v->formals);
          x->body = lval_copy(v->body);
        }
      break;
      case LVAL_NUM: x->num = v->num; break;
      case LVAL_ERR: x->err = malloc(strlen(v->err) + 1); strcpy(x->err, v->err); break;
      case LVAL_SYM: x->sym = malloc(strlen(v->sym) + 1); strcpy(x->sym, v->sym); break;
      case LVAL_SEXPR:
      case LVAL_QEXPR:
        x->count = v->count;
        x->cell = malloc(sizeof(lval*) * x->count);
        for (int i = 0; i < x->count; i++) {
          x->cell[i] = lval_copy(v->cell[i]);
        }
      break;
      case LVAL_STR: x->str = malloc(strlen(v->str) + 1); strcpy(x->str, v->str); break;
    }
    return x;
}

/* Gets an lval from an lenv, or an error if it isn't there */
lval* lenv_get(lenv* e, lval* k) {
    /* Iterate over each item in the lenv */
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) { return lval_copy(e->vals[i]); }
    }
    
    /* If no symbol check in parent otherwise error */
    if (e->par) {
        return lenv_get(e->par, k);
    } else {
        return lval_err("Unbound Symbol '%s'", k->sym);
    }
}

/* Define a variable globally */
void lenv_def(lenv* e, lval* k, lval* v) {
    /* Iterate until e has no parent */
    while (e->par) { e = e->par; }
    /* Put val in e */
    lenv_put(e, k, v);
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
    lenv_add_builtin(e, "=",   builtin_put);
    lenv_add_builtin(e, "\\",  builtin_lambda);

    /* Comparison functions */
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);
    lenv_add_builtin(e, ">",  builtin_gt);
    lenv_add_builtin(e, "<",  builtin_lt);
    lenv_add_builtin(e, ">=", builtin_ge);
    lenv_add_builtin(e, "<=", builtin_le);

    /* String functions */
    lenv_add_builtin(e, "load", builtin_load);
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "error", builtin_error);
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

/* Print an lval string */
void lval_print_str(lval* v) {
    /* Make a Copy of the string */
    char* escaped = malloc(strlen(v->str)+1);
    strcpy(escaped, v->str);
    /* Pass it through the escape function */
    escaped = mpcf_escape(escaped);
    /* Print it between " characters */
    printf("\"%s\"", escaped);
    /* free the copied string */
    free(escaped);
}

/* Print an lval */
void lval_print(lval* v) {
    switch (v->type) {
      case LVAL_FUN:
        if (v->builtin) {
          printf("<builtin>");
        } else {
          printf("(\\ "); lval_print(v->formals); putchar(' '); lval_print(v->body); putchar(')');
        }
      break;
      case LVAL_NUM:   printf("%li", v->num); break;
      case LVAL_ERR:   printf("Error: %s", v->err); break;
      case LVAL_SYM:   printf("%s", v->sym); break;
      case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
      case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
      case LVAL_STR:   lval_print_str(v); break;
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

/* Read in a string */
lval* lval_read_str(mpc_ast_t* t) {
    /* Cut off the final quote character */
    t->contents[strlen(t->contents)-1] = '\0';
    /* Copy the string missing out the first quote character */
    char* unescaped = malloc(strlen(t->contents+1));
    strcpy(unescaped, t->contents+1);
    /* Pass through the unescape function */
    unescaped = mpcf_unescape(unescaped);
    /* Construct a new lval using the string */
    lval* str = lval_str(unescaped);
    /* Free the string and return */
    free(unescaped);
    return str;
}

/* Read in the AST */
lval* lval_read(mpc_ast_t* t) {
    /* If sym or num, convert to that type */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    /* If str, return the read string */
    if (strstr(t->tag, "string")) { return lval_read_str(t); }

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
        /* If comment, ignore */
        if (strstr(t->children[i]->tag, "comment")) { continue; }
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
    lval* result = lval_call(e, f, v);
    lval_del(f);
    return result;
}

/* Call a function */
lval* lval_call(lenv* e, lval* f, lval* a) {
    /* If Builtin then simply apply that */
    if (f->builtin) { return f->builtin(e, a); }

    /* Record Argument Counts */
    int given = a->count;
    int total = f->formals->count;

    /* While arguments still remain to be processed */
    while (a->count) {

    /* If we've ran out of formal arguments to bind */
    if (f->formals->count == 0) {
      lval_del(a);
      return lval_err("Function passed too many arguments. Got %i, Expected %i.", given, total); 
    }

    /* Pop the first symbol from the formals */
    lval* sym = lval_pop(f->formals, 0);

    /* Special Case to deal with '&' */
    if (strcmp(sym->sym, "&") == 0) {
      
      /* Ensure '&' is followed by another symbol */
      if (f->formals->count != 1) {
        lval_del(a);
        return lval_err("Function format invalid. Symbol '&' not followed by single symbol.");
      }
      
      /* Next formal should be bound to remaining arguments */
      lval* nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(sym); lval_del(nsym);
      break;
    }

    /* Pop the next argument from the list */
    lval* val = lval_pop(a, 0);

    /* Bind a copy into the function's environment */
    lenv_put(f->env, sym, val);

    /* Delete symbol and value */
    lval_del(sym); lval_del(val);
    }

    /* Argument list is now bound so can be cleaned up */
    lval_del(a);

    /* If '&' remains in formal list it should be bound to empty list */
    if (f->formals->count > 0 &&
    strcmp(f->formals->cell[0]->sym, "&") == 0) {

    /* Check to ensure that & is not passed invalidly. */
    if (f->formals->count != 2) {
      return lval_err("Function format invalid. Symbol '&' not followed by single symbol.");
    }

    /* Pop and delete '&' symbol */
    lval_del(lval_pop(f->formals, 0));

    /* Pop next symbol and create empty list */
    lval* sym = lval_pop(f->formals, 0);
    lval* val = lval_qexpr();

    /* Bind to environment and delete */
    lenv_put(f->env, sym, val);
    lval_del(sym); lval_del(val);
    }

    /* If all formals have been bound evaluate */
    if (f->formals->count == 0) {

    /* Set Function Environment parent to current evaluation Environment */
    f->env->par = e;

    /* Evaluate and return */
    return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    } else {
    /* Otherwise return partially evaluated function */
    return lval_copy(f);
    }
}

/* See if two lvals are equal to each other by checking fields */
int lval_eq(lval* x, lval* y) {
    /* Diff types are unequal */
    if (x->type != y->type) { return 0; }

    /* Compare based on type */
    switch (x->type) {
        /* Compare numbers */
        case LVAL_NUM: return (x->num == y->num);

        /* Compare strings */
        case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
        case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
        case LVAL_STR: return (strcmp(x->str, y->str) == 0);

        /* If builtin compare functions, otherwise formals and body */
        case LVAL_FUN:
            if (x->builtin) {
                return x->builtin == y->builtin;
            } else {
                return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body);
            }

        /* If list compare each element */
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->count != y->count) { return 0; }
            for (int i = 0; i < x->count; i++) {
                /* If any element unequal, lists unequal */
                if (!lval_eq(x->cell[i], y->cell[i])) { return 0; }
            }
            /* Otherwise lists are equal */
            return 1;
        break;
    }
    return 0;
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

/* Builtin function for defining variables */
lval* builtin_var(lenv* e, lval* a, char* func) {
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);
    
    lval* syms = a->cell[0];
    for (int i = 0; i < syms->count; i++) {
      LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
        "Function '%s' cannot define non-symbol. Got %s, Expected %s.",
        func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }
    
    LASSERT(a, (syms->count == a->count-1),
      "Function '%s' passed too many arguments for symbols. Got %i, Expected %i.",
      func, syms->count, a->count-1);
      
    for (int i = 0; i < syms->count; i++) {
      /* If 'def' define in global scope. If 'put' define in local scope */
      if (strcmp(func, "def") == 0) { lenv_def(e, syms->cell[i], a->cell[i+1]); }
      if (strcmp(func, "=")   == 0) { lenv_put(e, syms->cell[i], a->cell[i+1]); } 
    }
    
    lval_del(a);
    return lval_sexpr();
}

/* Builtin functions for `def` and `put` */
lval* builtin_def(lenv* e, lval* a) { return builtin_var(e, a, "def"); }
lval* builtin_put(lenv* e, lval* a) { return builtin_var(e, a, "="); }

/* Builtin function for `\` */
lval* builtin_lambda(lenv* e, lval* a) {
    /* Check Two arguments, each of which are Q-Expressions */
    LASSERT_NUM("\\", a, 2);
    LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);
    
    /* Check first Q-Expression contains only Symbols */
    for (int i = 0; i < a->cell[0]->count; i++) {
      LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
        "Cannot define non-symbol. Got %s, Expected %s.",
        ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }
    
    /* Pop first two arguments and pass them to lval_lambda */
    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);
    
    return lval_lambda(formals, body);
}

/* Builtin function for ordering */
lval* builtin_ord(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    LASSERT_TYPE(op, a, 0, LVAL_NUM);
    LASSERT_TYPE(op, a, 1, LVAL_NUM);

    int r;
    if (strcmp(op, ">")  == 0) { r = (a->cell[0]->num >  a ->cell[1]->num); }
    if (strcmp(op, "<")  == 0) { r = (a->cell[0]->num <  a ->cell[1]->num); }
    if (strcmp(op, ">=") == 0) { r = (a->cell[0]->num >= a ->cell[1]->num); }
    if (strcmp(op, "<=") == 0) { r = (a->cell[0]->num <= a ->cell[1]->num); }
    lval_del(a);
    return lval_num(r);
}

/* Builtin functions for comparisons */
lval* builtin_gt(lenv* e, lval* a) { return builtin_ord(e, a, ">"); }
lval* builtin_lt(lenv* e, lval* a) { return builtin_ord(e, a, "<"); }
lval* builtin_ge(lenv* e, lval* a) { return builtin_ord(e, a, ">="); }
lval* builtin_le(lenv* e, lval* a) { return builtin_ord(e, a, "<="); }

/* Builtin function for equality comparing */
lval* builtin_cmp(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    int r;
    if (strcmp(op, "==") == 0) { r =  lval_eq(a->cell[0], a->cell[1]); }
    if (strcmp(op, "!=") == 0) { r = !lval_eq(a->cell[0], a->cell[1]); }
    lval_del(a);
    return lval_num(r);
}

/* Builtin functions for equality testing */
lval* builtin_eq(lenv* e, lval* a) { return builtin_cmp(e, a, "=="); }
lval* builtin_ne(lenv* e, lval* a) { return builtin_cmp(e, a, "!="); }

/* Builtin function for if statements */
lval* builtin_if(lenv* e, lval* a) {
    LASSERT_NUM("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_NUM);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    /* Mark both expressions as evaluable */
    lval* x;
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    if (a->cell[0]->num) {
        /* If condition is true, eval first expr */
        x = lval_eval(e, lval_pop(a, 1));
    } else {
        /* Otherwise, eval second expr */
        x = lval_eval(e, lval_pop(a, 2));
    }

    /* Delete arglist and return */
    lval_del(a);
    return x;
}

/* Builtin function for loading files */
lval* builtin_load(lenv* e, lval* a) {
    LASSERT_NUM("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);

    /* Parse File given by string name */
    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Lispy, &r)) {

    /* Read contents */
    lval* expr = lval_read(r.output);
    mpc_ast_delete(r.output);

    /* Evaluate each Expression */
    while (expr->count) {
        lval* x = lval_eval(e, lval_pop(expr, 0));
        /* If Evaluation leads to error print it */
        if (x->type == LVAL_ERR) { lval_println(x); }
        lval_del(x);
    }

    /* Delete expressions and arguments */
    lval_del(expr);    
    lval_del(a);

    /* Return empty list */
    return lval_sexpr();

    } else {
        /* Get Parse Error as String */
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        /* Create new error message using it */
        lval* err = lval_err("Could not load Library %s", err_msg);
        free(err_msg);
        lval_del(a);

        /* Cleanup and return error */
        return err;
    }
}

/* Builtin function to print strings */
lval* builtin_print(lenv* e, lval* a) {
    /* Print each arg followed by a space */
    for (int i = 0; i < a->count; i++) {
        lval_print(a->cell[i]);
        putchar(' ');
    }
    /* Print a newline and delete args */
    putchar('\n');
    lval_del(a);

    return lval_sexpr();
}

/* Builtin function to print errors */
lval* builtin_error(lenv* e, lval* a) {
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    /* Construct error */
    lval* err = lval_err(a->cell[0]->str);

    /* Delete args and return */
    lval_del(a);
    return err;
}

int main(int argc, char** argv) {
	/* Create parsers */
	Number  = mpc_new("number");
    Symbol  = mpc_new("symbol");
    String  = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr   = mpc_new("sexpr");
    Qexpr   = mpc_new("qexpr");
	Expr    = mpc_new("expr");
	Lispy   = mpc_new("lispy");

	/* Define parsers */
	mpca_lang(MPC_LANG_DEFAULT,
      "                                              \
        number  : /-?[0-9]+/ ;                       \
        symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
        string  : /\"(\\\\.|[^\"])*\"/ ;             \
        comment : /;[^\\r\\n]*/ ;                    \
        sexpr   : '(' <expr>* ')' ;                  \
        qexpr   : '{' <expr>* '}' ;                  \
        expr    : <number>  | <symbol> | <string>    \
                | <comment> | <sexpr>  | <qexpr>;    \
        lispy   : /^/ <expr>* /$/ ;                  \
      ",
      Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);


	/* Print Version and Exit Information */
	puts("Lispy Version 0.0.3.0.0");
	puts("Press Ctrl+c to Exit\n");

    /* Create a new environment */
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    /* Supplied with list of files */
    if (argc >= 2) {
        /* Loop other each filename */
        for (int i = 1; i < argc; i++) {
            /* Create an arglist with a single arg being the filename */
            lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));

            /* Pass to builtin_load and get the result */
            lval* x = builtin_load(e, args);

            /* If there's an error, print it */
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }
    }
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
	mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

	return 0;
}