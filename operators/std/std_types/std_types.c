
#include	<stdint.h>
#include	<stdlib.h>
#include	<unistd.h>

#include	"ddq.h"
#include	"pool.h"
#include	"std_types.h"



/*
 * 一些辅助函数
 */

int	is_little_endian()
{
	int	i = 1;

	return	*((char *)&i);
}

void	copy_switch_endian(int nbytes, void *dest, void *src)
{
	int	i;

	for (i=0; i<nbytes; i++)
		((char*)dest)[i] = ((char*)src)[nbytes-1-i];
}



/*
 * built-in 类型
 */

void *	new_int()
{
	return	malloc(sizeof(int));
}

void	del_int(void *p)
{
	free(p);
}

task_ret	op_ser_int(void **inputs, void **outputs, void **attribute)
{
	obj_mem		pm = outputs[0];
	int *		ps = inputs[0];
	int32_t *	pd = pm->p;		// FIXME: 需要讨论是否应该把类型换成固定长度？下同
	int		size = 4;
	int32_t		buf;

	if (pm->type != obj_mem_default)
		;				// TODO: 非普通内存的情况可能需要分类处理，下同

	if (pm->bufsize < size)
	{
		if (pm->bufsize != 0)
			free(pm->p);
		pm->p = malloc(size);
		pm->bufsize = size;
	}
	pm->size = size;

	if (is_little_endian())			// NOTE: 默认序列化都是little_endian，下同
		*pd = *ps;
	else					// FIXME: 目前额外只考虑了big_endian，会遇到其他情况吗？下同
	{
		buf = *ps;
		copy_switch_endian(size, pd, &buf);
	}

	return	task_ret_ok;
}

task_ret	op_deser_int(void **inputs, void **outputs, void **attribute)
{
	obj_mem		pm = inputs[0];
	int32_t *	ps = pm->p;
	int *		pd = outputs[0];
	int		size = 4;
	int32_t		buf;

	if (pm->type != obj_mem_default)
		;				// TODO: 非普通内存的情况可能需要分类处理

	if (pm->size != size)
		;				// TODO: 这里得报错……

	if (is_little_endian())
		*pd = *ps;
	else
	{
		copy_switch_endian(size, &buf, ps);
		*pd = buf;
	}

	return	task_ret_ok;
}


void *	new_real()
{
	return	malloc(sizeof(double));
}

void	del_real(void *p)
{
	free(p);
}

task_ret	op_ser_real(void **inputs, void **outputs, void **attribute)
{
	obj_mem		pm = outputs[0];
	double *	ps = inputs[0];
	double *	pd = pm->p;
	int		size = sizeof(double);	// FIXME: 浮点数是否不太容易跨平台的固定表示？

	if (pm->type != obj_mem_default)
		;				// TODO: 非普通内存的情况可能需要分类处理，下同

	if (pm->bufsize < size)
	{
		if (pm->bufsize != 0)
			free(pm->p);
		pm->p = malloc(size);
		pm->bufsize = size;
	}
	pm->size = size;

	if (is_little_endian())
		*pd = *ps;
	else
		copy_switch_endian(size, pd, ps);

	return	task_ret_ok;
}

task_ret	op_deser_real(void **inputs, void **outputs, void **attribute)
{
	obj_mem		pm = inputs[0];
	double *	ps = pm->p;
	double *	pd = outputs[0];
	int		size = sizeof(double);

	if (pm->type != obj_mem_default)
		;				// TODO: 非普通内存的情况可能需要分类处理

	if (pm->size != size)
		;				// TODO: 这里得报错……

	if (is_little_endian())
		*pd = *ps;
	else
		copy_switch_endian(size, pd, ps);

	return	task_ret_ok;
}


void	del_str(void *p)
{
	free(p);
}

void *	new_mem()
{
	obj_mem res;

	res = malloc(sizeof(obj_mem_t));
	res->type = obj_mem_default;
	res->bufsize = 0;
	res->size = 0;
	res->p = NULL;

	return	res;
}

void *	new_mem_cuda_device()
{
	obj_mem	res = new_mem();
	res->type = obj_mem_cuda_device;

	return	res;
}

void *	new_mem_file()
{
	obj_mem	res;
	int	fd;

	res = malloc(sizeof(obj_mem_t));
	res->type = obj_file_local;
	res->size = res->bufsize = strlen(PATH_TEMP) + strlen("/ddq.XXXXXX") + 1;
	res->p = malloc(res->bufsize);
	strcpy(res->p, PATH_TEMP);
	strcat(res->p, "/ddq.XXXXXX");
	fd = mkstemp(res->p);
	if (fd == -1)
		ddq_error("new_mem_file : failed to create temporary file.\n");
	close(fd);

	return	res;
}

void *	new_ptr()
{
	obj_mem	res = new_mem();

	res->type = obj_mem_rawptr;

	return	res;
}

static void	del_mem_file(void *p)
{
	unlink(((obj_mem)p)->p);
}

void	del_mem(void *p)
{
	if (((obj_mem)p)->type == obj_file_local)
		del_mem_file(p);

	free(((obj_mem)p)->p);
	((obj_mem)p)->p = NULL;
	((obj_mem)p)->size = 0;
	((obj_mem)p)->bufsize = 0;
	free(p);
}


void *	new_ring()
{
	return	ddq_new(NULL, 0,0);
}

void	clear_ring(void *p)
{
	// TODO
}

void	del_ring(void *p)
{
	ddq_delete(p);
}



void *	new_ast()
{
	return	ast_new();
}

void	del_ast(void *p)
{
	ast_delete(p);
}


void *	new_dummy()
{
	return	NULL;
}




