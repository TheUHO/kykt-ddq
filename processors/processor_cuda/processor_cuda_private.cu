#include	<cuda_runtime.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	"processor_cuda.h"

// #ifdef __cplusplus
// extern "C" {
// #endif

cuStreamPool* stream_pool_init()
{
	cuStreamPool* pool = (cuStreamPool*)malloc(sizeof(cuStreamPool));
	if (!pool)
	{
		return	NULL;
	}

	for (int i = 0; i < MAX_STREAMS; i++)
	{
		pool->streams[i] = NULL;
		pool->status[i] = stream_status_none;
	}
	pool->n_ref = 0;

	return	pool;
}

void stream_pool_destroy(cuStreamPool* pool)
{
	if (pool)
	{
		for (int i = 0; i < MAX_STREAMS; i++)
		{
			if (pool->streams[i] != NULL)
			{
				cudaStreamDestroy((cudaStream_t)pool->streams[i]);
				pool->streams[i] = NULL;
			}
		}
		free(pool);
	}
}

inline int	pick_stream(cuStreamPool* pool)
{
	int	i;

	for (i = 0; i < MAX_STREAMS && pool->status[i] != stream_status_available; i++);
	if (i >= MAX_STREAMS)
	{
		for (i = 0; i < MAX_STREAMS && pool->status[i] != stream_status_none; i++);
		if (i >= MAX_STREAMS)
			return	-1;
	}

	return	i;
}

int stream_pool_submit(cuStreamPool* pool, struct processor_cuda_t* p)
{
	if((p->istream = pick_stream(pool)) == -1)
	{
		return -1;
	}
	if (pool->status[p->istream] == stream_status_none)
		if (cudaStreamCreate((cudaStream_t*)(&pool->streams[p->istream])) != cudaSuccess){
			ddq_warning("processor_cuda : Something is wrong when calling cudaStreamCreate().\n");
			return -1;
		}
	pool->status[p->istream] = stream_status_inuse;
	
	p->ret = ((task_cuda_f *)(p->head.f->p))(p->head.p_inputs, p->head.p_outputs, p->head.p_attributes, pool->streams[p->istream]);

	return 1;
}

int stream_pool_query(cuStreamPool* pool, struct processor_cuda_t* p)
{
	if (cudaStreamQuery((cudaStream_t)pool->streams[p->istream]) != cudaErrorNotReady)
	{
		pool->status[p->istream] = stream_status_available;
		return 1;
	}
	return 0;
}

// #ifdef __cplusplus
// }
// #endif