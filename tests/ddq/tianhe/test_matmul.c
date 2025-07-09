
#include	<stdio.h>
#include	<stdlib.h>
#include <time.h>
#include	<unistd.h>
#include	"ddq.h"
#include	"oplib.h"
#include	"hthread_host.h"
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

// ddq_ring fib_ring(int core_num){
// 	//第一层ring，斐波那契实现

// 	obj	f_a, f_p, a, b;
// 	ddq_op	ab, ba, pa, pb;
//     f_a = obj_import(load_tianhe("op_f_add"), NULL, obj_prop_ready);
// 	f_p = obj_import(load_tianhe("op_f_print"), NULL, obj_prop_ready);

// 	unsigned long	*a0, *b0;
// 	a0 = hthread_malloc(0, sizeof(unsigned long)* core_num, HT_MEM_RW);
// 	b0 = hthread_malloc(0, sizeof(unsigned long)* core_num, HT_MEM_RW);
// 	for(int i=0;i<core_num;i++){
// 		a0[i] = 1;
// 		b0[i] = 1;
// 	}

// 	a = obj_import(a0, NULL, obj_prop_consumable | obj_prop_ready);
// 	b = obj_import(b0, NULL, obj_prop_consumable);
// 	// obj construct_v = oplib_get(oplib_tianhe, "tianhe_kernel", "vector_new");
// 	// obj destruct_v = oplib_get(oplib_tianhe, "tianhe_kernel", "vector_free");
// 	// b = obj_new(construct_v->p, destruct_v->p, obj_prop_consumable);

// 	ddq_ring ring = ddq_new(0,0);

// 	ab = ddq_spawn(ring, processor_direct, 1, 1);
// 	ddq_add_f(ab, f_a);
// 	ddq_add_inputs(ab, 0, a);
// 	ddq_add_outputs(ab, 0, b);

// 	ba = ddq_spawn(ring, processor_direct, 1, 1);
// 	ddq_add_f(ba, f_a);
// 	ddq_add_inputs(ba, 0, b);
// 	ddq_add_outputs(ba, 0, a);

// 	pa = ddq_spawn(ring, processor_direct, 1, 0);
// 	ddq_add_f(pa, f_p);
// 	ddq_add_inputs(pa, 0, a);

// 	pb = ddq_spawn(ring, processor_direct, 1, 0);
// 	ddq_add_f(pb, f_p);
// 	ddq_add_inputs(pb, 0, b);

// 	return ring;
// }

void* mat_gen(int_t row_size, int_t col_size){
    // double* res = malloc(sizeof(double) * row_size * col_size);
	double* res = hthread_malloc(0, sizeof(double) * row_size * col_size, HT_MEM_RW);
    // 初始化随机数生成器
    srand(time(NULL));

    // 生成随机数矩阵
    for(int i = 0; i < row_size; i++) {
        for(int j = 0; j < col_size; j++) {
            res[i*col_size + j] = (double)rand() / RAND_MAX * 1000; // 生成0到1之间的随机数
        }
    }

    return res;
}
ddq_ring matmul_ring(int_t row_size, int_t col_size, int_t reduce_size){
    void* imat0 = mat_gen(row_size, reduce_size);
    void* imat1 = mat_gen(col_size, reduce_size);
	// void* res = malloc(sizeof(double) * row_size * col_size);
	void* res = hthread_malloc(0, sizeof(double) * row_size * col_size, HT_MEM_RW);

    ddq_ring ring = ddq_new(NULL, 0,0);
	void* op_matmul = load_tianhe("op_matmul_raw");
	printf("op_matmul:%p\n", op_matmul);
	ddq_op matmul = ddq_spawn(ring, processor_direct, 5, 1);
	ddq_add_f(matmul, obj_import(ring, op_matmul, NULL, obj_prop_ready));
	ddq_add_inputs(matmul, 0, obj_import(ring, row_size, NULL, obj_prop_ready | obj_prop_consumable));
	ddq_add_inputs(matmul, 1, obj_import(ring, col_size, NULL, obj_prop_ready | obj_prop_consumable));
	ddq_add_inputs(matmul, 2, obj_import(ring, reduce_size, NULL, obj_prop_ready | obj_prop_consumable));
	ddq_add_inputs(matmul, 3, obj_import(ring, imat0, NULL, obj_prop_ready | obj_prop_consumable));
	ddq_add_inputs(matmul, 4, obj_import(ring, imat1, NULL, obj_prop_ready | obj_prop_consumable));
	ddq_add_outputs(matmul, 0, obj_import(ring, res, NULL, obj_prop_consumable));

	return ring;
}
int	main()
{
    load_tianhe_init();

    void* handle_op = load_so_open("device/tianhe/ops");
    void* handle_type = load_so_open("device/tianhe/types");

	int core_num = 1;
	// tianhe_thread thread_info = hthread_malloc(0, sizeof(tianhe_thread_t), HT_MEM_RW);


	//构建dsp上算子图
	obj_mem dsp_ring = pack_ring(matmul_ring(32,32,32));
    // ddq_ring unpack_dsp_ring = unpack_ring(dsp_ring);
    printf("dsp_ring:%p %ld %p\n",dsp_ring,dsp_ring->size, dsp_ring->p);

	//构建算子图
	ddq_ring ring = ddq_new(NULL, 0,0);

	//准备obj
	obj dspOp_obj = obj_import(ring, dsp_ring, NULL, obj_prop_ready);

	obj f_getMTthread = obj_import(ring, load_so_sym(handle_op, "op_getMTthread"), NULL, obj_prop_ready);
	obj f_freeMTthread = obj_import(ring, load_so_sym(handle_op, "op_freeMTthread"), NULL, obj_prop_ready);

	// obj thread_info_obj = obj_import( thread_, NULL, obj_prop_consumable);
	// obj clusterId_obj = obj_import(cluster_id, NULL, obj_prop_consumable);
    obj core_num_obj = obj_import(ring, core_num, NULL, obj_prop_ready);
	
	obj obj_fBegin = obj_import(ring, f_begin, NULL, obj_prop_ready);
    obj obj_fEnd = obj_import(ring, f_end, NULL, obj_prop_ready);
    
	obj begin_signal = obj_import(ring, NULL, NULL,  obj_prop_consumable);
    obj end_signal = obj_import(ring, NULL, NULL,  obj_prop_consumable);
	obj obj_dsp_finish = obj_import(ring, NULL, NULL, obj_prop_consumable);

    obj thread = obj_new(ring, load_so_sym(handle_type, "new_tianhe_thread"), load_so_sym(handle_type, "del_tianhe_thread"), obj_prop_consumable);

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
