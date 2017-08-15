#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <editline/history.h>
#include <errno.h>

#include "lval.h"

int number_of_nodes(mpc_ast_t *t)
{
    if (t->children_num == 0)
        return 1;

    if (t->children_num > 0) {
        int total = 1;
        for (int i = 0; i < t->children_num; i++) {
            total += number_of_nodes(t->children[i]);
        }
        return total;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
              " \
               number : /-?[0-9]+/ ;                                    \
               symbol: '+' | '-' | '*' | '/' ;                          \
               sexpr : '(' <expr>* ')' ;                                \
               expr : <number> | <symbol> | <sexpr> ;                   \
               lispy: /^/ <expr>* /$/ ;                                 \
              ",
              Number, Symbol, Sexpr, Expr, Lispy);

    puts("Lispy Version 0.0.1");
    puts("Press Ctrl + c to exit\n");

    while (1) {

        char *input = readline("lispy> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval *x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
            /* /\* printf("number of nodes: %d\n", number_of_nodes(r.output)); *\/ */
            /* mpc_ast_print(r.output); */
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

    return 0;
}
