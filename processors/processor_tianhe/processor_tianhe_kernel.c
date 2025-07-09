#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"hthread_device.h"
#include	"basic_types.h"
#include	"ddq.h"
#include	"task_types.h"
#include "ddq_plugin.h"
#include    "std/std_types/std_types.h"
#include    "device/tianhe/host/types.h"
#include "globals.h"
#include "error.h"

void gsm_initial(){
	int thread_id = get_thread_id();
	signals_gsm[thread_id] = !thread_id;
	ddq_log0("signals_gsm[%d] = %ld\n", thread_id, signals_gsm[thread_id]);
}

obj_mem b_search_ring(tianhe_op ops, int_t thread_id){
	int_t* array = ops->ranges;
	int_t left = 0, right = ops->op_num;
	int_t mid;
	while(left < right){
		mid = (left + right) / 2;
		if(array[mid] <= thread_id && array[mid+1] > thread_id){
			break;
		}
		else if(array[mid] < thread_id){
			left = mid + 1;
		}
		else{
			right = mid;
		}
	}
	return ops->ops[mid];
	
}

__global__ void op_run_kernel(int barrier_id, tianhe_op ops, task_ret *ret, void** mems_meta){
	gsm_initial();
	group_barrier(barrier_id);
	
	unsigned long start, end;
	
	int_t thread_id = get_thread_id();
	hthread_printf("op_run_kernel begin %ld\n", thread_id);
	int_t tianhe_ops_size = sizeof(tianhe_op_t) + ops->op_num * sizeof(obj_mem) + (ops->op_num + 1) * sizeof(int_t);
	tianhe_op ops_buf = malloc(tianhe_ops_size);
	// hthread_printf("op_run_kernel begin %ld %ld\n",ops->op_num,  tianhe_ops_size);
	scalar_load(ops, ops_buf, tianhe_ops_size);
	ops_buf->ranges = (char*)ops_buf + sizeof(tianhe_op_t);
	ops_buf->ops = (char*)ops_buf->ranges + sizeof(int_t) * (ops_buf->op_num + 1);

	obj_mem ring_mem = b_search_ring(ops_buf, thread_id);
	free(ops_buf);
	
	obj_mem ring_buf = malloc(ring_mem->size);
	// hthread_printf("op_run_kernel begin %ld %p %p %ld\n",thread_id, ring_mem, ring_buf, ring_mem->size);
	scalar_load(ring_mem, ring_buf, ring_mem->size);
	// hthread_printf("1\n");
	// hthread_printf("op_run_kernel begin %ld %p %p %ld\n",thread_id, ring_mem, ring_buf, ring_mem->size);
	ddq_ring ring = unpack_ring(ring_buf);

	ddq_loop_init();
	
	start = get_clk();
	ddq_loop(ring, 0);
	end = get_clk();
	double time = (end - start)/1800000.0;

	ddq_log0("kernel time : %d %f ms\n",get_thread_id(), time);
	
	group_barrier(barrier_id);
	// if(ring->n_ops > 0){
	// 	ddq_log0("thread:%d, ring->n_ops:%ld\n", get_thread_id(), ring->n_ops);
	// 	ddq_op op = ring->ops;
	// 	if(get_thread_id() == 13)
	// 		ddq_debug_iter(op);
	// }

	free(ring_buf);

    // meta meta_data = NULL;
    // for(int i = 0; i < ring->n_ops; i++){
    //     meta_merge(&meta_data, ring->metadatas[i]);
    // }

    // obj_mem buf_meta = pack_meta(meta_data);
    // obj_mem mem_meta = hbm_malloc(buf_meta->size);
    // scalar_store(buf_meta, mem_meta, buf_meta->size);
   
    // free(buf_meta);
    // meta_delete(meta_data);

    // mems_meta[thread_id] = (void*)mem_meta;
	
	group_barrier(barrier_id);

	if(thread_id == 0){
		*ret = task_ret_done;
	}
}

// __global__ void op_run_kernel(int barrier_id, unsigned int lock_id, tianhe_op_queue op_queue, task_ret *ret, void** mems_meta){

//     //Critical
//     tianhe_op_node node = NULL;

// 	while(op_queue->tail && op_queue->head){
// 		if(rwlock_try_wrlock(lock) != 0){
// 			continue;
// 		}
// 		if(op_queue->head != op_queue->tail){
// 			node = op_queue->head;
// 			node->device_id++;
// 			if(node->device_id >= node->device_num){
// 				op_queue->head = op_queue->head->next;
// 			}
// 		}
// 		rwlock_unlock(lock);
// 		if(node){
// 			tianhe_op ops = node->tianhe_op_;
// 			int_t tianhe_ops_size = sizeof(tianhe_op_t) + ops->op_num * sizeof(obj_mem) + (ops->op_num + 1) * sizeof(int_t);
// 			tianhe_op ops_buf = malloc(tianhe_ops_size);
// 			scalar_load(ops, ops_buf, tianhe_ops_size);
// 			ops_buf->ranges = (char*)ops_buf + sizeof(tianhe_op_t);
// 			ops_buf->ops = (char*)ops_buf->ranges + sizeof(int_t) * (ops_buf->op_num + 1);

// 			obj_mem ring_mem = b_search_ring(ops_buf, thread_id);
// 			free(ops_buf);

// 			obj_mem ring_buf = malloc(ring_mem->size);
// 			scalar_load(ring_mem, ring_buf, ring_mem->size);
// 			ddq_ring ring = unpack_ring(ring_buf);
			
// 			ddq_loop_init();
	
// 			start = get_clk();
// 			ddq_loop(ring, 0);
// 			end = get_clk();


// 			free(ring_buf);
			
// 		}
// 	}
	
// 	group_barrier(barrier_id);

// 	if(thread_id == 0){
// 		*ret = task_ret_done;
// 	}
// }