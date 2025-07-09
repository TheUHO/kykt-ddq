
#include	<stdint.h>

#include	"processor_call.h"
#include    "ddq.h"	

// MSQ:有没有什么办法可以让这里返回done？如果不返回done，init这种执行一次就析构的算子使用defcall就和普通的call行为不一致
task_ret	ddq_run(struct processor_call_t *p){
	ddq_loop(p->ring, p->nstep);
	if(p->ring->ops)
		return task_ret_again;
	return task_ret_ok;
}


