#include	<stdio.h>

#include	<unistd.h>

#include	"ddq_plugin.h"
#include	"processor_pthread_qmp.h"

int	processor_pthread_qmp_initialized = 0;

void *	run_pthread_qmp(void *args)
{
	struct processor_pthread_qmp_t	*p = args;

	while (ddq_resource_pthread_pick(p->share->comms, p->head.metadata) == 0)
		sleep(1);

	p->ret = ((task_f*)p->head.p_f) (p->head.p_inputs, p->head.p_outputs, p->head.p_attribute);
	p->is_done = 1;

	ddq_resource_pthread_return(p->share->comms);

	return	NULL;
}

QMP_comm_t	QMP_comm_get_default(void)
{
	ENTER;
	LEAVE;

	if (processor_pthread_qmp_initialized)
		return	ddq_resource_pthread_get("qmp");
	else
		return	QMP_comm_get_default_origin();
}


