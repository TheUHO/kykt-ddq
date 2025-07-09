
#include	<stdio.h>
#include	<string.h>

#include	"error.h"
#include	"std_types.h"


/*
 * 与str类型有关的代码
 */

void	obj_realloc(obj_mem po, int_t size)
{
	void *	pold;

	if (po->bufsize < size)
	{
		pold = po->p;
		po->bufsize = size;
		po->p = malloc(po->bufsize);
		if (po->size > 0)
		{
			memcpy(po->p, pold, po->size);
			free(pold);
		}
	}
}

task_ret	str2obj(void **inputs, void **outputs, void **attribute)
{
	obj_mem	po = (obj_mem)outputs[0];

	if (po->p && po->type != obj_mem_default)
		ddq_warning("obj/str2obj : an unknown pointer is discarded.\n");

	po->size = strlen(inputs[0]) + 1;

	if (po->p && po->bufsize < po->size && po->type == obj_mem_default)
		free(po->p);

	if (!(po->p && po->bufsize >= po->size && po->type == obj_mem_default))
	{
		po->bufsize = po->size;
		po->p = malloc(po->bufsize);
	}

	po->type = obj_mem_default;
	strcpy(po->p, inputs[0]);

	return	task_ret_done;
}

task_ret	strcat2obj(void **inputs, void **outputs, void **attribute)
{
	int_t	size;
	obj_mem	po = (obj_mem)outputs[0];

	if (po->type != obj_mem_default)
		ddq_error("obj/strcat2obj : Wrong memory type.\n");

	if (po->size == 0 || ! po->p)
		return	str2obj(inputs, outputs, attribute);

	if (((char *)(po->p))[po->size-1] != '\0')
	{
		ddq_warning("obj/strcat2obj : Not a C-style string in obj : Truncated.\n");
		((char *)(po->p))[po->size-1] = '\0';
	}

	size = strlen(po->p) + strlen(inputs[0]) + 1;
	obj_realloc(po, size);
	po->size = size;

	strcat(po->p, inputs[0]);

	return	task_ret_ok;
}

task_ret	obj_print(void **inputs, void **outputs, void **attribute)
{
	obj_mem	pi = (obj_mem)inputs[0];

	if (pi->type != obj_mem_default || !pi->p || pi->size == 0)
		ddq_error("obj/print : Something is wrong.\n");

	if (((char *)(pi->p))[pi->size-1] != '\0')
	{
		ddq_warning("obj/print : Not a C-style string in obj : Truncated.\n");
		((char *)(pi->p))[pi->size-1] = '\0';
	}

	printf("%s", (char *)pi->p);
	fflush(stdout);

	return	task_ret_ok;
}

task_ret	obj_cat(void **inputs, void **outputs, void **attribute)
{
	static	void *	empty = "";
	int	i;

	str2obj(&empty, outputs, attribute);
	for (i=1; i<=*(int *)inputs[0] && strcat2obj(&((obj_mem)inputs[i])->p, outputs, attribute) == task_ret_ok; i++);

	if (i==*(int *)inputs[0])
		return	task_ret_ok;
	else
		return	task_ret_error;
}

