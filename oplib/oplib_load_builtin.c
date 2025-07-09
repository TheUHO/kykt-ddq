
#include	"passes.h"
#include	"pool.h"


static	ast	builtin = NULL;


void *	load_builtin(char *key)
{
	ast_node	res;

	res = ast_map_get(builtin, ast_access(builtin, ast_type_str, (ast_value){.s0=key}));

	if (res && res->t == ast_type_ptr)
		return	res->v.p;
	else
		return	NULL;
}

void	load_builtin_finish()
{
	ast_delete(builtin);
	builtin = NULL;
}

void	load_builtin_register(char *key, void *value)
{
	ast_map_set(builtin, ast_access(builtin, ast_type_str, (ast_value){.s0=key}), ast_access(builtin, ast_type_ptr, (ast_value){.p=value}));
}


#include	"oplib_load_builtin_gen.h"

void	load_builtin_init()
{
	if (!builtin)
	{
		builtin = ast_new();

#include	"oplib_load_builtin_gen.c"

	}
}

