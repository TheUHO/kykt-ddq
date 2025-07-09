
// #include <cuda.h>

#include	"task_types.h"
#include 	"ddq.h"

#define	MAX_STREAMS	8		// FIXME: 设为32适合Kepler，如果能根据运行时环境动态设置才好


typedef	enum
{
	stream_status_none = 0,
	stream_status_available,
	stream_status_inuse,
} stream_status;

typedef struct{
	void*	streams[MAX_STREAMS];
	stream_status	status[MAX_STREAMS];
	int		n_ref;
} cuStreamPool;

struct processor_cuda_share
{
	cuStreamPool*	pool;
};


struct processor_cuda_t
{
	ddq_op_t	head;		// C99 6.7.2.1.13 : head的地址和整个结构体的地址相同 ，因此可以强制转换指针类型

	task_ret	ret;
	int		istream;
};

#ifdef __cplusplus
extern "C" {
#endif

cuStreamPool* stream_pool_init();
void stream_pool_destroy(cuStreamPool* pool);
int stream_pool_submit(cuStreamPool* pool, struct processor_cuda_t* p);
int stream_pool_query(cuStreamPool* pool, struct processor_cuda_t* p);



#ifdef __cplusplus
}
#endif