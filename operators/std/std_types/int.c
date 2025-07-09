
#include	<stdio.h>

#include	"task_types.h"


task_ret	init(void **inputs, void **outputs, void **attribute)
{
	*(int *)(outputs[0]) = *(int *)(inputs[0]);
	return	task_ret_done;
}


task_ret	add(void **inputs, void **outputs, void **attribute)
{
	*(int *)(outputs[0]) = *(int *)(inputs[0]) + *(int *)(inputs[1]);
	return	task_ret_ok;
}

task_ret	sub(void **inputs, void **outputs, void **attribute)
{
	*(int *)(outputs[0]) = *(int *)(inputs[0]) - *(int *)(inputs[1]);
	return	task_ret_ok;
}

task_ret	mul(void **inputs, void **outputs, void **attribute)
{
	*(int *)(outputs[0]) = *(int *)(inputs[0]) * *(int *)(inputs[1]);
	return	task_ret_ok;
}

task_ret	div(void **inputs, void **outputs, void **attribute)
{
	*(int *)(outputs[0]) = *(int *)(inputs[0]) / *(int *)(inputs[1]);
	return	task_ret_ok;
}


task_ret	print(void **inputs, void **outputs, void **attribute)
{
	printf("%d\n", *(int *)(inputs[0]));
	fflush(stdout);
	return	task_ret_ok;
}


// 临时弄一些函数用于测试

task_ret	copy(void **inputs, void **outputs, void **attribute)
{
	*(int *)(outputs[0]) = *(int *)(inputs[0]);
	if (*(int *)(inputs[0])>1000)
		return	task_ret_done;
	return	task_ret_ok;
}

task_ret	add2(void **inputs, void **outputs, void **attribute)
{
	*(int *)(outputs[0]) = *(int *)(inputs[0]) + *(int *)(inputs[1]);
	*(int *)(outputs[1]) = *(int *)(outputs[0]) + *(int *)(inputs[1]);
	return	task_ret_ok;
}

task_ret	lessthan(void **inputs, void **outputs, void **attribute)
{
	if (*(int *)(inputs[0]) < *(int *)(inputs[1]))
		return	task_ret_ok;
	else
		return	task_ret_done;
}

