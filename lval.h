#ifndef LVAL_H
#define LVAL_H

#include "mpc.h"

typedef struct _lval {
    int type;
    long num;
    char *err;
    char *sym;
    int count;
    struct _lval **cell;
} lval;

enum {
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_SEXPR,
};

enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM,
};

inline static lval* lval_num(long x)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

inline static lval* lval_err(char *m)
{
    lval *v = malloc(sizeof (lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

inline static lval* lval_sym(char *s)
{
    lval *v = malloc(sizeof (lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

inline static lval* lval_sexpr(void)
{
    lval *v = malloc(sizeof (lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

inline static void lval_del(lval *v)
{
    switch (v->type) {
        case LVAL_NUM:          /* do nothing */
                break;
        case LVAL_ERR:
                free(v->err);
                break;
        case LVAL_SYM:
                free(v->sym);
                break;

        case LVAL_SEXPR:
                for (int i = 0; i < v->count; i++) {
                    lval_del(v->cell[i]);
                }
                free(v->cell);
                break;
    }

    free(v);
}

inline static lval* lval_read_num(mpc_ast_t *t)
{
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

inline static lval* lval_add(lval *v, lval *x)
{
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}

inline static lval * lval_read(mpc_ast_t *t)
{
    if (strstr(t->tag, "number"))
        return lval_read_num(t);

    if (strstr(t->tag, "symbol"))
        return lval_sym(t->contents);

    lval *x = NULL;
    if (!strcmp(t->tag, ">"))
        x = lval_sexpr();
    if (strstr(t->tag, "sexpr"))
        x = lval_sexpr();

    for (int i = 0; i < t->children_num; i++) {
        if (!strcmp(t->children[i]->contents, "("))
            continue;
        if (!strcmp(t->children[i]->contents, ")"))
            continue;
        if (!strcmp(t->children[i]->tag, "regex"))
            continue;

        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

inline static void lval_print(lval *v);

inline static void lval_expr_print(lval *v, char open, char close)
{
    putchar(open);

    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);

        if (i != (v->count - 1))
            putchar(' ');
    }

    putchar(close);
}

inline static void lval_print(lval *v)
{
    switch (v->type) {
        case LVAL_NUM:
                printf("%li", v->num);
                break;
        case LVAL_SYM:
                printf("%s", v->sym);
                break;
        case LVAL_ERR:
                printf("Error: %s", v->err);
                break;
        case LVAL_SEXPR:
                lval_expr_print(v, '(', ')');
                break;
    }
}

inline static void lval_println(lval *v)
{
    lval_print(v);
    putchar('\n');
}

inline static lval * lval_pop(lval *v, int i)
{
    /* Find the item at "i" */
    lval *x = v->cell[i];

    /* Shift memory after the item at "i" over the top */
    memmove(&v->cell[i], &v->cell[i+1], 
            sizeof (lval *) * (v->count - i - 1));

    /* Decrease the count of items in the lis */
    v->count--;

    /* Reallocate the memory used */
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    return x;
}

inline static lval * lval_take(lval *v, int i)
{
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}

inline static lval * builtin_op(lval *a, char* op)
{
    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on non-number!");
        }
    }

    /* Pop the first element */
    lval *x = lval_pop(a, 0);

    /* If no arguments and sub then perform unary negation */
    if (!strcmp (op, "-") && a->count == 0)
        x->num = -x->num;

    /* While there are still elements remaining */
    while (a->count > 0) {
        /* Pop the next element */
        lval *y = lval_pop(a, 0);

        if (!strcmp(op, "+"))
            x->num += y->num;
        if (!strcmp(op, "-"))
            x->num -= y->num;
        if (!strcmp(op, "*"))
            x->num *= y->num;
        if (!strcmp(op, "/")) {
            if (y->num == 0) {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division By Zero!");
                break;
            }
            x->num /= y->num;
        }

        lval_del(y);
    }

    lval_del(a);
    return x;
}

inline static lval* lval_eval(lval *v);

inline static lval * lval_eval_sexpr(lval* v)
{
    /* Evaluate children */
    for (int i = 0; i < v->count; i++)
        v->cell[i] = lval_eval(v->cell[i]);

    /* Error checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            lval_take(v, i);
        }
    }

    /* Empty expression */
    if (v->count == 0)
        return v;

    /* Single expression */
    if (v->count == 1)
        return lval_take(v, 0);

    /* Ensure first element is symbol */
    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f);
        lval_del(v);
        return lval_err("S-expression does not start with symbol!");
    }

    /* Call builtin with operator */
    lval * result = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}

inline static lval* lval_eval(lval *v)
{
    if (v->type == LVAL_SEXPR)
        return lval_eval_sexpr(v);

    return v;
}

#endif /* LVAL_H */
