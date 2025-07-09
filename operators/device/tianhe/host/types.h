#ifndef DEVICE_TIANHE_TYPES_H
#define DEVICE_TIANHE_TYPES_H

#include	"basic_types.h"

#include    "std/std_types/std_types.h"

typedef	struct	tianhe_thread_t	tianhe_thread_t;
typedef	tianhe_thread_t *		tianhe_thread;
struct tianhe_thread_t{
	int cluster_id;
	int thread_id;
	int core_num;
};

typedef	struct	tianhe_op_t	tianhe_op_t;
typedef	tianhe_op_t *		tianhe_op;
struct tianhe_op_t{
	int_t device_num;
	int_t op_num;
	int_t* ranges;
	obj_mem* ops;
};

void* new_tianhe_thread();
void del_tianhe_thread(void *p);

void* new_tianhe_op();
void del_tianhe_op(void *p);

#endif