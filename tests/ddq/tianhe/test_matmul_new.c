
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
#include "std/std_types/std_types.h"

ddq_ring matmul_ring(obj_mem A, obj_mem B, obj_mem C, int_t* size_array, int_t Ma, int_t Ms,int_t Mg, int_t Kg,int_t Ka,int_t Ng,int_t Na,int_t M, int_t N, int_t K,
               int_t* gen_array_A, int_t* gen_array_B, int_t* gen_array_C, 
               int_t* gen_array_Agsm_1, int_t* gen_array_Agsm_2, int_t* gen_array_Bgsm, int_t* gen_array_Cgsm,
               int_t* params_A2Agsm, int_t* params_B2Bam, int_t* params_C2Cam, int_t* params_gsm2Asm, int_t* params_Cam2C){

    ddq_ring res = ddq_new(NULL, 0, 0);

    obj generator = obj_import(res, load_tianhe("op_generator_row"), NULL, obj_prop_ready);
    obj generator_col = obj_import(res, load_tianhe("op_generator_col"), NULL, obj_prop_ready);
    obj cluster2gsm = obj_import(res, load_tianhe("op_dma_cluster2gsm_Async"), NULL, obj_prop_ready);
    obj cluster2sm = obj_import(res, load_tianhe("op_dma_cluster2sLDM_Async"), NULL, obj_prop_ready);
    obj cluster2am = obj_import(res, load_tianhe("op_dma_cluster2vLDM_Async"), NULL, obj_prop_ready);
    obj gsm2sm = obj_import(res, load_tianhe("op_dma_gsm2sLDM_Async"), NULL, obj_prop_ready);
    obj matmul = obj_import(res, load_tianhe("op_gemm_old"), NULL, obj_prop_ready);
    obj am2cluster = obj_import(res, load_tianhe("op_dma_sLDM2cluster_Async"), NULL, obj_prop_ready);
    obj finish = obj_import(res, load_tianhe("op_finish"), NULL, obj_prop_ready);
    
    obj obj_A = obj_import(res, A, NULL, obj_prop_ready);
    obj obj_B = obj_import(res, B, NULL, obj_prop_ready);
    obj obj_C = obj_import(res, C, NULL, obj_prop_ready);
    obj obj_C_res = obj_import(res, C, NULL, obj_prop_consumable);
    obj obj_Agsm = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable | obj_prop_read_contest | (Mg*Kg/Ms/Ka*Ng/Na));
    obj obj_Asm = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable);
    obj obj_Bam = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable | obj_prop_read_contest | (Mg/Ms));
    obj obj_Cam = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable | obj_prop_read_contest | (Ma/Ms));
    obj obj_Cam_res = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable);

    printf("gen_array_B: len:%ld\n", gen_array_B[0]);
    obj obj_size_array = obj_import(res, size_array, NULL, obj_prop_ready);//TODO
    obj obj_gen_array_A = obj_import(res, gen_array_A, NULL, obj_prop_ready);
    obj obj_gen_array_B = obj_import(res, gen_array_B, NULL, obj_prop_ready);
    obj obj_gen_array_C = obj_import(res, gen_array_C, NULL, obj_prop_ready);
    obj obj_gen_array_Agsm_1 = obj_import(res, gen_array_Agsm_1, NULL, obj_prop_ready);
    obj obj_gen_array_Agsm_2 = obj_import(res, gen_array_Agsm_2, NULL, obj_prop_ready);
    obj obj_gen_array_Bgsm = obj_import(res, gen_array_Bgsm, NULL, obj_prop_ready);
    obj obj_gen_array_Cgsm = obj_import(res, gen_array_Cgsm, NULL, obj_prop_ready);

    obj obj_params_A2Agsm = obj_import(res, params_A2Agsm, NULL, obj_prop_ready);
    obj obj_params_B2Bam = obj_import(res, params_B2Bam, NULL, obj_prop_ready);
    obj obj_params_C2Cam = obj_import(res, params_C2Cam, NULL, obj_prop_ready);
    obj obj_params_gsm2Asm = obj_import(res, params_gsm2Asm, NULL, obj_prop_ready);
    obj obj_params_Cam2C = obj_import(res, params_Cam2C, NULL, obj_prop_ready);

    // obj symbol = obj_import(res, NULL, NULL, obj_prop_consumable | obj_prop_read_contest | (Kg/Ka));

    int_t* iter_start = hthread_malloc(0, sizeof(int_t), HT_MEM_RW);
    *iter_start = 0;
    obj obj_iter_start = obj_import(res, iter_start, NULL, obj_prop_ready | obj_prop_consumable);
    obj obj_iter_start_Agsm = obj_import(res, iter_start, NULL, obj_prop_consumable);

    obj obj_iter_A = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_B = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable | obj_prop_read_contest | (M/Mg));
    obj obj_iter_C = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable | obj_prop_read_contest | (Kg/Ka));
    obj obj_iter_Agsm_1 = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable | obj_prop_read_contest | (Ng/Na));
    obj obj_iter_Agsm_2 = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_Bgsm = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_Cgsm = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);

    ddq_op generator_A = ddq_spawn(res, processor_direct, 2, 2);
    ddq_add_f(generator_A, generator_col);
    ddq_add_inputs(generator_A, 0, obj_gen_array_A);
    ddq_add_inputs(generator_A, 1, obj_iter_start);
    ddq_add_outputs(generator_A, 0, obj_iter_A);
    ddq_add_outputs(generator_A, 1, obj_iter_start_Agsm);

    ddq_op generator_Agsm_1 = ddq_spawn(res, processor_direct, 2, 1);
    ddq_add_f(generator_Agsm_1, generator);
    ddq_add_inputs(generator_Agsm_1, 0, obj_gen_array_Agsm_1);
    ddq_add_inputs(generator_Agsm_1, 1, obj_iter_start_Agsm);
    // ddq_add_inputs(generator_Agsm, 2, symbol);
    ddq_add_outputs(generator_Agsm_1, 0, obj_iter_Agsm_1);

    ddq_op generator_Agsm_2 = ddq_spawn(res, processor_direct, 2, 1);
    ddq_add_f(generator_Agsm_2, generator);
    ddq_add_inputs(generator_Agsm_2, 0, obj_gen_array_Agsm_2);
    ddq_add_inputs(generator_Agsm_2, 1, obj_iter_Agsm_1);
    // ddq_add_inputs(generator_Agsm, 2, symbol);
    ddq_add_outputs(generator_Agsm_2, 0, obj_iter_Agsm_2);

    ddq_op A2Agsm = ddq_spawn(res, processor_direct, 3, 1); //4001012f8
    ddq_add_f(A2Agsm, cluster2gsm);
    ddq_add_inputs(A2Agsm, 0, obj_A);
    ddq_add_inputs(A2Agsm, 1, obj_iter_A);
    ddq_add_inputs(A2Agsm, 2, obj_params_A2Agsm);
    ddq_add_outputs(A2Agsm, 0, obj_Agsm);

    ddq_op Agsm2Asm = ddq_spawn(res, processor_direct, 3, 1); //4001013c8
    ddq_add_f(Agsm2Asm, gsm2sm);
    ddq_add_inputs(Agsm2Asm, 0, obj_Agsm);
    ddq_add_inputs(Agsm2Asm, 1, obj_iter_Agsm_2);
    ddq_add_inputs(Agsm2Asm, 2, obj_params_gsm2Asm);
    ddq_add_outputs(Agsm2Asm, 0, obj_Asm);

    ddq_op generator_B = ddq_spawn(res, processor_direct, 2, 1);
    ddq_add_f(generator_B, generator);
    ddq_add_inputs(generator_B, 0, obj_gen_array_B);
    ddq_add_inputs(generator_B, 1, obj_iter_start);
    ddq_add_outputs(generator_B, 0, obj_iter_B);

    ddq_op generator_Bgsm = ddq_spawn(res, processor_direct, 2, 1);
    ddq_add_f(generator_Bgsm, generator);
    ddq_add_inputs(generator_Bgsm, 0, obj_gen_array_Bgsm);
    ddq_add_inputs(generator_Bgsm, 1, obj_iter_B);
    ddq_add_outputs(generator_Bgsm, 0, obj_iter_Bgsm);

    ddq_op B2Bam = ddq_spawn(res, processor_direct, 3, 1);
    ddq_add_f(B2Bam, cluster2am);
    ddq_add_inputs(B2Bam, 0, obj_B);
    ddq_add_inputs(B2Bam, 1, obj_iter_Bgsm);
    ddq_add_inputs(B2Bam, 2, obj_params_B2Bam);
    ddq_add_outputs(B2Bam, 0, obj_Bam);

    ddq_op generator_C = ddq_spawn(res, processor_direct, 2, 1);
    ddq_add_f(generator_C, generator);
    ddq_add_inputs(generator_C, 0, obj_gen_array_C);
    ddq_add_inputs(generator_C, 1, obj_iter_start);
    ddq_add_outputs(generator_C, 0, obj_iter_C);

    ddq_op generator_Cgsm = ddq_spawn(res, processor_direct, 2, 1);
    ddq_add_f(generator_Cgsm, generator_col);
    ddq_add_inputs(generator_Cgsm, 0, obj_gen_array_Cgsm);
    ddq_add_inputs(generator_Cgsm, 1, obj_iter_C);
    ddq_add_outputs(generator_Cgsm, 0, obj_iter_Cgsm);

    ddq_op C2Cam = ddq_spawn(res, processor_direct, 3, 1);
    ddq_add_f(C2Cam, cluster2am);
    ddq_add_inputs(C2Cam, 0, obj_C);
    ddq_add_inputs(C2Cam, 1, obj_iter_Cgsm);
    ddq_add_inputs(C2Cam, 2, obj_params_C2Cam);
    ddq_add_outputs(C2Cam, 0, obj_Cam);

    ddq_op op_matmul = ddq_spawn(res, processor_direct, 4, 1);
    ddq_add_f(op_matmul, matmul);
    ddq_add_inputs(op_matmul, 0, obj_Asm);
    ddq_add_inputs(op_matmul, 1, obj_Bam);
    ddq_add_inputs(op_matmul, 2, obj_Cam);
    ddq_add_inputs(op_matmul, 3, obj_size_array);
    ddq_add_outputs(op_matmul, 0, obj_Cam_res);

    ddq_op Cam2C = ddq_spawn(res, processor_direct, 3, 1);
    ddq_add_f(Cam2C, am2cluster);
    ddq_add_inputs(Cam2C, 0, obj_Cam_res);
    ddq_add_inputs(Cam2C, 1, obj_iter_Cgsm);
    ddq_add_inputs(Cam2C, 2, obj_params_Cam2C);
    ddq_add_outputs(Cam2C, 0, obj_C_res);

    ddq_op op_finish = ddq_spawn(res, processor_direct, 1, 0);
    ddq_add_f(op_finish, finish);
    ddq_add_inputs(op_finish, 0, obj_C_res);

    return res;
}

ddq_ring cpu_ring(ddq_ring dsp_ring, int core_num){
   //构建算子图
	ddq_ring ring = ddq_new(NULL, 0,0);

	//准备obj
	void* handle_op = load_so_open("device/tianhe/ops");
    void* handle_type = load_so_open("device/tianhe/types");

	obj f_getMTthread = obj_import(ring, load_so_sym(handle_op, "op_getMTthread"), NULL, obj_prop_ready);
	obj f_freeMTthread = obj_import(ring, load_so_sym(handle_op, "op_freeMTthread"), NULL, obj_prop_ready);
   obj core_num_obj = obj_import(ring, core_num, NULL, obj_prop_ready);
	obj thread = obj_new(ring, load_so_sym(handle_type, "new_tianhe_thread"), load_so_sym(handle_type, "del_tianhe_thread"), 0); //TODO:如果consumable停不下来
   obj obj_dsp_finish = obj_import(ring, NULL, NULL, obj_prop_consumable);

	ddq_op getMTthread = ddq_spawn(ring, processor_direct, 1, 1);
	ddq_add_f(getMTthread, f_getMTthread);
	ddq_add_inputs(getMTthread, 0, core_num_obj);
	ddq_add_outputs(getMTthread, 0, thread);
	
    obj_mem mem_dsp_ring = pack_ring(dsp_ring);
	obj_mem mem_dsp_ring_1 = hthread_malloc(0, mem_dsp_ring->size, HT_MEM_RW);
    memcpy(mem_dsp_ring_1, mem_dsp_ring, mem_dsp_ring->size);
    free(mem_dsp_ring);

    int_t tianhe_op_size = sizeof(tianhe_op_t) + 1 * sizeof(obj_mem) + (1 + 1) * sizeof(int_t);
    tianhe_op tianhe_op_ = hthread_malloc(0, tianhe_op_size, HT_MEM_RW);
    tianhe_op_->op_num = 1;
    tianhe_op_->ranges = (char*)tianhe_op_ + sizeof(tianhe_op_t);
    tianhe_op_->ranges[0] = 0;
    tianhe_op_->ranges[1] = 1;
    tianhe_op_->ops = (char*)tianhe_op_->ranges + sizeof(int_t) * (tianhe_op_->op_num + 1);
    tianhe_op_->ops[0] = mem_dsp_ring_1;

    printf("tianhe_op obj_mem:%p %ld\n", tianhe_op_->ops[0], tianhe_op_->ops[0]->size);
	ddq_op op = ddq_spawn(ring, processor_tianhe, 1, 1);
	ddq_add_f(op, obj_import(ring, tianhe_op_, NULL, obj_prop_ready));
	ddq_add_inputs(op, 0, thread);
	ddq_add_outputs(op, 0, obj_dsp_finish);

   ddq_op freeMTthread = ddq_spawn(ring, processor_direct, 2, 0);
	ddq_add_f(freeMTthread, f_freeMTthread);
	ddq_add_inputs(freeMTthread, 0, thread);
   ddq_add_inputs(freeMTthread, 1, obj_dsp_finish);

   return ring;
}

void* mat_gen(int_t row_size, int_t col_size){
   double* res = hthread_malloc(0, sizeof(double) * row_size * col_size, HT_MEM_RW);

    // 生成随机数矩阵
    for(int i = 0; i < row_size; i++) {
        for(int j = 0; j < col_size; j++) {
            res[i*col_size + j] = (double)rand() / RAND_MAX * 10; // 生成0到1之间的随机数
        }
    }

    return res;
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
                omat[i*col_size+j] += imat0[i*reduce_size+k] * imat1[k*col_size+j];
            }
        }
    }
}

int main(){
    // 初始化随机数生成器
    srand(time(NULL));

   clock_t ref_start, ref_end, th_start, th_end;

    load_tianhe_init();

   int Mg = 1512;
   int Ng = 1152;
   int Kg = 512;
   int Ma = 252;
   int Na = 48;
   int Ka = 512;
   int Ms = 6;
   int row_size = Mg;
	int col_size = Ng;
	int reduce_size = Kg;

   obj_mem A = hthread_malloc(0, sizeof(obj_mem_t), HT_MEM_RW);
   obj_mem B = hthread_malloc(0, sizeof(obj_mem_t), HT_MEM_RW);
   obj_mem C = hthread_malloc(0, sizeof(obj_mem_t), HT_MEM_RW);
   A->p = mat_gen(row_size, reduce_size);
   A->type = obj_mem_mt3000_dev_mem;
   A->size = row_size * reduce_size * sizeof(double);
   A->bufsize = row_size * reduce_size * sizeof(double);
   B->p = mat_gen(reduce_size, col_size);
   B->type = obj_mem_mt3000_dev_mem;
   B->size = reduce_size * col_size * sizeof(double);
   B->bufsize = reduce_size * col_size * sizeof(double);
   C->p = mat_gen(row_size, col_size);
   C->type = obj_mem_mt3000_dev_mem;
   C->size = row_size * col_size * sizeof(double);
   C->bufsize = row_size * col_size * sizeof(double);
	// double* imat0 = mat_gen(row_size, reduce_size);
	// double* imat1 = mat_gen(reduce_size, col_size);
	// double* omat = malloc(row_size * col_size * sizeof(double));
	// memset(omat, 0, row_size * col_size * sizeof(double));
	double* omat_ref = malloc(row_size * col_size  * sizeof(double));
	// memset(omat_ref, 0, row_size * col_size * sizeof(double));
    memcpy(omat_ref, C->p, row_size * col_size  * sizeof(double));

	ref_start = clock();
	// matmul_ref(A->p, B->p, omat_ref, row_size, col_size, reduce_size);
	ref_end = clock();

    int_t* size_array = hthread_malloc(0, sizeof(int_t) * 5, HT_MEM_RW);
    size_array[0] = Ms;
    size_array[1] = Na;
    size_array[2] = Ka;
    size_array[3] = Ma/Ms;
    size_array[4] = 0;

    //gen_array的step有误
   int_t* gen_array_A = hthread_malloc(0, sizeof(int_t) * (2 * 3 + 1), HT_MEM_RW);
   gen_array_A[0] = 2;
   gen_array_A[1] = reduce_size/Kg;
   gen_array_A[2] = row_size/Mg;
   gen_array_A[3] = Kg*sizeof(double); //Kg
   gen_array_A[4] = Mg*Kg*sizeof(double); //Mg
   gen_array_A[5] = 0;
   gen_array_A[6] = 0;

   int_t* gen_array_B = hthread_malloc(0, sizeof(int_t) * (2 * 3 + 1), HT_MEM_RW);
   gen_array_B[0] = 2;
   gen_array_B[1] = col_size/Ng;
   gen_array_B[2] = reduce_size/Kg;
   gen_array_B[3] = Ng*sizeof(double); //Ng
   gen_array_B[4] = Kg*Ng*sizeof(double); //Kg
   gen_array_B[5] = 0;
   gen_array_B[6] = 0;

   int_t* gen_array_C = hthread_malloc(0, sizeof(int_t) * (2 * 3 + 1), HT_MEM_RW);
   gen_array_C[0] = 2;
   gen_array_C[1] = col_size/Ng;
   gen_array_C[2] = row_size/Mg;
   gen_array_C[3] = Ng*sizeof(double); //Ng
   gen_array_C[4] = Mg*Ng*sizeof(double); //Mg
   gen_array_C[5] = 0;
   gen_array_C[6] = 0;

   int_t* gen_array_Agsm_1 = hthread_malloc(0, sizeof(int_t) * (2 * 3 + 1), HT_MEM_RW);
   gen_array_Agsm_1[0] = 2;
   gen_array_Agsm_1[1] = Kg/Ka;
   gen_array_Agsm_1[2] = 1;
   gen_array_Agsm_1[3] = Ka*sizeof(double);
   gen_array_Agsm_1[4] = Mg*Ka*sizeof(double); 
   gen_array_Agsm_1[5] = 0;
   gen_array_Agsm_1[6] = 0;

   int_t* gen_array_Agsm_2 = hthread_malloc(0, sizeof(int_t) * (2 * 3 + 1), HT_MEM_RW);
   gen_array_Agsm_2[0] = 2;
   gen_array_Agsm_2[1] = 1;
   gen_array_Agsm_2[2] = Mg/Ms;
   gen_array_Agsm_2[3] = Ka*sizeof(double); //Ka
   gen_array_Agsm_2[4] = Ms*Ka*sizeof(double); //Ms
   gen_array_Agsm_2[5] = 0;
   gen_array_Agsm_2[6] = 0;

   int_t* gen_array_Bgsm = hthread_malloc(0, sizeof(int_t) * (2 * 3 + 1), HT_MEM_RW);
   gen_array_Bgsm[0] = 2;
   gen_array_Bgsm[1] = Ng/Na;
   gen_array_Bgsm[2] = Kg/Ka;
   gen_array_Bgsm[3] = Na*sizeof(double); //Na
   gen_array_Bgsm[4] = Ka*Ng*sizeof(double); //Ka
   gen_array_Bgsm[5] = 0;
   gen_array_Bgsm[6] = 0;
   int_t* gen_array_Cgsm = hthread_malloc(0, sizeof(int_t) * (2 * 3 + 1), HT_MEM_RW);
   gen_array_Cgsm[0] = 2;
   gen_array_Cgsm[1] = Ng/Na;
   gen_array_Cgsm[2] = Mg/Ma;
   gen_array_Cgsm[3] = Na*sizeof(double); //Na
   gen_array_Cgsm[4] = Ma*Ng*sizeof(double); //Ma
   gen_array_Cgsm[5] = 0;
   gen_array_Cgsm[6] = 0;

   int_t* params_A2Agsm = hthread_malloc(0, sizeof(int_t) * 6, HT_MEM_RW);
   params_A2Agsm[0] = Mg;
   params_A2Agsm[1] = Kg*sizeof(double);
   params_A2Agsm[2] = (reduce_size-Kg)*sizeof(double);
   params_A2Agsm[3] = 1;
   params_A2Agsm[4] = Mg*Kg*sizeof(double);
   params_A2Agsm[5] = 0;
   int_t* params_B2Bam = hthread_malloc(0, sizeof(int_t) * 6, HT_MEM_RW);
   params_B2Bam[0] = Ka;
   params_B2Bam[1] = Na*sizeof(double);
   params_B2Bam[2] = (col_size-Na) * sizeof(double);
   params_B2Bam[3] = 1;
   params_B2Bam[4] = Ka * Na * sizeof(double);
   params_B2Bam[5] = 0;
   int_t* params_C2Cam = hthread_malloc(0, sizeof(int_t) * 6, HT_MEM_RW);
   params_C2Cam[0] = Ma;
   params_C2Cam[1] = Na * sizeof(double);
   params_C2Cam[2] = (col_size-Na) * sizeof(double);
   params_C2Cam[3] = 1;
   params_C2Cam[4] = Ma*Na*sizeof(double);
   params_C2Cam[5] = 0;
   int_t* params_gsm2Asm = hthread_malloc(0, sizeof(int_t) * 6, HT_MEM_RW);
   params_gsm2Asm[0] = Ms;
   params_gsm2Asm[1] = Ka * sizeof(double);
   params_gsm2Asm[2] = (Kg-Ka) * sizeof(double);
   params_gsm2Asm[3] = 1;
   params_gsm2Asm[4] = Ms * Ka * sizeof(double);
   params_gsm2Asm[5] = 0;
   int_t* params_Cam2C = hthread_malloc(0, sizeof(int_t) * 6, HT_MEM_RW);
   params_Cam2C[0] = 1;
   params_Cam2C[1] = Ma*Na*sizeof(double);
   params_Cam2C[2] = 0;
   params_Cam2C[3] = Ma;
   params_Cam2C[4] = Na * sizeof(double);
   params_Cam2C[5] = (col_size-Na) * sizeof(double);
   ddq_ring dsp_ring = matmul_ring(A, B, C, size_array,Ma,Ms,Mg, Kg,Ka,Ng,Na, row_size, col_size, reduce_size,
                           gen_array_A, gen_array_B, gen_array_C, 
                           gen_array_Agsm_1, gen_array_Agsm_2, gen_array_Bgsm, gen_array_Cgsm,
                           params_A2Agsm, params_B2Bam, params_C2Cam, params_gsm2Asm, params_Cam2C);
	ddq_ring ring = cpu_ring(dsp_ring, 1);
	ddq_loop_init();
    
	th_start = clock();
	ddq_loop(ring, 0);
	th_end = clock();
	// sleep(20);
    printf("similarity cal ref: %f, res: %f\n", omat_ref[0], ((double*)C->p)[0]);
	double similarity = cosine_similarity(C->p, omat_ref, row_size*col_size);
    if(1 - similarity > 0.0001){
        printf("error!, similarity: %f\n", similarity);
        for(int i=0; i<row_size; i++){
            for(int j=0; j<col_size; j++){
                printf("%d\t%d\t%f\t%f\n", i, j, ((double*)C->p)[i*row_size+j], omat_ref[i*row_size+j]);
            }
        }
    }else{
		printf("success!, similarity: %f\n", similarity);
	}
    load_tianhe_finish();
	printf("Ref runtime : %fs, TH runtime : %fs\n", (double)(ref_end - ref_start) / CLOCKS_PER_SEC,(double)(th_end - th_start) / CLOCKS_PER_SEC );
	printf("finish\n");
	return 0;
}