#include	"common_objs.h"


void *	obj_mem_new_size(int_t bufsize)
{
	obj_mem	res;

	res = malloc(sizeof(obj_mem_t));
	res->type = obj_mem_default;
	res->bufsize = bufsize;
	res->size = 0;
	res->p = malloc(bufsize);

	return	res;
}

void *	obj_mem_new()
{
	obj_mem res;

	res = malloc(sizeof(obj_mem_t));
	res->type = obj_mem_default;
	res->bufsize = 0;
	res->size = 0;
	res->p = NULL;

	return	res;
}

void	obj_mem_del(void *p)
{
	free(((obj_mem)p)->p);
	((obj_mem)p)->p = NULL;
	free(p);
}


void *	obj_var_new()
{
	return	malloc(sizeof(obj_var_t));	// FIXME: 这里是否需要清零？
}

void	obj_var_del(void *p)
{
	free(p);
}

