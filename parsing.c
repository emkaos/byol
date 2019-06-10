#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include "mpc.h"

long eval_op(long a1, char* op, long a2) {
	if (strcmp(op, "+") == 0) { return a1 + a2; }
	if (strcmp(op, "-") == 0) { return a1 - a2; }
	if (strcmp(op, "*") == 0) { return a1 * a2; }
	if (strcmp(op, "/") == 0) { return a1 / a2; }
	if (strcmp(op, "%") == 0) { return a1 % a2; }
	return 0;
}

long eval(mpc_ast_t* t) {
	/* Number */
	if (strstr(t->tag, "number")) {
		return atoi(t->contents);
	}

	/* operator is always second child */
	char* op = t->children[1]->contents;

	/* first argument to operator */
	long x = eval(t->children[2]);

	int i=3;
	while(strstr(t->children[i]->tag, "expression")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

int main(int argc, char** argv) {
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expression = mpc_new("expression");
	mpc_parser_t* Lispy = mpc_new("lispy");

	mpca_lang(MPCA_LANG_DEFAULT, "\
			number		: /-?[0-9]+/ ;\
			operator	: '+' | '-' | '*' | '/' | '%' ;\
			expression	: <number> | '(' <operator> <expression>+ ')';\
			lispy		: /^/ <operator> <expression>+ /$/ ;\
			", 
			Number, Operator, Expression, Lispy);

	puts("Lispy version 0.0.0.0.3");
	puts("Press Ctrl-C to exit");
	while(1) {
		char* input = readline("lispy> ");
		add_history(input);

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			mpc_ast_print(r.output);

			long result = eval(r.output);
			printf("%li\n", result);

			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}
	mpc_cleanup(4, Number, Operator, Expression, Lispy);
	return 0;
}
