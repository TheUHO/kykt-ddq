
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"hthread_host.h"
#include	"ddq.h"
#include	"oplib.h"
#include	"mt_hybrid.h"
#include    "device/tianhe/host/types.h"

task_ret	f_begin(void **inputs, void **outputs, void **attribute)
{
	printf("test begin!!!\n");

	return task_ret_done;
}

task_ret	f_end(void **inputs, void **outputs, void **attribute)
{
	printf("test end!!!\n");

	return task_ret_done;
}

ddq_ring fib_ring(int core_num){
	//第一层ring，斐波那契实现

	obj	f_a, f_p, a, b;
	ddq_op	ab, ba, pa, pb;
    f_a = obj_import(load_tianhe("op_f_add"), NULL, obj_prop_ready);
	f_p = obj_import(load_tianhe("op_f_print"), NULL, obj_prop_ready);

	unsigned long	*a0, *b0;
	a0 = hthread_malloc(0, sizeof(unsigned long)* core_num, HT_MEM_RW);
	b0 = hthread_malloc(0, sizeof(unsigned long)* core_num, HT_MEM_RW);
	for(int i=0;i<core_num;i++){
		a0[i] = 1;
		b0[i] = 1;
	}

	a = obj_import(a0, NULL, obj_prop_consumable | obj_prop_ready);
	b = obj_import(b0, NULL, obj_prop_consumable);
	// obj construct_v = oplib_get(oplib_tianhe, "tianhe_kernel", "vector_new");
	// obj destruct_v = oplib_get(oplib_tianhe, "tianhe_kernel", "vector_free");
	// b = obj_new(construct_v->p, destruct_v->p, obj_prop_consumable);

	ddq_ring ring = ddq_new(0,0);

	ab = ddq_spawn(ring, processor_direct, 1, 1);
	ddq_add_f(ab, f_a);
	ddq_add_inputs(ab, 0, a);
	ddq_add_outputs(ab, 0, b);

	ba = ddq_spawn(ring, processor_direct, 1, 1);
	ddq_add_f(ba, f_a);
	ddq_add_inputs(ba, 0, b);
	ddq_add_outputs(ba, 0, a);

	pa = ddq_spawn(ring, processor_direct, 1, 0);
	ddq_add_f(pa, f_p);
	ddq_add_inputs(pa, 0, a);

	pb = ddq_spawn(ring, processor_direct, 1, 0);
	ddq_add_f(pb, f_p);
	ddq_add_inputs(pb, 0, b);

	return ring;
}
int	main()
{
    load_tianhe_init();

    void* handle_op = load_so_open("device/tianhe/ops");
    void* handle_type = load_so_open("device/tianhe/types");

	int core_num = 0;
	int thread_id = 0, cluster_id = 0;
	// core_num = hthread_malloc(0, sizeof(int), HT_MEM_RW);
	// thread_id = hthread_malloc(0, sizeof(int), HT_MEM_RW);
	// cluster_id  = hthread_malloc(0, sizeof(int), HT_MEM_RW);

	core_num = 1;

	//构建dsp上算子图
	obj_mem dsp_ring = pack_ring(fib_ring(core_num));
    // ddq_ring unpack_dsp_ring = unpack_ring(dsp_ring);
    printf("dsp_ring:%p %ld %p\n",dsp_ring,dsp_ring->size, dsp_ring->p);

	//准备obj
	obj dspOp_obj = obj_import(dsp_ring, NULL, obj_prop_ready);

	obj f_getMTthread = obj_import(load_so_sym(handle_op, "op_getMTthread"), NULL, obj_prop_ready);
	obj f_freeMTthread = obj_import(load_so_sym(handle_op, "op_freeMTthread"), NULL, obj_prop_ready);

	obj threadId_obj = obj_import( thread_id, NULL, obj_prop_consumable);
	obj clusterId_obj = obj_import(cluster_id, NULL, obj_prop_consumable);
    obj core_num_obj = obj_import(core_num, NULL, obj_prop_ready);
	
	obj obj_fBegin = obj_import(f_begin, NULL, obj_prop_ready);
    obj obj_fEnd = obj_import(f_end, NULL, obj_prop_ready);
    
	obj begin_signal = obj_import(NULL, NULL,  obj_prop_consumable);
    obj end_signal = obj_import(NULL, NULL,  obj_prop_consumable);
	obj obj_dsp_finish = obj_import(NULL, NULL, obj_prop_consumable);

    obj thread = obj_new(load_so_sym(handle_type, "new_tianhe_thread"), load_so_sym(handle_type, "del_tianhe_thread"), obj_prop_consumable);

	//构建算子图
	ddq_ring ring = ddq_new(0,0);

	ddq_op begin = ddq_spawn(ring, processor_direct, 0, 1);
	ddq_add_f(begin, obj_fBegin);
	ddq_add_outputs(begin, 0, begin_signal);

	ddq_op getMTthread = ddq_spawn(ring, processor_direct, 2, 1);
	ddq_add_f(getMTthread, f_getMTthread);
	ddq_add_inputs(getMTthread, 0, core_num_obj);
	ddq_add_inputs(getMTthread, 1, begin_signal);
	ddq_add_outputs(getMTthread, 0, thread);
	
	ddq_op op = ddq_spawn(ring, processor_tianhe, 1, 1);
	ddq_add_f(op, dspOp_obj);
	ddq_add_inputs(op, 0, thread);
	ddq_add_outputs(op, 0, obj_dsp_finish);

	ddq_op freeMTthread = ddq_spawn(ring, processor_direct, 2, 1);
	ddq_add_f(freeMTthread, f_freeMTthread);
	ddq_add_inputs(freeMTthread, 0, thread);
    ddq_add_inputs(freeMTthread, 1, obj_dsp_finish);
	ddq_add_outputs(freeMTthread, 0, end_signal);

	ddq_op end = ddq_spawn(ring, processor_direct, 1, 0);
	ddq_add_f(end, obj_fEnd);
	ddq_add_inputs(end, 0, end_signal);

	ddq_loop_init();
	
	ddq_loop(ring, 0);

    load_tianhe_finish();

	return 0;
}
