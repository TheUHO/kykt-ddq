#include	"op_common.h"
#include 	"ddq_plugin.h"
#include 	"ddq.h"

#include	<stdarg.h>

task_ret	op_noop(void **inputs, void **outputs, void **attribute)
{
	return	task_ret_ok;
}

//有3个输入，分别表示
//0.输入的ring
//1.op本身
//2.op_bridge_status
task_ret	op_bridge(void **inputs, void **outputs, void **attribute){
	ddq_op bridgeOp_1 = (ddq_op)(inputs[1]);
	enum_op_bridge_status status = (enum_obj_status)(inputs[2]);
	ddq_op ring_new;
	if(status == op_bridge_status_tounpack){
		// ddq_unpack();
	}else{
		ring_new = (ddq_op)(inputs[0]);
	}
	if(status <= op_bridge_status_toinit){
		obj f_bridge, obj_twinOp, obj_selfOp, obj_status;
		f_bridge = obj_import_with_name("gen_bridge_func", op_bridge, NULL, obj_prop_ready);
		obj_twinOp = obj_import_with_name("gen_twin_op", bridgeOp_1, NULL, obj_prop_ready);
		obj_status = obj_import_with_name("gen_op_status", op_bridge_status_subsequent, NULL, obj_prop_ready);	// FIXME: 这里有个warning?
		ddq_op bridgeOp_2 = ddq_spawn_with_name("gen_sub_bridgeOp", &ring_new, processor_direct, 3, 0);
		obj_selfOp = obj_import_with_name("gen_self_op", bridgeOp_2, NULL, obj_prop_ready);
		bridgeOp_2->f = f_bridge;
		bridgeOp_2->inputs[0] = obj_twinOp;
		bridgeOp_2->inputs[1] = obj_selfOp;
		bridgeOp_2->inputs[2] = obj_status;

		ring_new = bridgeOp_2;

		inputs[0] = (void *)bridgeOp_2;
		inputs[2] = (void *)op_bridge_status_ready;
	}

	ddq_put(ring_new);

	if(status == op_bridge_status_subsequent && bridgeOp_1->next == bridgeOp_1){
		free(bridgeOp_1->f);
		free(bridgeOp_1->inputs[0]);
		free(bridgeOp_1->inputs[1]);
		free(bridgeOp_1->inputs[2]);
		free(bridgeOp_1);
		bridgeOp_1 = NULL;
		return task_ret_done;
	}else{
		return task_ret_again;
	}

}


/*
 * 这个函数有一个输入和一个输出，它们应当指向同一个obj！
 * 这个对象指向如下的int_t数组的中间某位置，这个数组总共是2+n_dims*5个int_t
 * 		n_ind[0]
 * 		n_ind[1]
 * 		...
 * 		n_ind[n_dims-1]
 * 		i_start[0]
 * 		i_start[1]
 * 		...
 * 		i_start[n_dims-1]
 * 		i_step[0]
 * 		i_step[1]
 * 		...
 * 		i_step[n_dims-1]
 * 		index_next[0]
 * 		index_next[1]
 * 		...
 * 		index_next[n_dims-1]
 * 		n_para
 * 		n_dims
 * 	p ->	res[0]
 * 		res[1]
 * 		...
 * 		res[n_dims-1]
 */
task_ret	op_iterator_iota(void **inputs, void **outputs, void **attribute)
{
	// FIXME: 这里应当先检查一下inputs[0] == outputs[0]。

	int_t	*pi = inputs[0];
	int_t	n_dims = pi[-1];
	int_t	i, c, t;

	for (i=0; i<n_dims; i++)
		pi[i] = pi[i-2-n_dims*3] + pi[i-2-n_dims*2] * pi[i-2-n_dims];

	for (i=n_dims-1, c=pi[-2]; c && i>=0; i--)
	{
		t = pi[i-2-n_dims] + c;
		pi[i-2-n_dims] = t % pi[i-2-n_dims*4];
		c = t / pi[i-2-n_dims*4];
	}

	return (c == 0) ? task_ret_ok : task_ret_done;
}

void	iterator_iota_destruct(void *p)
{
	int_t	*pi = p;
	free(pi-2-pi[-1]*4);
}

obj *	objs_iterator_iota(ddq_op *ring, int_t n_para, int_t n_dims, ...)
{
	// FIXME: 这里应当先检查一下n_para和n_dims都大于0。

	va_list	args;
	int_t	i_para, i, c, t;
	int_t	*pi;
	ddq_op	f;
	obj	*res;

	res = malloc(n_para*sizeof(obj));

	for (i_para=0; i_para<n_para; i_para++)
	{
		pi = malloc((2+n_dims*5)*sizeof(int_t));
		pi += 2+n_dims*4;

		pi[-1] = n_dims;
		pi[-2] = n_para;

		va_start(args, n_dims*3);	// FIXME: 这里有个warning?
		for (i=0; i<n_dims; i++)
		{
			pi[i-2-n_dims*4] = va_arg(args, int_t);
			pi[i-2-n_dims*3] = va_arg(args, int_t);
			pi[i-2-n_dims*2] = va_arg(args, int_t);
			pi[i-2-n_dims] = 0;
		}
		va_end (args);

		for (i=n_dims-1, c=i_para; c && i>=0; i--)
		{
			t = pi[i-2-n_dims] + c;
			pi[i-2-n_dims] = t % pi[i-2-n_dims*4];
			c = t / pi[i-2-n_dims*4];
		}

		if (c==0)
		{
			res[i_para] = obj_import(pi, iterator_iota_destruct, obj_prop_consumable | obj_prop_ready);
			if (op_iterator_iota((void **)&pi, (void **)&pi) == task_ret_ok)
			{
				f = ddq_spawn(ring, processor_direct, 1, 1);
				f->f = obj_import(op_iterator_iota, NULL, obj_prop_ready);
				f->inputs[0] = res[i_para];
				f->outputs[0] = res[i_para];
			}
		}
		else			// NOTE: 如果n_para大于总共的需要，那么会在返回值的后面没有用到的部分填充NULL。
		{
			free(pi-2-n_dims*4);
			res[i_para] = NULL;
		}
	}

	return	res;
}









