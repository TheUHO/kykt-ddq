#ifndef PROCESSOR_TIANHE_SINGLE_H
#define PROCESSOR_TIANHE_SINGLE_H

#include	"task_types.h"
#include	"ddq_plugin.h"
#include    "device/tianhe/host/types.h"
#include    "device/tianhe/kernel/types.h"

#include 	"hthread_host.h"

typedef	enum
{
	thread_status_idle = 0,
	thread_status_busy,
} thread_status_t;

struct processor_tianhe_share
{
	int inited;
    tianhe_op_queue op_queue;
};


struct processor_tianhe_t
{
	ddq_op_t	head;		// C99 6.7.2.1.13 : head的地址和整个结构体的地址相同 ，因此可以强制转换指针类型

	volatile task_ret	*kernel_ret;
	task_ret	ret;

    tianhe_thread thread_info;
	int barrier_id;

	uint64_t *args;
	void* pack_ring;

	void** mems_meta;
};

void* flush_from_memory(void *ptr, uint_t size);

task_ret run_tianhe(struct processor_tianhe_t *p);

void merge_meta_tianhe(meta* dst , struct processor_tianhe_t *p);

task_ret get_ret(struct processor_tianhe_t *p);

typedef void 		tianhe_f_free(uint_t* args);

#endif