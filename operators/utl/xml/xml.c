
#include	<string.h>
#include	<stdio.h>

#include	"std/std_types/std_types.h"
#include	"std/std_types/str.h"


task_ret	node_str(void **inputs, void **outputs, void **attribute)
// inputs[0]	:	C风格字符串，表示xml tag
// inputs[1]	:	C风格字符串，表示子xml节点
// outputs[0]	:	obj_mem，指向C风格字符串，内容是包装起来的xml节点
{
	obj_mem	po = (obj_mem)outputs[0];
	char *	pt = (char *)inputs[0];
	char *	ps = (char *)inputs[1];
	char *	pl1 = "<";
	char *	pl2 = "</";
	char *	pr = ">";

	obj_realloc(po, (po->p ? strlen(po->p) : 0) + strlen(ps) + strlen(pt)*2 + 5 + 1);
	strcat2obj((void **)&pl1, (void **)&po);		// FIXME: 需要错误处理吗？
	strcat2obj((void **)&pt, (void **)&po);
	strcat2obj((void **)&pr, (void **)&po);
	strcat2obj((void **)&ps, (void **)&po);
	strcat2obj((void **)&pl2, (void **)&po);
	strcat2obj((void **)&pt, (void **)&po);
	strcat2obj((void **)&pr, (void **)&po);

	return	task_ret_ok;
}

task_ret	node_int(void **inputs, void **outputs, void **attribute)
// inputs[0]	:	C风格字符串，表示xml tag
// inputs[1]	:	指向int，表示xml节点的内容
// outputs[0]	:	obj_mem，指向C风格字符串，内容是包装起来的xml节点
{
	char	buf[64];
	char *	pin[2];

	pin[0] = (char *)inputs[0];
	pin[1] = buf;
	sprintf(buf, "%d", *(int *)(inputs[1]));

	return	node_str((void **)pin, outputs);
}

task_ret	node_real(void **inputs, void **outputs, void **attribute)
// inputs[0]	:	C风格字符串，表示xml tag
// inputs[1]	:	指向double，表示xml节点的内容
// outputs[0]	:	obj_mem，指向C风格字符串，内容是包装起来的xml节点
{
	char	buf[64];
	char *	pin[2];

	pin[0] = (char *)inputs[0];
	pin[1] = buf;
	sprintf(buf, "%lf", *(double *)(inputs[1]));

	return	node_str((void **)pin, outputs);
}

task_ret	node(void **inputs, void **outputs, void **attribute)
// inputs[0]	:	C风格字符串，表示xml tag
// inputs[1]	:	obj_mem，指向C风格字符串，表示子xml节点
// outputs[0]	:	obj_mem，指向C风格字符串，内容是包装起来的xml节点
{
	char *	pin[2] = {(char *)inputs[0], ((obj_mem)inputs[1])->p};

	return	node_str((void **)pin, outputs);
}

