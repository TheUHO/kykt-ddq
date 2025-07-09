
#define __USE_GNU
#include <sched.h>
#include <pthread.h>

#include	"task_types.h"
#define MAX_TASKS 24

struct processor_pthread_pool_t
{
	ddq_op_t	head;		// C99 6.7.2.1.13 : head的地址和整个结构体的地址相同 ，因此可以强制转换指针类型

	task_ret	ret;
	flag_t		is_done;
	pthread_t	pt;
};

typedef struct {
    struct processor_pthread_t* tasks[MAX_TASKS];
    int task_count;
    int remain_task_count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t* threads; // 动态分配线程数组
    int thread_count;   // 动态线程数
    int stop;
	int n_ref;
} ThreadPool;

struct processor_pthread_pool_share
{
	ThreadPool* pool;
};

void *	run_pthread(void *args);
ThreadPool* thread_pool_init();
void thread_pool_destroy(ThreadPool* pool);
int thread_pool_submit(ThreadPool* pool,  struct processor_pthread_pool_t* p);
