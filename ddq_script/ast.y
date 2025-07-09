%define api.pure full
%locations

%lex-param {yyscan_t scanner}{ast pool}
%parse-param {yyscan_t scanner}{ast pool}

%code top
{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "pool.h"

static char *		strunwrap(char *s);
ast_node		mk_node(ast pool, ast_type t, ast_value v);
static ast_node_list	ast_node_list_add(ast_node_list l, ast_node n, ast_node v);
static void		ast_node_list_free(ast_node_list l);
static void		ast_node_list_map(ast pool, ast_node_list l, ast_node op, int d);
}

%code requires
{
	typedef void* yyscan_t;
}

%code
{
	int yylex(YYSTYPE* yylvalp, YYLTYPE* yyllocp, yyscan_t scanner, ast pool);
	void yyerror(YYLTYPE* yyllocp, yyscan_t unused, ast pool, const char* msg);
}

%union
{
	char *		str;
	int		num_i;
	double		num_r;
	ast_node	node;
	ast_node_list	list;
}

%token <str> STRING
%token <str> QUOTE_STRING
%token <str> PATH_STRING
%token <num_i> INTEGER
%token <num_r> REAL
%token <num_i> BOOLEAN
%token <num_i> LOAD_OP LOAD
%token	<num_i> DEFCALL

%type <node> dup eq any lvalue rvalue property name load_op connect2 defcall op
%type <list> connect1 list

%%


script	:	%empty					;
       	|	script eq				;
	|	script dup				;
	|	script defcall				;
	|	script connect				;

eq	:	any '=' any				{ ast_insert(pool, ast_type_eq, (ast_value){.n2={$1, $3}}); $$ = $1; }
	|	eq '=' any				{ ast_insert(pool, ast_type_eq, (ast_value){.n2={$1, $3}}); $$ = $1; }

dup	:	lvalue '~' lvalue			{ ast_insert(pool, ast_type_dup, (ast_value){.n2={$1, $3}}); $$ = $1; }
	|	dup '~' lvalue				{ ast_insert(pool, ast_type_dup, (ast_value){.n2={$1, $3}}); $$ = $1; }

defcall :	lvalue DEFCALL load_op 			{ ast_insert(pool, ast_type_defcall, (ast_value){.n2={$1, $3}}); $$ = $3; }
		|   lvalue DEFCALL lvalue			{ ast_insert(pool, ast_type_defcall, (ast_value){.n2={$1, $3}}); $$ = $1; }

any	:	lvalue					{ $$ = $1; }
    	|	rvalue					{ $$ = $1; }

lvalue	:	name					{ $$ = $1; }
	|	property				{ $$ = $1; }

rvalue	:	LOAD PATH_STRING			{ $$ = ast_load(pool, $1, strunwrap($2)); free($2); }
	|	INTEGER					{ $$ = mk_node(pool, ast_type_int, (ast_value){.i=$1}); }
	|	REAL					{ $$ = mk_node(pool, ast_type_real, (ast_value){.r=$1}); }
	|	BOOLEAN					{ $$ = mk_node(pool, ast_type_bool, (ast_value){.i=$1}); }
	|	QUOTE_STRING				{ $$ = mk_node(pool, ast_type_str, (ast_value){.s0=strunwrap($1)}); free($1); }
	|	load_op					{ $$ = $1; }

property :	lvalue '.' STRING			{ $$ = ast_access(pool, ast_type_prop, (ast_value){.n2={$1, ast_access(pool, ast_type_str, (ast_value){.s0=$3})}}); free($3); }
	|	lvalue '.' INTEGER			{ $$ = ast_access(pool, ast_type_prop, (ast_value){.n2={$1, ast_access(pool, ast_type_int, (ast_value){.i=$3})}}); }

name	:	STRING					{ $$ = ast_access(pool, ast_type_name, (ast_value){.n1=ast_access(pool, ast_type_str, (ast_value){.s0=$1})}); free($1); }

load_op	:	LOAD_OP PATH_STRING			{ $$ = ast_load(pool, $1, strunwrap($2)); free($2); }

connect	:	connect1				{ ast_node_list_free($1); }
	|	connect2				;

connect1 :	op '<' '[' list ']'			{ ast_node_list_map(pool, $4, $1, -1); $$ = $4; }
	 |	connect2 '<' '[' list ']'		{ ast_node_list_map(pool, $4, $1, -1); $$ = $4; }

connect2 :	'[' list ']' '<' op			{ ast_node_list_map(pool, $2, $5, 1); ast_node_list_free($2); $$ = $5; }
	 |	connect1 '<' op				{ ast_node_list_map(pool, $1, $3, 1); ast_node_list_free($1); $$ = $3; }

op	:	lvalue					{ ast_node n=ast_access(pool, ast_type_anchor, (ast_value){.n1=ast_access(pool, ast_type_any, (ast_value){.i=-1})}); ast_insert(pool, ast_type_eq, (ast_value){.n2={n, $1}}); $$ = n; }
	|	load_op					{ $$ = $1; }

list	:	%empty					{ $$ = NULL; }
     	|	list any				{ ast_node_list p = ast_node_list_add($1, NULL, $2); $$ = $1 ? $1 : p; }
	|	list STRING ':' any			{ ast_node_list p = ast_node_list_add($1, ast_access(pool, ast_type_str, (ast_value){.s0=$2}), $4); $$ = $1 ? $1 : p; free($2); }



%%


void	yyerror(YYLTYPE* yyllocp, yyscan_t unused, ast pool, const char* msg)
{
	fprintf(stderr, "[%d:%d]: %s\n", yyllocp->first_line, yyllocp->first_column, msg);
}

static char *	strunwrap(char *s)		// TODO: 如果是用括号包围的字符串，应清理掉空格、制表符等
{
	char *p;

	for (p=s; *p++;)
		p[-1] = p[0];
	p[-3] = '\0';

	return	s;
}

ast_node	mk_node(ast pool, ast_type t, ast_value v)
{
	ast_node	res;

	res = ast_access(pool, ast_type_any, (ast_value){.i=-1});
	ast_insert(pool, ast_type_eq, (ast_value){.n2={res, ast_access(pool, t, v)}});

	return	res;
}

static ast_node_list	ast_node_list_add(ast_node_list l, ast_node n, ast_node v)
{
	if (l)
	{
		for (; l->next; l=l->next);
		l->next = (ast_node_list) malloc(sizeof(ast_node_list_t));
		l->next->next = NULL;
		l->next->name = n;
		l->next->value = v;
		return l->next;
	}
	else
	{
		l = (ast_node_list) malloc(sizeof(ast_node_list_t));
		l->next = NULL;
		l->name = n;
		l->value = v;
		return	l;
	}
}

static void	ast_node_list_free(ast_node_list l)
{
	ast_node_list	l2;
	for (; l; l=l2)
	{
		l2 = l->next;
		free(l);
	}
}

static void	ast_node_list_map(ast pool, ast_node_list l, ast_node op, int d)
{
	for (int i = d; l; l=l->next)
		if (l->name)
			ast_insert(pool, ast_type_eq, (ast_value){.n2={ ast_access(pool, ast_type_prop, (ast_value){.n2={op, l->name}}), l->value}});
		else
		{
			ast_insert(pool, ast_type_eq, (ast_value){.n2={ ast_access(pool, ast_type_prop, (ast_value){.n2={op, ast_access(pool, ast_type_int, (ast_value){.i=i})}}), l->value}});
			i += d;
		}
}



