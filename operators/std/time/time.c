
#include	<unistd.h>

#include	"task_types.h"


task_ret	sleeps(void **inputs, void **outputs, void **attribute)
{
	sleep(*(int *)(inputs[0]));
	return	task_ret_ok;
}


