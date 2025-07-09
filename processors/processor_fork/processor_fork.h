
#include	<unistd.h>
#include	<stdlib.h>
#include	<fcntl.h>

#include	"task_types.h"

struct processor_fork_share
{
	uint_t	max_procs, n_procs;
};


struct processor_fork_t
{
	ddq_op_t	head;		// C99 6.7.2.1.13 : head的地址和整个结构体的地址相同 ，因此可以强制转换指针类型

	task_ret	ret;
	int		pipefd[2];
	pid_t		pid;
};

