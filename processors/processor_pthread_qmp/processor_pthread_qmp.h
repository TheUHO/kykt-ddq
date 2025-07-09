
#include	<pthread.h>

#include	<string.h>		// for memcpy()

#include	<mpi.h>
#include	"QMP_P_COMMON.h"
#include	"qmp.h"

#include	"task_types.h"
#include	"../processor_pthread/processor_pthread_public.h"

#define	N_MPI_COMMS	100


extern	int	processor_pthread_qmp_initialized;


struct processor_pthread_qmp_share
{
	ddq_resource_pthread	comms;
};


struct processor_pthread_qmp_t
{
	ddq_op_t	head;		// C99 6.7.2.1.13 : head的地址和整个结构体的地址相同 ，因此可以强制转换指针类型

	struct processor_pthread_qmp_share *	share;

	task_ret	ret;
	flag_t		is_done;
	pthread_t	pt;
};

void *	run_pthread_qmp(void *args);

