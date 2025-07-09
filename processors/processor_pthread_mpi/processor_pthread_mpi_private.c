#include	"ddq_plugin.h"
#include	"processor_pthread_mpi.h"

// NOTE: 这个在processor_pthread里定义
extern	pthread_key_t	ddq_pthread_data;

static	pthread_once_t	key_once = PTHREAD_ONCE_INIT;

static void	make_key()
{
	pthread_key_create(&ddq_pthread_data, NULL);
}

void *	run_pthread_mpi(void *args)
{
	struct processor_pthread_mpi_t	*p = args;

	pthread_once(&key_once, make_key);
	pthread_setspecific(ddq_pthread_data, &p->comm);

	p->ret = ((task_f*)p->head.p_f) (p->head.p_inputs, p->head.p_outputs, p->head.p_attribute);
	p->is_done = 1;

	return	NULL;
}

