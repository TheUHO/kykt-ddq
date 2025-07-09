#ifndef	F_OP_ZMQ_H
#define	F_OP_ZMQ_H

#include	"task_types.h"


/*
 * 对于op_zmq_recv()来说：
 * 	inputs[0]指向zmq端口，类型就是void*
 * 	outputs[0]指向obj_mem_t对象
 *
 * 对于op_zmq_send()来说：
 * 	inputs[0]指向obj_mem_t对象
 * 	outputs[0]指向zmq端口，类型就是void*
 *
 * 对于op_zmq_sendrecv()来说：
 * 	inputs[0]指向zmq端口，类型就是void*。一般是ZMQ_REQ类型的端口，已经发起过connect()。
 * 	inputs[1]指向obj_mem_t对象
 * 	outputs[0]指向obj_mem_t对象
 */
task_ret	op_zmq_recv(void **inputs, void **outputs, void **attribute);
task_ret	op_zmq_send(void **inputs, void **outputs, void **attribute);
task_ret	op_zmq_sendrecv(void **inputs, void **outputs, void **attribute);

void		obj_zmq_del(void *p);

#endif
