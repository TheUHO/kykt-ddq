//#include	<stdio.h>

#include	<pthread.h>
#include	<string.h>

#include	"processor_pthread_public.h"


static	ddq_resource_pthread	resource = NULL;


int	ddq_resource_pthread_new(char *name, ddq_resource_pthread *pp, uint_t count)	// FIXME: 相应的析构过程是否需要？
{
	if (*pp)
		return	0;

	*pp = malloc(sizeof(ddq_resource_pthread_t));
	(*pp)->name = name;		// FIXME: 检查名字冲突
	(*pp)->count = count;
	pthread_key_create(&((*pp)->key), NULL);
	(*pp)->next = resource;
	resource = *pp;
	(*pp)->status = malloc(sizeof(ddq_resource_status_e)*count);
	memset((*pp)->status, 0, sizeof(ddq_resource_status_e)*count);
	(*pp)->pdata = malloc(sizeof(void*)*count);
	memset((*pp)->pdata, 0, sizeof(void*)*count);

	return	1;
}

int	ddq_resource_pthread_pick(ddq_resource_pthread p, meta m)
{
	static	pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;

	uint_t	i;
	meta	j;
	uint_t*	buf;

	for (j=m; j && strcmp(p->name, j->name); j=j->next);
	pthread_mutex_lock(&mutex);
	if (j && j->type == meta_type_int)
	{
		i = (j->value.ival % (p->count-1) + p->count-1) % (p->count-1) + 1;
		if (p->status[i] != ddq_resource_status_available)
			return	0;
//		printf("RESOURCE FOUND : %ld.\n", i);fflush(stdout);
	}
	else
	{
		for (i=0; i<p->count && p->status[i]!=ddq_resource_status_available; i++);
		if (i == p->count)
			return	0;
//		printf("RESOURCE PICKED : %ld.\n", i);fflush(stdout);
	}
	p->status[i] = ddq_resource_status_inuse;
	pthread_mutex_unlock(&mutex);

	buf = malloc(sizeof(uint_t));
	*buf = i;
	pthread_setspecific(p->key, buf);

	return	1;
}

void	ddq_resource_pthread_return(ddq_resource_pthread p)
{
	uint_t*	buf;

	buf = (uint_t*)pthread_getspecific(p->key);
	p->status[*buf] = ddq_resource_status_available;
	free(buf);
}

void *	ddq_resource_pthread_get(char *name)
{
	ddq_resource_pthread	i;

	for (i=resource; i && strcmp(i->name, name); i=i->next);
	if (i)
	//{
	//	printf("RESOURCE %s Got : (%ld) %p\n", name, *(uint_t*)pthread_getspecific(i->key), i->pdata[*(uint_t*)pthread_getspecific(i->key)]);fflush(stdout);
		return	i->pdata[*(uint_t*)pthread_getspecific(i->key)];
	//}
	else
		return	NULL;
}



