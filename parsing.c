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
lval* lval_err(char* m) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
}

// create a lval symbol
lval* lval_sym(char* s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

// create a lval sym expression
lval* lval_sexpr() {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

void lval_del(lval* v) {
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

lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long x = strtod(t->contents, NULL);
	return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_read(mpc_ast_t* t) {
	if (strstr(t->tag, "number")) { return lval_read_num(t);}
	if (strstr(t->tag, "symbol")) { return lval_sym(t->contents);}
	
	lval* x = NULL;
	if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); } // root element
	if (strstr(t->tag, "sexpression")) { x = lval_sexpr(); } 

	for (int i = 0; i < t->children_num; i++) {
		if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
		if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
		if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for (int i = 0; i < v->count; i++) {
		lval_print(v->cell[i]);

		if (i != (v->count-1)) {
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print(lval* v) {
	switch(v->type) {
		case LVAL_NUM: printf("%f", v->num); break;
		case LVAL_ERR: printf("Error: %s", v->err); break;
		case LVAL_SYM: printf("%s", v->sym); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
	}
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); } 

int main(int argc, char** argv) {
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Expression = mpc_new("expression");
	mpc_parser_t* Lispy = mpc_new("lispy");
	mpc_parser_t* Sexpression = mpc_new("sexpression");

	mpca_lang(MPCA_LANG_DEFAULT, "\
			number		: /-?[0-9]+(\\.[0-9]+)?/ ;\
			symbol   	: '+' | '-' | '*' | '/'  ;\
			sexpression : '(' <expression>* ')' ;\
			expression	: <number> | <symbol> | <sexpression> ;\
			lispy		: /^/ <expression>+ /$/ ;\
			", 
			Number, Symbol, Sexpression, Expression, Lispy);

	puts("Lispy version 0.0.0.0.4");
	puts("Press Ctrl-C to exit");
	while(1) {
		char* input = readline("lispy> ");
		add_history(input);

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			//mpc_ast_print(r.output);

			lval* x = lval_read(r.output);
			lval_println(x);
			lval_del(x);

			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}
	mpc_cleanup(4, Number, Symbol, Expression, Lispy, Sexpression);
	return 0;
}
