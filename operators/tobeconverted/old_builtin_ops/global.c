
//#include	"global.h"
//#include	"passes.h"
//#include	"common_objs.h"


/*
 * built-in types
 */

void *	new_int()
{
	return	malloc(sizeof(int));
}

void	del_int(void *p)
{
	free(p);
}

void *	new_real()
{
	return	malloc(sizeof(double));
}

void	del_real(void *p)
{
	free(p);
}

void	del_str(void *p)
{
	free(p);
}

void *	new_ast()
{
}

void *	del_ast()
{
}

void *	new_ring()
{
}

void	clear_ring(void *p)
{
}

void	del_ring(void *p)
{
}


/*
 * built-in operators
 */

task_ret	op_ast2ring(void **inputs, void **outputs, void **attribute)
	// inputs[0]是输入的ast，其中包含newop，可以不是消耗品
	// outputs[0]是输出的ring，一般是消耗品
	// inputs[1]和inputs[2]是外部输入和输出的个数，外部输入输出从inputs[3]和outputs[1]开始排列
{
	ast		pool;
	ast_node	i, j;
	ddq_op		ret;
	int		k;

	pool = pool_init();
	ast_merge(pool, *(ast *)inputs[0]);

	for (k=1; k<=*(int *)inputs[1]; k++)
		ast_eq(pool, ast_prop(pool, ast_name(pool, ast_str(pool, "newop")), ast_int(pool, -k-3)), ast_ptr(pool, inputs[k+2]));
	for (k=1; k<=*(int *)inputs[2]; k++)
		ast_eq(pool, ast_prop(pool, ast_name(pool, ast_str(pool, "newop")), ast_int(pool, k+1)), ast_ptr(pool, outputs[k]));

	pass_merge_ast(pool);

	pass_simpilify(pool);

	pass_apply_calls(pool);

	pass_create_objs(pool);

	clear_ring(outputs[0]);

	j = ast_str(pool, "processor");
	pool_iter(pool, i)
		if (i->type == ast_type_prop && i->p.n2[1] == j)
			ast_node2op(pool, (ddq_ring)outputs[0], i->p.n2[0]);

	ret = ddq_spawn((ddq_ring)outputs[0], processor_direct, 1, 0);
	ret->inputs[0] = obj_new(new_ring, del_ring, obj_prop_ready);
	*(ddq_ring)outputs[0] = ret;

	return	task_ret_ok;
}

task_ret	op_call(void **inputs, void **outputs, void **attribute)
	// inputs[0]是要被运行的ring
	// 外部的输入输出从inputs[1]和outputs[0]开始排列，算子忽略这些输入输出
{
	if (!(*(ddq_ring)inputs[0])->inputs[0]->p)
		(*(ddq_ring)inputs[0])->inputs[0]->p = ddq_get();

	if ((*(ddq_ring)inputs[0])->next == *(ddq_ring)inputs[0])		// FIXME: 确认一下指针是不是弄对了……
		return	task_ret_ok;
	else
	{
		ddq_put(*(ddq_ring)inputs[0]);
		return	task_ret_again;
	}
}

task_ret	op_return(void **inputs, void **outputs, void **attribute)
	// inputs[0]是要返回的那个调用它的ring
{
	ddq_put(*(ddq_ring)inputs[0]);

	return	task_ret_again;
}


/*
 * exported functions
 */
/*
void	global_init()
{
	if (!builtin)// && !cache)
	{
		builtin = pool_init();
		//cache = pool_init();

		builtin_set("new_int", (void *)new_int);
		builtin_set("del_int", (void *)del_int);
		builtin_set("new_real", (void *)new_real);
		builtin_set("del_real", (void *)del_real);
		// NOTE: no new_str
		builtin_set("del_str", (void *)del_str);
		builtin_set("new_ast", (void *)new_ast);
		builtin_set("del_ast", (void *)del_ast);
		builtin_set("new_ring", (void *)new_ring);
		builtin_set("del_ring", (void *)del_ring);

		// FIXME: defined in common/common_objs.c , move it somewhere here?
		//builtin_set("new_mem", (void *)obj_mem_new);
		//builtin_set("del_mem", (void *)obj_mem_del);
		//builtin_set("new_var", (void *)obj_var_new);
		//builtin_set("del_var", (void *)obj_var_del);

		builtin_set("op_ast2ring", (void *)op_ast2ring);
		builtin_set("op_call", (void *)op_call);

		// TODO: other builtin functions..

	}
}

void	builtin_set(char *key, void *value)
{
	ast_str(builtin, key)->v = value;
}

void *	builtin_get(char *key)
{
	return	ast_str(builtin, key)->v;
}
*/
//ast_node	cache_access(ast_node value)
//{
//	return	pool_insert(cache, value);
//}

