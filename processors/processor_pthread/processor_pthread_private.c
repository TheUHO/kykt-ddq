#include	"ddq_plugin.h"
#include	"processor_pthread.h"
#include 	"error.h"

pthread_key_t		ddq_pthread_data;
static pthread_once_t	key_once = PTHREAD_ONCE_INIT;

static void	make_key()
{
	pthread_key_create(&ddq_pthread_data, NULL);
}

void *	run_pthread(void *args)
{
	struct processor_pthread_t	*p = args;

	pthread_once(&key_once, make_key);

	p->ret = ((task_f*)p->head.p_f) (p->head.p_inputs, p->head.p_outputs, p->head.p_attributes);
	p->is_done = 1;

	return	NULL;
}

