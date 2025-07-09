
#include	"ddq_plugin.h"


struct processor_call_share
{
	int	nothing;
};


struct processor_call_t
{
	ddq_op_t	head;		// C99 6.7.2.1.13 : head的地址和整个结构体的地址相同 ，因此可以强制转换指针类型

	ddq_ring		ring;
	int		nstep;
	
	task_ret ret;
	char* log;
};

task_ret	ddq_run(struct processor_call_t *p);

