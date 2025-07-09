#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"hthread_device.h"
#include	"ddq.h"
#include	"task_types.h"
#include "ddq_plugin.h"
#include "error.h"

//f_print
__global__ task_ret	op_f_print(void **inputs, void **outputs, void **attribute)
{
	int thread_id = get_thread_id();
	unsigned long* l = ACCESS_INPUTS(0, unsigned long*);
	hthread_printf("%ld\t",l[thread_id]);
	fflush(stdout);

	return task_ret_ok;
}

__global__ task_ret	op_f_add(void **inputs, void **outputs, void **attribute)
{
	int thread_id = get_thread_id();
	unsigned long* l0 = ACCESS_INPUTS(0, unsigned long*);
	unsigned long* l1 = ACCESS_OUTPUTS(0, unsigned long*);
	l1[thread_id] += l0[thread_id];

	if (l1[thread_id] > 1000)
		return	task_ret_done;
	else
		return	task_ret_ok;
}