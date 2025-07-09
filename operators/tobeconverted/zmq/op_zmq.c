
#include	<string.h>

#include	<zmq.h>

#include	"error.h"
#include	"op_zmq.h"
#include	"common_objs.h"


task_ret	op_zmq_recv(void **inputs, void **outputs, void **attribute)
{
	zmq_msg_t	message;
	obj_mem		pd;
	int_t		size, s;

	zmq_msg_init(&message);
	size = zmq_msg_recv(&message, inputs[0], 0);
	if (size < 0)
		ddq_error("op_zmq_recv() : zmq_msg_recv() failed.\n");
	if (size != zmq_msg_size(&message))
		ddq_warning("op_zmq_recv() : received size (%ld) is not the same with zmq_msg_size (%ld). Why?\n", size, zmq_msg_size(&message));

	pd = (obj_mem)outputs[0];
	if (pd->bufsize < size)
	{
		if (pd->p)
			free(pd->p);
		pd->p = malloc(size);
		pd->bufsize = size;
	}
	pd->size = size;

	memcpy(pd->p, zmq_msg_data(&message), size);

	s = zmq_msg_close(&message);
	if (s)
		ddq_warning("op_zmq_recv() : Something is wrong when calling zmq_msg_close()");

	return	task_ret_ok;		// FIXME: 是否需要判断连接终止并返回task_ret_done？
}

task_ret	op_zmq_send(void **inputs, void **outputs, void **attribute)
{
	obj_mem	pd;
	int_t	s;

	pd = (obj_mem)inputs[0];
	s = zmq_send(outputs[0], pd->p, pd->size, 0);
	if (s != pd->size)
		ddq_warning("op_zmq_send() : Partially sent (%ld/%ld) bytes.\n", s, pd->size);

	return	task_ret_ok;
}

task_ret	op_zmq_sendrecv(void **inputs, void **outputs, void **attribute)
{
	op_zmq_send(inputs+1, inputs);
	op_zmq_recv(inputs, outputs);

	return	task_ret_ok;
}


void	obj_zmq_del(void *p)
{
	int_t	s;

	s = zmq_close(p);
	if (s)
		ddq_warning("obj_zmq_del() : Something is wrong when calling zmq_close()");
}

