#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"basic_types.h"
#include	"task_types.h"

task_ret	op_print(void **inputs, void **outputs, void **attribute)
{
	printf("%ld\t", *(unsigned long *)inputs[0]);
	fflush(stdout);

	return task_ret_ok;
}

task_ret	op_add(void **inputs, void **outputs, void **attribute)
{
	*(unsigned long *)outputs[0] += *(unsigned long *)inputs[0];

	sleep(1);

	if (*(unsigned long *)outputs[0] > 1000)
		return	task_ret_done;
	else
		return	task_ret_ok;
}
