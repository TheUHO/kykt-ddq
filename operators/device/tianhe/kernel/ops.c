#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"hthread_device.h"
#include	"ddq.h"
#include	"task_types.h"
#include "ddq_plugin.h"
#include "error.h"
#include "std/std_types/std_types.h"
#include "device/tianhe/kernel/types.h"
//监听并获取主存图，通过processor_tianhe_share管理
task_ret op_fetchOpGraph(void** inputs, void** outputs){
    tianhe_op_queue op_queue = inputs[0];
    unsigned int lock = inputs[1];
    //Critical
    tianhe_op_node node = NULL;

    if(rwlock_try_wrlock(lock) != 0){
        return task_ret_again;
    }
    if(op_queue->head != op_queue->tail){
        node = op_queue->head;
        op_queue->head = op_queue->head->next;
    }
    rwlock_unlock(lock);

    if(node){
        obj_mem ring_mem = node->mem;
	    obj_mem ring_buf = malloc(ring_mem->size);
        scalar_load(ring_mem, ring_buf, ring_mem->size);
        outputs[0] = ring_buf;
    }else if(!op_queue->tail){
        return task_ret_done;
    }

    return task_ret_ok;
}