
#include	<string.h>
#include	<stdio.h>

#include	"ddq.h"
#include	"ast.h"
#include	"pool.h"
#include	"ast.tab.h"
#include	"ast.lex.h"


/*
 * debug functions
 */

void	ast_print(ast_node n)
{
	char *	typelist[] = {
		"ast_type_empty", 
		"ast_type_atom_start", "ast_type_any", "ast_type_int", "ast_type_bool", "ast_type_real", "ast_type_ptr", "ast_type_fun", "ast_type_obj", "ast_type_call", "ast_type_str", "ast_type_atom_end",
		"ast_type_n1_start", "ast_type_anchor", "ast_type_name", "ast_type_load_data", "ast_type_load_builtin", "ast_type_load_python", "ast_type_load_so_file", "ast_type_load_ast", "ast_type_n1_end",
		"ast_type_n2_start", "ast_type_eq", "ast_type_dup", "ast_type_defcall", "ast_type_prop", "ast_type_load_op", "ast_type_load_type", "ast_type_load_so", "ast_type_map", "ast_type_n2_end"
	};

	if (!n)
	{
		printf("{NULL}");
		return;
	}

	printf("{[%p]%s:", n, typelist[n->t]);
	switch (n->t)
	{
		case	ast_type_any :
		case	ast_type_int :
		case	ast_type_bool :
			printf("%i", n->v.i);
			break;
		case	ast_type_real :
			printf("%.17lg", n->v.r);
			break;
		case	ast_type_obj :
			printf("(%p)", ((obj)(n->v.p))->p);
		case	ast_type_ptr :
			printf("%p", n->v.p);
			break;
		case	ast_type_str :
			printf("%s", n->v.s);
			break;
		default	:
			if (n->t > ast_type_n1_start && n->t < ast_type_n1_end)
				ast_print(n->v.n1);
			else if (n->t > ast_type_n2_start && n->t < ast_type_n2_end)
			{
				ast_print(n->v.n2[0]);
				ast_print(n->v.n2[1]);
			}
	}
	printf("}");
	fflush(stdout);
}


/*
 * ast type functions
 */

ast_group_type	ast_gtype(ast_node n)
{
	switch (n->t)
	{
		case	ast_type_eq :
		case	ast_type_dup :
		case	ast_type_defcall :
		case	ast_type_map :
			return	ast_group_type_script;
		case	ast_type_any :
		case	ast_type_anchor :
		case	ast_type_name :
		case	ast_type_prop :
		case	ast_type_load_ast :
		case	ast_type_load_op :
		case	ast_type_load_type :
			return	ast_group_type_lvalue;
		case	ast_type_int :
		case	ast_type_bool :
		case	ast_type_real :
		case	ast_type_ptr :
		case	ast_type_fun :
		case	ast_type_obj :
		case	ast_type_call :
		case	ast_type_str :
		case	ast_type_load_data :
		case	ast_type_load_builtin :
		case	ast_type_load_python :
		case	ast_type_load_so_file :
		case	ast_type_load_so :
			return	ast_group_type_rvalue;
		default	:
			return	ast_group_type_unknown;
	}
}



/*
 * the parser
 */

ast		str2ast(char *p)
{
	yyscan_t	scan;
	ast		res;

	yylex_init(&scan);
	res = ast_new();

	yy_scan_string(p, scan);
	yyparse(scan, res);

	yylex_destroy(scan);

	return	res;
}

ast		file2ast(char *fn)
{
	ast		res;
	yyscan_t	scan;
	FILE *		fp;

	yylex_init(&scan);
	res = ast_new();

	fp = fopen(fn, "r");				// FIXME: 文件的错误处理
	yypush_buffer_state(yy_create_buffer(fp, YY_BUF_SIZE, scan), scan);
	yyparse(scan, res);
	fclose(fp);

	yylex_destroy(scan);

	return	res;
}


/*
 * 有关load的辅助函数
 */

ast_node	ast_load(ast pool, int type, char *buf)
{
	ast_value	res;
	char		*p, *p1;

	if (ast_type_n1_start < type && type < ast_type_n1_end)
		res.n1 = ast_access(pool, ast_type_str, (ast_value){.s0=buf});
	else
		switch (type)
		{
			case	ast_type_load_op :
			case	ast_type_load_type :
				res.n2[0] = ast_load(pool, ast_type_load_ast, buf);
				res.n2[1] = ast_access(pool, ast_type_any, (ast_value){.i=-1});
				break;

			case	ast_type_load_so :
				for (p=buf, p1=NULL; *p; p++)
					if (*p == '/')
						p1 = p;
				if (!p1)
					;		// FIXME: 报错load参数不对
				*p1 = '\0';
				res.n2[0] = ast_load(pool, ast_type_load_so_file, buf);
				res.n2[1] = ast_access(pool, ast_type_str, (ast_value){.s0=p1+1});
				break;

				// TODO: 其他需要缓冲的动态库调用等 ...

			default	:
				// FIXME: 这里得报错
				break;
		}

	return	ast_access(pool, type, res);
}


