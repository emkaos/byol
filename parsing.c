#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include "mpc.h"

// Value type
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };
// Error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef struct lval {
	int type;
	double num;
	char* err;
	char* sym;
	int count; //lval count
	struct lval** cell;
} lval;

// create a lval number
lval* lval_num(double x) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

// create an lval error
lval lval_err(char* m) {
	lval v = maloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
}

// create a lval symbol
lval lval_sym(char* s) {
	lval v = maloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(m) + 1);
	strcpy(v->sym, s);
	return v;
}

// create a lval sym expression
lval lval_sexpr() {
	lval v = maloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v-> count = 0;
	v-> cell = NULL;
	return v;
}

lval_del(lval* v) {
	switch (v->type) {
		case LVAL_NUM: break;
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;
		case LVAL_SEXPR:
			for (int i=0;i < v->count; i++) {
				lval_del(v->cell[i]);
			}
			free(v->cell);
			break;
	}
	free(v);
}



void lval_print(lval v) {
	switch(v.type) {
		case LVAL_NUM: printf("%f", v.num); break;
		case LVAL_ERR: if (v.err == LERR_DIV_ZERO) {
						   printf("Error: Division By Zero!");
					   }
					   if (v.err == LERR_BAD_OP) {
						   printf("Error: Invalid Operator!");
					   }
					   if (v.err == LERR_BAD_NUM) {
						   printf("Error: Invalid Number!");
					   }
					   break;
	}
}

void lval_println(lval v) { lval_print(v); putchar('\n'); } 

lval eval_op(lval a1, char* op, lval a2) {
	if (a1.type == LVAL_ERR) { return a1; }
	if (a2.type == LVAL_ERR) { return a2; }

	if (strcmp(op, "+") == 0) { return lval_num(a1.num + a2.num); }
	if (strcmp(op, "-") == 0) { return lval_num(a1.num - a2.num); }
	if (strcmp(op, "*") == 0) { return lval_num(a1.num * a2.num); }
	if (strcmp(op, "/") == 0) { 
		if ( a2.num == 0 ) {
			return lval_err(LERR_DIV_ZERO);
		} else {
			return lval_num(a1.num / a2.num); 
		}
	}
	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
	/* Number */
	if (strstr(t->tag, "number")) {
		errno = 0;
		double x = strtod(t->contents, NULL);
		if (errno == ERANGE) {
			lval_err(LERR_BAD_NUM);
		} else {
			return lval_num(x);
		}
	}

	/* operator is always second child */
	char* op = t->children[1]->contents;

	/* first argument to operator */
	lval x = eval(t->children[2]);

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
	mpc_parser_t* Sexpression = mpc_new("sexpression");

	mpca_lang(MPCA_LANG_DEFAULT, "\
			number		: /-?[0-9]+(\\.[0-9]+)?/ ;\
			operator	: '+' | '-' | '*' | '/'  ;\
			sexpression : '(' <expression>* ')' ;\
			expression	: <number> | '(' <operator> <expression>+ ')';\
			lispy		: /^/ <operator> <expression>+ /$/ ;\
			", 
			Number, Operator, Expression, Lispy);

	puts("Lispy version 0.0.0.0.4");
	puts("Press Ctrl-C to exit");
	while(1) {
		char* input = readline("lispy> ");
		add_history(input);

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			//mpc_ast_print(r.output);

			lval result = eval(r.output);
			lval_println(result);

			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}
	mpc_cleanup(4, Number, Operator, Expression, Lispy, Sexpression);
	return 0;
}
