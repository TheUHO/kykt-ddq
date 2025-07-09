
#include	<pthread.h>
#include	<mpi.h>

#include	"task_types.h"


struct processor_pthread_mpi_share
{
	int	nothing;
};


struct processor_pthread_mpi_t
{
	ddq_op_t	head;		// C99 6.7.2.1.13 : head的地址和整个结构体的地址相同 ，因此可以强制转换指针类型

	task_ret	ret;
	flag_t		is_done;
	pthread_t	pt;

	MPI_Comm	comm;

};

void *	run_pthread_mpi(void *args);

