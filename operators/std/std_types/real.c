
#include	<stdio.h>
#include	<math.h>

#include	"task_types.h"


task_ret	init(void **inputs, void **outputs, void **attribute)
{
	*(double *)(outputs[0]) = *(double *)(inputs[0]);
	return	task_ret_done;
}


task_ret	add(void **inputs, void **outputs, void **attribute)
{
	*(double *)(outputs[0]) = *(double *)(inputs[0]) + *(double *)(inputs[1]);
	return	task_ret_ok;
}

task_ret	sub(void **inputs, void **outputs, void **attribute)
{
	*(double *)(outputs[0]) = *(double *)(inputs[0]) - *(double *)(inputs[1]);
	return	task_ret_ok;
}

task_ret	mul(void **inputs, void **outputs, void **attribute)
{
	*(double *)(outputs[0]) = *(double *)(inputs[0]) * *(double *)(inputs[1]);
	return	task_ret_ok;
}

task_ret	div(void **inputs, void **outputs, void **attribute)
{
	*(double *)(outputs[0]) = *(double *)(inputs[0]) / *(double *)(inputs[1]);
	return	task_ret_ok;
}


task_ret	rexp(void **inputs, void **outputs, void **attribute)
{
	*(double *)(outputs[0]) = exp(*(double *)(inputs[0]));
	return	task_ret_ok;
}

// TODO: 其他各种计算函数……


task_ret	print(void **inputs, void **outputs, void **attribute)
{
	printf("%.17lg\n", *(double *)(inputs[0]));
	fflush(stdout);
	return	task_ret_ok;
}



