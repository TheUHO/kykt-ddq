#ifndef DEVICE_TIANHE_KERNEL_TYPES_H
#define DEVICE_TIANHE_KERNEL_TYPES_H

#include	"basic_types.h"

#include    "std/std_types/std_types.h"

typedef	struct	tianhe_op_node_t	tianhe_op_node_t;
typedef	tianhe_op_node_t *		tianhe_op_node;
struct tianhe_op_node_t{
    int device_num;
    int device_id;
	obj_mem mem;
    tianhe_op_node next;
};

typedef	struct	tianhe_op_queue_t	tianhe_op_queue_t;
typedef	tianhe_op_queue_t *		tianhe_op_queue;
struct tianhe_op_queue_t{
	tianhe_op_node head;
    tianhe_op_node tail;
};

#endif