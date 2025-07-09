
#include	<stdio.h>
#include	<stdlib.h>
#include <time.h>
#include	<unistd.h>
#include <math.h>
#include <string.h>
#include	"ddq.h"
#include	"oplib.h"
#include	"mt_hybrid.h"
#include    "device/tianhe/host/types.h"
#include	"hthread_host.h"

#define BLOCK_ROW_SIZE 16
#define BLOCK_COL_SIZE 16
#define BLOCK_REDUCE_SIZE 16

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

void* mat_gen(int_t row_size, int_t col_size){
	double* res = malloc(sizeof(double) * row_size * col_size);
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

ddq_ring matmul_ring(int_t* size_array, double* imat0, double* imat1, double* omat, int_t* res_array, int_t res_idx){
	void* ptr_new = load_tianhe("ptr_new");
	void* mat_new = load_tianhe("mat_new");
	void* int_new = load_tianhe("int_new");
	void* long_new = load_tianhe("long_new");
	void* raw_del = load_tianhe("raw_delete");

	ddq_ring ring = ddq_new(NULL, 0,0);
	obj size_array_dup = obj_import(ring, size_array, NULL, obj_prop_ready);
	obj res_array_dup = obj_import(ring, res_array, NULL, obj_prop_ready);
	obj imat0_dup = obj_import(ring, imat0, NULL, obj_prop_ready);
	obj imat1_dup = obj_import(ring, imat1, NULL, obj_prop_ready);
	obj omat_dup = obj_import(ring, omat, NULL, obj_prop_ready);

	obj imat0_blk = obj_new(ring, ptr_new, raw_del, obj_prop_consumable);
	obj imat1_blk = obj_new(ring, ptr_new, raw_del, obj_prop_consumable);
	obj omat_blk = obj_new(ring, ptr_new, raw_del, obj_prop_consumable);
	obj imat0_ldm = obj_new(ring, mat_new, raw_del, obj_prop_consumable);
	obj imat1_ldm = obj_new(ring, mat_new, raw_del, obj_prop_consumable);
	obj omat_ldm = obj_new(ring, mat_new, raw_del, obj_prop_consumable);
	obj dma_chl0 = obj_new(ring, int_new, raw_del, obj_prop_consumable);
	obj dma_chl1 = obj_new(ring, int_new, raw_del, obj_prop_consumable);
	obj dma_symbol0 = obj_new(ring, int_new, raw_del, obj_prop_consumable);
	obj dma_symbol1 = obj_new(ring, int_new, raw_del, obj_prop_consumable);
	
	int_t* res_idx_ptr = hthread_malloc(0, sizeof(int_t), HT_MEM_RW);
	*res_idx_ptr = res_idx;
	obj res_idx_obj = obj_import(ring, res_idx_ptr, NULL, obj_prop_ready);

	obj idx = obj_new(ring, long_new, raw_del, 0);
	ddq_op initial = ddq_spawn(ring, processor_direct, 0, 1);
	ddq_add_f(initial, obj_import(ring, load_tianhe("op_initial"), NULL, obj_prop_ready));
	ddq_add_outputs(initial, 0, idx);

	ddq_op op_iter_to_ptr = ddq_spawn(ring, processor_direct, 7, 3);
	ddq_add_f(op_iter_to_ptr, obj_import(ring, load_tianhe("op_iter_to_ptr"), NULL, obj_prop_ready));
	ddq_add_inputs(op_iter_to_ptr, 0, size_array_dup);
	ddq_add_inputs(op_iter_to_ptr, 1, res_array_dup);
	ddq_add_inputs(op_iter_to_ptr, 2, res_idx_obj);
	//TODO:需要初始化
	ddq_add_inputs(op_iter_to_ptr, 3, idx);
	ddq_add_inputs(op_iter_to_ptr, 4, imat0_dup);
	ddq_add_inputs(op_iter_to_ptr, 5, imat1_dup);
	ddq_add_inputs(op_iter_to_ptr, 6, omat_dup);
	ddq_add_outputs(op_iter_to_ptr, 0, imat0_blk);
	ddq_add_outputs(op_iter_to_ptr, 1, imat1_blk);
	ddq_add_outputs(op_iter_to_ptr, 2, omat_blk);

	ddq_op cluster2sLDM_00 = ddq_spawn(ring, processor_direct, 2, 2);
	ddq_add_f(cluster2sLDM_00, obj_import(ring, load_tianhe("op_cluster2sLDM_Async"), NULL, obj_prop_ready));
	ddq_add_inputs(cluster2sLDM_00, 0, imat0_blk);
	ddq_add_inputs(cluster2sLDM_00, 1, obj_import(ring, BLOCK_ROW_SIZE * BLOCK_REDUCE_SIZE  * sizeof(double), NULL, obj_prop_ready));
	ddq_add_outputs(cluster2sLDM_00, 0, imat0_ldm);
	ddq_add_outputs(cluster2sLDM_00, 1, dma_chl0);

	ddq_op cluster2sLDM_01 = ddq_spawn(ring, processor_direct, 2, 2);
	ddq_add_f(cluster2sLDM_01, obj_import(ring, load_tianhe("op_cluster2sLDM_Async"), NULL, obj_prop_ready));
	ddq_add_inputs(cluster2sLDM_01, 0, imat1_blk);
	ddq_add_inputs(cluster2sLDM_01, 1, obj_import(ring, BLOCK_COL_SIZE * BLOCK_REDUCE_SIZE  * sizeof(double), NULL, obj_prop_ready));
	ddq_add_outputs(cluster2sLDM_01, 0, imat1_ldm);
	ddq_add_outputs(cluster2sLDM_01, 1, dma_chl1);
	// ddq_op cluster2sLDM_10 = ddq_spawn(ring, processor_direct, 2, 2);
	// ddq_add_f(cluster2sLDM_10, obj_import(load_tianhe("op_cluster2sLDM_Async"), NULL, obj_prop_ready));
	// ddq_op cluster2sLDM_11 = ddq_spawn(ring, processor_direct, 2, 2);
	// ddq_add_f(cluster2sLDM_11, obj_import(load_tianhe("op_cluster2sLDM_Async"), NULL, obj_prop_ready));
	
	ddq_op dma_query_00 = ddq_spawn(ring, processor_direct, 1, 1);
	ddq_add_f(dma_query_00, obj_import(ring, load_tianhe("op_dma_query"), NULL, obj_prop_ready));
	ddq_add_inputs(dma_query_00, 0, dma_chl0);
	ddq_add_outputs(dma_query_00, 0, dma_symbol0);
	
	ddq_op dma_query_01 = ddq_spawn(ring, processor_direct, 1, 1);
	ddq_add_f(dma_query_01, obj_import(ring, load_tianhe("op_dma_query"), NULL, obj_prop_ready));
	ddq_add_inputs(dma_query_01, 0, dma_chl1);
	ddq_add_outputs(dma_query_01, 0, dma_symbol1);

	// ddq_op dma_query_10 = ddq_spawn(ring, processor_direct, 1, 1);
	// ddq_add_f(dma_query_10, obj_import(load_tianhe("op_dma_query"), NULL, obj_prop_ready));
	// ddq_op dma_query_11 = ddq_spawn(ring, processor_direct, 1, 1);
	// ddq_add_f(dma_query_11, obj_import(load_tianhe("op_dma_query"), NULL, obj_prop_ready));

	ddq_op matmul = ddq_spawn(ring, processor_direct, 5, 1);
	ddq_add_f(matmul, obj_import(ring, load_tianhe("op_matmul"), NULL, obj_prop_ready));
	ddq_add_inputs(matmul, 0, size_array_dup);
	ddq_add_inputs(matmul, 1, imat0_ldm);
	ddq_add_inputs(matmul, 2, imat1_ldm);
	ddq_add_inputs(matmul, 3, dma_symbol0);
	ddq_add_inputs(matmul, 4, dma_symbol1);
	ddq_add_outputs(matmul, 0, omat_ldm);

	//output的逻辑
	ddq_op op_sLDM2cluster0 = ddq_spawn(ring, processor_direct, 3, 0);
	ddq_add_f(op_sLDM2cluster0, obj_import(ring, load_tianhe("op_sLDM2cluster"), NULL, obj_prop_ready));
	ddq_add_inputs(op_sLDM2cluster0, 0, omat_ldm);
	ddq_add_inputs(op_sLDM2cluster0, 1, obj_import(ring, BLOCK_COL_SIZE * BLOCK_ROW_SIZE * sizeof(double), NULL, obj_prop_ready));
	ddq_add_inputs(op_sLDM2cluster0, 2, omat_blk);
	// ddq_add_outputs(op_sLDM2cluster0, 0, obj_dup(ring, omat_blk)); //TODO 有bug
	// ddq_op op_sLDM2cluster1 = ddq_spawn(ring, processor_direct, 2, 1);

	return ring;
}

void thread_ring(ddq_ring top_ring, obj idx_array_obj, obj finish_iter_idx_obj,  int_t*idx_array, int_t* size_array, double* imat0, double* imat1, double* omat, int_t thread_idx){
	int core_num = 1;
	
	void* handle_op = load_so_open("device/tianhe/ops");
    void* handle_type = load_so_open("device/tianhe/types");

	obj f_getMTthread = obj_import(top_ring, load_so_sym(handle_op, "op_getMTthread"), NULL, obj_prop_ready);
	obj f_freeMTthread = obj_import(top_ring, load_so_sym(handle_op, "op_freeMTthread"), NULL, obj_prop_ready);
    obj core_num_obj = obj_import(top_ring, core_num, NULL, obj_prop_ready);
	obj thread = obj_new(top_ring, load_so_sym(handle_type, "new_tianhe_thread"), load_so_sym(handle_type, "del_tianhe_thread"), 0);
	obj obj_dsp_finish = obj_import(top_ring, NULL, NULL, obj_prop_consumable);

	ddq_op getMTthread = ddq_spawn(top_ring, processor_direct, 1, 1);
	ddq_add_f(getMTthread, f_getMTthread);
	ddq_add_inputs(getMTthread, 0, core_num_obj);
	// ddq_add_inputs(getMTthread, 1, idx_array_obj);
	ddq_add_outputs(getMTthread, 0, thread);
	
	obj_mem dsp_ring = pack_ring(matmul_ring(size_array, imat0, imat1, omat, idx_array, thread_idx));
	ddq_op op = ddq_spawn(top_ring, processor_tianhe, 2, 1);
	ddq_add_f(op, obj_import(top_ring, dsp_ring, NULL, obj_prop_ready));
	ddq_add_inputs(op, 0, thread);
	ddq_add_inputs(op, 1, idx_array_obj);
	// ddq_add_outputs(op, 0, obj_dsp_finish);
	ddq_add_outputs(op, 0, finish_iter_idx_obj);

	// ddq_op freeMTthread = ddq_spawn(top_ring, processor_direct, 2, 1);
	// ddq_add_f(freeMTthread, f_freeMTthread);
	// ddq_add_inputs(freeMTthread, 0, thread);
    // ddq_add_inputs(freeMTthread, 1, obj_dsp_finish);
	// ddq_add_outputs(freeMTthread, 0, finish_iter_idx_obj);
}

ddq_ring cpu_ring(double* imat0, double* imat1, double* omat, int_t row_size, int_t col_size, int_t reduce_size){
    int_t iter_size = 1;
	int thread_size = 24;
	
	double* imat0_blk = hthread_malloc(0, sizeof(double) * row_size * reduce_size, HT_MEM_RW);
	double* imat1_blk = hthread_malloc(0, sizeof(double) * col_size * reduce_size, HT_MEM_RW);
	double* omat_blk = hthread_malloc(0, sizeof(double) * row_size * col_size, HT_MEM_RW);
	int_t* size_array = hthread_malloc(0, sizeof(int_t) * 7, HT_MEM_RW);
	int_t* iter_idx = hthread_malloc(0, sizeof(int_t), HT_MEM_RW);
	int_t* finish_iter_idx = hthread_malloc(0, sizeof(int_t), HT_MEM_RW);
	int_t* idx_array = hthread_malloc(0, sizeof(int_t) * iter_size, HT_MEM_RW);

	*iter_idx = 0;
	*finish_iter_idx = 0;

	size_array[0] = row_size;
	size_array[1] = col_size;
	size_array[2] = reduce_size;
	size_array[3] = BLOCK_ROW_SIZE;
	size_array[4] = BLOCK_COL_SIZE;
	size_array[5] = BLOCK_REDUCE_SIZE;
	size_array[6] = iter_size;

	ddq_ring ring = ddq_new(NULL, 0,0);

	void* handle_tensor_ops = load_so_open("tensor/host/ops");
    obj op_imat_to_blk = obj_import(ring, load_so_sym(handle_tensor_ops, "op_imat_to_blk"), NULL, obj_prop_ready);
    
	obj imat0_obj = obj_import(ring, imat0, NULL, obj_prop_ready);
	obj imat1_obj = obj_import(ring, imat1, NULL, obj_prop_ready);
	obj omat_obj = obj_import(ring, omat, NULL, 0);

	obj imat0_blk_obj = obj_import(ring, imat0_blk, NULL, 0);
	obj imat1_blk_obj = obj_import(ring, imat1_blk, NULL, 0);
	obj omat_blk_obj = obj_import(ring, omat_blk, NULL, obj_prop_ready);

	obj size_array_obj = obj_import(ring, size_array, NULL, obj_prop_ready);
	obj idx_array_obj = obj_import(ring, idx_array, NULL, obj_prop_consumable | obj_prop_read_contest | iter_size);
	printf("idx_array_obj: n_readers:%ld, n_writers:%ld\n", idx_array_obj->n_readers, idx_array_obj->n_writers);
	obj iter_idx_obj = obj_import(ring, iter_idx, NULL, obj_prop_ready);

	obj finish_iter_idx_obj = obj_import(ring, finish_iter_idx, NULL, obj_prop_consumable);
	
	ddq_op imat_to_blk = ddq_spawn(ring, processor_direct, 3, 2);
	ddq_add_f(imat_to_blk, op_imat_to_blk);
	ddq_add_inputs(imat_to_blk, 0, size_array_obj);
	ddq_add_inputs(imat_to_blk, 1, imat0_obj);
	ddq_add_inputs(imat_to_blk, 2, imat1_obj);
	ddq_add_outputs(imat_to_blk, 0, imat0_blk_obj);
	ddq_add_outputs(imat_to_blk, 1, imat1_blk_obj);
	ddq_log0("imat_to_blk:%p\n", imat_to_blk);

	obj op_iter_blk_raw_obj = obj_import(ring, load_so_sym(handle_tensor_ops, "op_iter_blk_raw"), NULL, obj_prop_ready);
	ddq_op iter_blk_raw = ddq_spawn(ring, processor_direct, 2, 1);
	ddq_add_f(iter_blk_raw, op_iter_blk_raw_obj);
	ddq_add_inputs(iter_blk_raw, 0, size_array_obj);
	ddq_add_inputs(iter_blk_raw, 1, iter_idx_obj);
	ddq_add_outputs(iter_blk_raw, 0, idx_array_obj);
	ddq_log0("iter_blk_raw:%p\n", iter_blk_raw);

	obj op_iter_finish_obj = obj_import(ring, load_so_sym(handle_tensor_ops, "op_iter_finish"), NULL, obj_prop_ready);
	ddq_op iter_finish = ddq_spawn(ring, processor_direct, 2, 0);
	ddq_add_f(iter_finish, op_iter_finish_obj);
	ddq_add_inputs(iter_finish, 0, size_array_obj);
	ddq_add_inputs(iter_finish, 1, finish_iter_idx_obj);
	
	for(int i=0; i<thread_size; i++){
			thread_ring(ring, idx_array_obj, finish_iter_idx_obj, idx_array, size_array, imat0_blk, imat1_blk, omat_blk, i);

	}

	obj finish_iter_idx_obj_dup = obj_dup(ring, finish_iter_idx_obj);
	finish_iter_idx_obj_dup->prop = obj_prop_ready;
	finish_iter_idx_obj_dup->status = obj_status_toread;
	ddq_op blk_to_omat = ddq_spawn(ring, processor_direct, 3, 1);
	ddq_add_f(blk_to_omat, obj_import(ring, load_so_sym(handle_tensor_ops, "op_blk_to_omat"), NULL, obj_prop_ready));
	ddq_add_inputs(blk_to_omat, 0, size_array_obj);
	ddq_add_inputs(blk_to_omat, 1, omat_blk_obj);
	ddq_add_inputs(blk_to_omat, 2, finish_iter_idx_obj_dup);
	ddq_add_outputs(blk_to_omat, 0, omat_obj);
	ddq_log0("blk_to_omat:%p, ring->mem->bufsize %ld, ring->mem->size %ld\n", blk_to_omat,ring->mem->bufsize, ring->mem->size);

	return ring;
}

// 计算向量的模长
double vector_magnitude(double *vector, int size) {
    double magnitude = 0;
    for (int i = 0; i < size; i++) {
        magnitude += vector[i] * vector[i];
    }
    return sqrt(magnitude);
}

// 计算两个向量的点积
double dot_product(double *vectorA, double *vectorB, int size) {
    double dot = 0;
    for (int i = 0; i < size; i++) {
        dot += vectorA[i] * vectorB[i];
    }
    return dot;
}

// 计算余弦相似度
double cosine_similarity(double *vectorA, double *vectorB, int size) {
    double dot = dot_product(vectorA, vectorB, size);
    double magnitudeA = vector_magnitude(vectorA, size);
    double magnitudeB = vector_magnitude(vectorB, size);
	printf("magnitudeA %f, magnitudeB %f\n", magnitudeA,magnitudeB );
    return dot / (magnitudeA * magnitudeB);
}

void matmul_ref(double* imat0, double* imat1, double* omat, int_t row_size, int_t col_size, int_t reduce_size){
	int_t i, j, k;
    for(i=0; i<row_size; i++){
        for(j=0; j<col_size; j++){
            for(k=0; k<reduce_size; k++){
                omat[i*col_size+j] += imat0[i*reduce_size+k] * imat1[k*reduce_size+j];
            }
        }
    }
}
int	main()
{
	clock_t ref_start, ref_end, th_start, th_end;

    load_tianhe_init();

    int row_size = 16*16*1;
	int col_size = 16*16*1;
	int reduce_size = 16*16*1;

	double* imat0 = mat_gen(row_size, reduce_size);
	double* imat1 = mat_gen(reduce_size, col_size);
	double* omat = malloc(row_size * col_size * sizeof(double));
	memset(omat, 0, row_size * col_size * sizeof(double));
	double* omat_ref = malloc(row_size * col_size  * sizeof(double));
	memset(omat_ref, 0, row_size * col_size * sizeof(double));

	ref_start = clock();
	matmul_ref(imat0, imat1, omat_ref, row_size, col_size, reduce_size);
	ref_end = clock();

	// printf("1\n");
	ddq_ring ring = cpu_ring(imat0, imat1, omat, row_size, col_size, reduce_size);
	// printf("2\n");
	ddq_loop_init();
	// printf("3\n");
	th_start = clock();
	ddq_loop(ring, 0);
	th_end = clock();
	// printf("4\n");
    load_tianhe_finish();

	// ddq_log0("omat_ref:\n");
    // for(int i=0; i<row_size; i++){
    //     for(int j=0; j<reduce_size; j++){
    //         ddq_log0("%f ", omat_ref[i * reduce_size + j]);
    //     }
    //     ddq_log0("\n");
    // }

	double similarity = cosine_similarity(omat, omat_ref, row_size*col_size);
    if(1 - similarity > 0.001){
        printf("error!, similarity: %f\n", similarity);
    }else{
		printf("success!, similarity: %f\n", similarity);
	}

	printf("Ref runtime : %fs, TH runtime : %fs\n", (double)(ref_end - ref_start) / CLOCKS_PER_SEC,(double)(th_end - th_start) / CLOCKS_PER_SEC );

	// for(int i=0; i< row_size * col_size; i++){
	// 	printf("omat[%d][%d]: %f\n", i/col_size, i%col_size, omat[i]);
	// }
	printf("finish\n");
	return 0;
}
