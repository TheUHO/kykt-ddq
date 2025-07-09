#ifndef	F_AST_H
#define	F_AST_H

#include	"uthash.h"

#include	"task_types.h"


//	data structures

typedef	struct ast_node_t	ast_node_t;
typedef	ast_node_t *		ast_node;

typedef	enum
{
	ast_type_empty = 0,	// not used

	ast_type_atom_start,

	ast_type_any,		// i

	ast_type_int,		// i
	ast_type_bool,		// i : TRUE=1, FALSE=0

	ast_type_real,		// r

	ast_type_ptr,		// p
	ast_type_fun,		// f

	ast_type_obj,		// p
	ast_type_call,		// p

	ast_type_str,		// s, s0

	ast_type_atom_end,

	ast_type_n1_start,

	ast_type_anchor,	// n1 : ast_type_any	, 在cache里是ast_type_load_ast

	ast_type_name,		// n1 : ast_type_str

	ast_type_load_data,	// n1 : ast_type_str

	ast_type_load_builtin,	// n1 : ast_type_str			v : void*

	ast_type_load_python,	// n1 : ast_type_str

	ast_type_load_so_file,	// n1 : ast_type_str			v : void*

	ast_type_load_ast,	// n1 : ast_type_str			v : ast

	ast_type_n1_end,

	ast_type_n2_start,

	ast_type_eq,		// n2

	ast_type_dup,		// n2

	ast_type_defcall,	// n2 : ast_type_name, ast_type_load_op / ast_type_name

	ast_type_prop,		// n2

	ast_type_load_op,	// n2 : ast_type_load_ast, ast_type_any

	ast_type_load_type,	// n2 : ast_type_load_ast, ast_type_any

	ast_type_load_so,	// n2 : ast_type_load_so_file, ast_type_str	v : pfun

	ast_type_map,		// n2


	ast_type_n2_end,

	// ...

	ast_type_last
} ast_type;

typedef	union
{
	int		i;
	double		r;
	void		*p;
	task_f_wildcard	*f;
	ast_node	n1;
	ast_node	n2[2];
	char		*s0;
	char		s[1];
} ast_value;

struct	ast_node_t
{
	ast_type	t;
	ast_value	v;
};

typedef	struct ast_t	ast_t;
typedef	ast_t *		ast;


typedef	struct ast_node_list_t	ast_node_list_t;
typedef	ast_node_list_t *	ast_node_list;
struct	ast_node_list_t
{
	ast_node	name;
	ast_node	value;
	ast_node_list	next;
};

typedef	enum
{
	ast_group_type_unknown,
	ast_group_type_script,
	ast_group_type_lvalue,
	ast_group_type_rvalue
} ast_group_type;


//	debug functions
void	ast_print(ast_node n);

//	ast type functions
ast_group_type	ast_gtype(ast_node n);

//	the parser
ast		str2ast(char *p);
ast		file2ast(char *fn);

ast_node	ast_load(ast pool, int type, char *buf);


#endif
