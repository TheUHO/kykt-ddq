
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

const int core_num = 1;
const int Mg = 1512;//1512
const int Ng = 1152;//1152
const int Kg = 512;//512
const int Ma = 252;//252
const int Na = 48;
const int Ka = 512;
const int Ms = 6;
const int row_size = Mg;
const int col_size = Ng * core_num;
const int reduce_size = Kg;

// //DSP func
// void* obj_Agsm_retention_count_new(){
//     int_t ptr = long_new();
//     *ptr = Mg*Kg/Ms/Ka*Ng/Na;
//     return ptr;
// }

// void* obj_Bam_retention_count_new(){
//     int_t ptr = long_new();
//     *ptr = Mg/Ms;
//     return ptr;
// }

// void* obj_Cam_retention_count_new(){
//     int_t ptr = long_new();
//     *ptr = Ma/Ms;
//     return ptr;
// }

// void* obj_iter_start_C_retention_count_new(){
//     int_t ptr = long_new();
//     *ptr = reduce_size/Kg;
//     return ptr;
// }

// void* obj_iter_B_retention_count_new(){
//     int_t ptr = long_new();
//     *ptr = M/Mg;
//     return ptr;
// }

// void* obj_iter_Agsm_1_retention_count_new(){
//     int_t ptr = long_new();
//     *ptr = Ng/Na;
//     return ptr;
// }

ddq_ring matmul_ring(obj_mem A, obj_mem B, obj_mem C, int_t row_size, int_t col_size,int_t reduce_size, int_t Ma, int_t Ms,int_t Mg, int_t Kg,int_t Ka,int_t Ng,int_t Na,int_t M, int_t N, int_t K,
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
    obj nothing = obj_import(res, load_tianhe("op_nothing"), NULL, obj_prop_ready);
    obj shift_with_thread_id = obj_import(res, load_tianhe("op_shift_with_thread_id"), NULL, obj_prop_ready);
    obj initial = obj_import(res, load_tianhe("op_my_initial"), NULL, obj_prop_ready);
    obj remain_input = obj_import(res, load_tianhe("op_input_retention"), NULL, obj_prop_ready);
    obj remain_output = obj_import(res, load_tianhe("op_output_retention"), NULL, obj_prop_ready);

    ddq_log0("op_gemm_old : %p\n", load_tianhe("op_gemm_old"));
    ddq_log0("op_input_retention : %p\n", load_tianhe("op_input_retention"));
    
    obj obj_A = obj_import(res, A, NULL, obj_prop_ready);
    obj obj_B = obj_import(res, B, NULL, obj_prop_ready);
    obj obj_C = obj_import(res, C, NULL, obj_prop_ready);
    obj obj_C_res = obj_import(res, C, NULL, obj_prop_consumable);
    
    obj obj_Agsm = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable);
    obj obj_Agsm_copy = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable);
    obj obj_Asm = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable);
    obj obj_Bam = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable);
    obj obj_Bam_copy = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable);
    obj obj_Cam = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable);
    obj obj_Cam_copy = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable);
    obj obj_Cam_res = obj_new(res, load_tianhe("obj_mem_new"), load_tianhe("obj_mem_delete"), obj_prop_consumable);

    obj obj_size_array = obj_new(res, load_tianhe("raw_new"), NULL, 0);//TODO
    obj obj_gen_array_A = obj_new(res, load_tianhe("raw_new"), NULL, 0);
    obj obj_gen_array_B = obj_new(res, load_tianhe("raw_new"), NULL, 0);
    obj obj_gen_array_C = obj_new(res, load_tianhe("raw_new"), NULL, 0);
    obj obj_gen_array_Agsm_1 = obj_new(res, load_tianhe("raw_new"), NULL, 0);
    obj obj_gen_array_Agsm_2 = obj_new(res, load_tianhe("raw_new"), NULL, 0);
    obj obj_gen_array_Bgsm = obj_new(res, load_tianhe("raw_new"), NULL, 0);
    obj obj_gen_array_Cgsm = obj_new(res, load_tianhe("raw_new"), NULL, 0);

    obj obj_params_A2Agsm = obj_new(res, load_tianhe("raw_new"), NULL, 0);
    obj obj_params_B2Bam = obj_new(res, load_tianhe("raw_new"), NULL, 0);
    obj obj_params_C2Cam = obj_new(res, load_tianhe("raw_new"), NULL, 0);
    obj obj_params_gsm2Asm = obj_new(res, load_tianhe("raw_new"), NULL, 0);
    obj obj_params_Cam2C = obj_new(res, load_tianhe("raw_new"), NULL, 0);

    obj obj_iter_start = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_start_Agsm = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_start_B = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_start_C = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_start_C_copy = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);

    obj obj_shift_B = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), 0);
    obj obj_shift_C = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), 0);

    obj obj_iter_A = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_B = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_B_copy = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_C = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_Agsm_1 = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_Agsm_1_copy = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_Agsm_2 = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_Bgsm = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    obj obj_iter_Cgsm = obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), obj_prop_consumable);
    
    int_t* initial_params = hthread_malloc(0, sizeof(int_t) * 10, HT_MEM_RW);
    initial_params[0] = row_size;
    initial_params[1] = col_size;
    initial_params[2] = reduce_size;
    initial_params[3] = Mg;
    initial_params[4] = Ng;
    initial_params[5] = Kg;
    initial_params[6] = Ma;
    initial_params[7] = Na;
    initial_params[8] = Ka;
    initial_params[9] = Ms;
    obj obj_initial_params = obj_import(res, initial_params, NULL, obj_prop_ready);

    ddq_op op_initial = ddq_spawn(res, processor_direct, 1, 16);
    ddq_add_f(op_initial, initial);
    ddq_add_inputs(op_initial, 0, obj_initial_params);
    ddq_add_outputs(op_initial, 0, obj_params_A2Agsm);
    ddq_add_outputs(op_initial, 1, obj_params_B2Bam);
    ddq_add_outputs(op_initial, 2, obj_params_C2Cam);
    ddq_add_outputs(op_initial, 3, obj_params_gsm2Asm);
    ddq_add_outputs(op_initial, 4, obj_params_Cam2C);
    ddq_add_outputs(op_initial, 5, obj_size_array);
    ddq_add_outputs(op_initial, 6, obj_gen_array_A);
    ddq_add_outputs(op_initial, 7, obj_gen_array_B);
    ddq_add_outputs(op_initial, 8, obj_gen_array_C);
    ddq_add_outputs(op_initial, 9, obj_gen_array_Agsm_1);
    ddq_add_outputs(op_initial, 10, obj_gen_array_Agsm_2);
    ddq_add_outputs(op_initial, 11, obj_gen_array_Bgsm);
    ddq_add_outputs(op_initial, 12, obj_gen_array_Cgsm);
    ddq_add_outputs(op_initial, 13, obj_iter_start);
    ddq_add_outputs(op_initial, 14, obj_shift_B);
    ddq_add_outputs(op_initial, 15, obj_shift_C);
    
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

    ddq_op remain_iter_Agsm_1 = ddq_spawn(res, processor_direct, 3, 1);
    ddq_add_f(remain_iter_Agsm_1, remain_input);
    ddq_add_inputs(remain_iter_Agsm_1, 0, obj_iter_Agsm_1);
    ddq_add_inputs(remain_iter_Agsm_1, 1, obj_new(res, load_tianhe("obj_iter_Agsm_1_retention_count_new"), load_tianhe("raw_delete"), 0));
    ddq_add_inputs(remain_iter_Agsm_1, 2, obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), 0));
    ddq_add_outputs(remain_iter_Agsm_1, 0, obj_iter_Agsm_1_copy);

    ddq_op generator_Agsm_2 = ddq_spawn(res, processor_direct, 2, 1); //0x400101560
    ddq_add_f(generator_Agsm_2, generator_col);
    ddq_add_inputs(generator_Agsm_2, 0, obj_gen_array_Agsm_2); //0x400100ca8
    ddq_add_inputs(generator_Agsm_2, 1, obj_iter_Agsm_1_copy); //0x400101280
    // ddq_add_inputs(generator_Agsm, 2, symbol);
    ddq_add_outputs(generator_Agsm_2, 0, obj_iter_Agsm_2); //0x4001012d8

    ddq_op A2Agsm = ddq_spawn(res, processor_direct, 3, 1); //
    ddq_add_f(A2Agsm, cluster2gsm);
    ddq_add_inputs(A2Agsm, 0, obj_A);
    ddq_add_inputs(A2Agsm, 1, obj_iter_A);
    ddq_add_inputs(A2Agsm, 2, obj_params_A2Agsm);
    ddq_add_outputs(A2Agsm, 0, obj_Agsm);

    ddq_op remainAgsm = ddq_spawn(res, processor_direct, 3, 1);
    ddq_add_f(remainAgsm, remain_input);
    ddq_add_inputs(remainAgsm, 0, obj_Agsm);
    ddq_add_inputs(remainAgsm, 1, obj_new(res, load_tianhe("obj_Agsm_retention_count_new"), load_tianhe("raw_delete"), 0));
    ddq_add_inputs(remainAgsm, 2, obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), 0));
    ddq_add_outputs(remainAgsm, 0, obj_Agsm_copy);

    ddq_op Agsm2Asm = ddq_spawn(res, processor_direct, 3, 1); //0x4001016e8 //BUG
    ddq_add_f(Agsm2Asm, gsm2sm);
    ddq_add_inputs(Agsm2Asm, 0, obj_Agsm_copy); //0x400100938 0x400102058
    ddq_add_inputs(Agsm2Asm, 1, obj_iter_Agsm_2); //0x4001012d8 0x4001020c8
    ddq_add_inputs(Agsm2Asm, 2, obj_params_gsm2Asm); //0x400100eb8 0x90db66000
    ddq_add_outputs(Agsm2Asm, 0, obj_Asm); //0x400100990 0x400102140

    ddq_op op_shift_B = ddq_spawn(res, processor_direct, 1, 1);
    ddq_add_f(op_shift_B, shift_with_thread_id);
    ddq_add_inputs(op_shift_B, 0, obj_shift_B);
    ddq_add_outputs(op_shift_B, 0, obj_iter_start_B);

    ddq_op generator_B = ddq_spawn(res, processor_direct, 2, 1);
    ddq_add_f(generator_B, generator);
    ddq_add_inputs(generator_B, 0, obj_gen_array_B);
    ddq_add_inputs(generator_B, 1, obj_iter_start_B);
    ddq_add_outputs(generator_B, 0, obj_iter_B);

    ddq_op remain_iter_B = ddq_spawn(res, processor_direct, 3, 1);
    ddq_add_f(remain_iter_B, remain_input);
    ddq_add_inputs(remain_iter_B, 0, obj_iter_B);
    ddq_add_inputs(remain_iter_B, 1, obj_new(res, load_tianhe("obj_iter_B_retention_count_new"), load_tianhe("raw_delete"), 0));
    ddq_add_inputs(remain_iter_B, 2, obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), 0));
    ddq_add_outputs(remain_iter_B, 0, obj_iter_B_copy);

    ddq_op generator_Bgsm = ddq_spawn(res, processor_direct, 2, 1); //0x400101918
    ddq_add_f(generator_Bgsm, generator);
    ddq_add_inputs(generator_Bgsm, 0, obj_gen_array_Bgsm); //0x400100d00
    ddq_add_inputs(generator_Bgsm, 1, obj_iter_B_copy); //0x4001011d0
    ddq_add_outputs(generator_Bgsm, 0, obj_iter_Bgsm); //0x400101330

    ddq_op B2Bam = ddq_spawn(res, processor_direct, 3, 1); //0x4001019d0
    ddq_add_f(B2Bam, cluster2am);
    ddq_add_inputs(B2Bam, 0, obj_B); //0x400100830
    ddq_add_inputs(B2Bam, 1, obj_iter_Bgsm); //0x400101330
    ddq_add_inputs(B2Bam, 2, obj_params_B2Bam); //0x400100e08
    ddq_add_outputs(B2Bam, 0, obj_Bam); //0x4001009e8

    ddq_op remainBam = ddq_spawn(res, processor_direct, 3, 1);
    ddq_add_f(remainBam, remain_input);
    ddq_add_inputs(remainBam, 0, obj_Bam);
    ddq_add_inputs(remainBam, 1, obj_new(res, load_tianhe("obj_Bam_retention_count_new"), load_tianhe("raw_delete"), 0));
    ddq_add_inputs(remainBam, 2, obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), 0));
    ddq_add_outputs(remainBam, 0, obj_Bam_copy);

    ddq_op op_shift_C = ddq_spawn(res, processor_direct, 1, 1);
    ddq_add_f(op_shift_C, shift_with_thread_id);
    ddq_add_inputs(op_shift_C, 0, obj_shift_C);
    ddq_add_outputs(op_shift_C, 0, obj_iter_start_C);

    ddq_op remain_iter_start_C = ddq_spawn(res, processor_direct, 3, 1);
    ddq_add_f(remain_iter_start_C, remain_input);
    ddq_add_inputs(remain_iter_start_C, 0, obj_iter_start_C);
    ddq_add_inputs(remain_iter_start_C, 1, obj_new(res, load_tianhe("obj_iter_start_C_retention_count_new"), load_tianhe("raw_delete"), 0));
    ddq_add_inputs(remain_iter_start_C, 2, obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), 0));
    ddq_add_outputs(remain_iter_start_C, 0, obj_iter_start_C_copy);


    ddq_op generator_C = ddq_spawn(res, processor_direct, 2, 1);
    ddq_add_f(generator_C, generator);
    ddq_add_inputs(generator_C, 0, obj_gen_array_C);
    ddq_add_inputs(generator_C, 1, obj_iter_start_C_copy);
    ddq_add_outputs(generator_C, 0, obj_iter_C);

    ddq_op generator_Cgsm = ddq_spawn(res, processor_direct, 2, 1); //0x400101c00
    ddq_add_f(generator_Cgsm, generator_col);
    ddq_add_inputs(generator_Cgsm, 0, obj_gen_array_Cgsm);  //0x400100d58
    ddq_add_inputs(generator_Cgsm, 1, obj_iter_C); //0x400101228
    ddq_add_outputs(generator_Cgsm, 0, obj_iter_Cgsm);

    ddq_op C2Cam = ddq_spawn(res, processor_direct, 3, 1);//0x400101cb8
    ddq_add_f(C2Cam, cluster2am);
    ddq_add_inputs(C2Cam, 0, obj_C); //0x400100888
    ddq_add_inputs(C2Cam, 1, obj_iter_Cgsm); //0x400101388
    ddq_add_inputs(C2Cam, 2, obj_params_C2Cam); //0x400100e60
    ddq_add_outputs(C2Cam, 0, obj_Cam);

    ddq_op remainCam = ddq_spawn(res, processor_direct, 3, 1);
    ddq_add_f(remainCam, remain_input);
    ddq_add_inputs(remainCam, 0, obj_Cam);
    ddq_add_inputs(remainCam, 1, obj_new(res, load_tianhe("obj_Cam_retention_count_new"), load_tianhe("raw_delete"), 0));
    ddq_add_inputs(remainCam, 2, obj_new(res, load_tianhe("long_new"), load_tianhe("raw_delete"), 0));
    ddq_add_outputs(remainCam, 0, obj_Cam_copy);

    ddq_op op_matmul = ddq_spawn(res, processor_direct, 4, 1); //0x400101d88
    ddq_add_f(op_matmul, matmul);
    ddq_add_inputs(op_matmul, 0, obj_Asm); //0x400100990 0x400102140
    ddq_add_inputs(op_matmul, 1, obj_Bam_copy); //0x4001009e8
    ddq_add_inputs(op_matmul, 2, obj_Cam_copy); //0x400100a40
    ddq_add_inputs(op_matmul, 3, obj_size_array); //0x400100af0
    ddq_add_outputs(op_matmul, 0, obj_Cam_res); //0x400100a98

    ddq_op Cam2C = ddq_spawn(res, processor_direct, 3, 1); //0x400101e68
    ddq_add_f(Cam2C, am2cluster);
    ddq_add_inputs(Cam2C, 0, obj_Cam_res);
    ddq_add_inputs(Cam2C, 1, obj_iter_Cgsm);
    ddq_add_inputs(Cam2C, 2, obj_params_Cam2C);
    ddq_add_outputs(Cam2C, 0, obj_C_res);

    ddq_op op_nothing = ddq_spawn(res, processor_direct, 1, 0); //0x400101f38
    ddq_add_f(op_nothing, nothing);
    ddq_add_inputs(op_nothing, 0, obj_C_res); //0x4001008e0 0x900002000

    ddq_update(res);

    return res;
}

ddq_ring cpu_ring(tianhe_op tianhe_op_, int core_num){
   //构建算子图
	ddq_ring ring = ddq_new(NULL, 0,0);

	//准备obj
	void* handle_op = load_so_open("device/tianhe/host/ops");
    void* handle_type = load_so_open("device/tianhe/host/types");

	obj f_getMTthread = obj_import(ring, load_so_sym(handle_op, "op_getMTthread"), NULL, obj_prop_ready);
	obj f_freeMTthread = obj_import(ring, load_so_sym(handle_op, "op_freeMTthread"), NULL, obj_prop_ready);
   obj core_num_obj = obj_import(ring, core_num, NULL, obj_prop_ready);
	obj thread = obj_new(ring, load_so_sym(handle_type, "new_tianhe_thread"), load_so_sym(handle_type, "del_tianhe_thread"), 0); //TODO:如果consumable停不下来
   obj obj_dsp_finish = obj_import(ring, NULL, NULL, obj_prop_consumable);

	ddq_op getMTthread = ddq_spawn(ring, processor_direct, 1, 1);
	ddq_add_f(getMTthread, f_getMTthread);
	ddq_add_inputs(getMTthread, 0, core_num_obj);
	ddq_add_outputs(getMTthread, 0, thread);

	ddq_op op = ddq_spawn(ring, processor_tianhe, 1, 1);
	ddq_add_f(op, obj_import(ring, tianhe_op_, NULL, obj_prop_ready));
	ddq_add_inputs(op, 0, thread);
	ddq_add_outputs(op, 0, obj_dsp_finish);

   ddq_op freeMTthread = ddq_spawn(ring, processor_direct, 2, 0);
	ddq_add_f(freeMTthread, f_freeMTthread);
	ddq_add_inputs(freeMTthread, 0, thread);
   ddq_add_inputs(freeMTthread, 1, obj_dsp_finish);

   ddq_update(ring);

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

    printf("A->p %p, B->p %p, C->p %p\n", A->p, B->p, C->p);


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
    
    int_t tianhe_op_size = sizeof(tianhe_op_t) + core_num * sizeof(obj_mem) + (core_num + 1) * sizeof(int_t);
    tianhe_op tianhe_op_ = hthread_malloc(0, tianhe_op_size, HT_MEM_RW);
    tianhe_op_->op_num = core_num;
    tianhe_op_->ranges = (char*)tianhe_op_ + sizeof(tianhe_op_t);
    tianhe_op_->ops = (char*)tianhe_op_->ranges + sizeof(int_t) * (tianhe_op_->op_num + 1);

    for(int i=0; i<core_num; i++){
        tianhe_op_->ranges[i] = i;

        ddq_ring dsp_ring = matmul_ring(A, B, C,row_size, col_size,reduce_size, Ma,Ms,Mg, Kg,Ka,Ng,Na, row_size, col_size, reduce_size,
                           params_A2Agsm, params_B2Bam, params_C2Cam, params_gsm2Asm, params_Cam2C);
        obj_mem mem_dsp_ring = pack_ring(dsp_ring);
        obj_mem mem_dsp_ring_1 = hthread_malloc(0, mem_dsp_ring->size, HT_MEM_RW);
        memcpy(mem_dsp_ring_1, mem_dsp_ring, mem_dsp_ring->size);
        free(mem_dsp_ring);
        tianhe_op_->ops[i] = mem_dsp_ring_1;
    }
    tianhe_op_->ranges[core_num] = core_num;

    // printf("tianhe_op obj_mem:%p %ld\n", tianhe_op_->ops[0], tianhe_op_->ops[0]->size);

	ddq_ring ring = cpu_ring(tianhe_op_, core_num);
	ddq_loop_init();
    
	th_start = clock();
	ddq_loop(ring, 0);
	th_end = clock();
	// sleep(20);
    printf("similarity cal ref: %f, res: %f\n", omat_ref[0], ((double*)C->p)[0]);
	double similarity = cosine_similarity(C->p, omat_ref, row_size*col_size);
    if(1 - similarity > 0.0001){
        printf("error!, similarity: %f\n", similarity);
        // for(int i=0; i<row_size; i++){
        //     for(int j=0; j<col_size; j++){
        //         if(abs(((double*)C->p)[i*row_size+j] - omat_ref[i*row_size+j])>0.0001)
        //         printf("%d\t%d\t%f\t%f\n", i, j, ((double*)C->p)[i*row_size+j], omat_ref[i*row_size+j]);
        //     }
        // }
    }else{
		printf("success!, similarity: %f\n", similarity);
	}
    load_tianhe_finish();
	printf("Ref runtime : %fs, TH runtime : %fs\n", (double)(ref_end - ref_start) / CLOCKS_PER_SEC,(double)(th_end - th_start) / CLOCKS_PER_SEC );
	printf("finish\n");
	return 0;
}