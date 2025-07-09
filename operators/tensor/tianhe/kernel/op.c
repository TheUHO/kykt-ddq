#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"hthread_device.h"
#include	"ddq.h"
#include	"task_types.h"
#include "ddq_plugin.h"
#include "error.h"
#include <compiler/m3000.h>
#include <string.h>

//TODO dup wrong
task_ret op_initial(void** inputs, void** outputs, void** attributes){
    int_t obj_count = *(int_t*)inputs[0];
    int_t size = 0;
    obj_mem obj_mem_;
    for(int_t i=1; i<=obj_count; i++){
        obj_mem_ = inputs[i];
        size += aligned8(obj_mem_->size);
    }
    void* ptr = malloc(size);
    for(int_t i=0; i<obj_count; i++){
        outputs[i] = ptr;
        obj_mem_ = inputs[i+1];
        scalar_load(obj_mem_->p, outputs[i], aligned8(obj_mem_->size));
        ptr += aligned8(obj_mem_->size);
    }
    return task_ret_done;
}
task_ret op_matmul_raw(void** inputs, void** outputs, void** attributes){
    // hthread_printf("op_matmul_raw begin\n");
    int_t row_size, col_size, reduce_size;
    double* imat0, * imat1, * omat;
    row_size = ACCESS_INPUTS(0, int_t);
    col_size = ACCESS_INPUTS(1, int_t);
    reduce_size = ACCESS_INPUTS(2, int_t);

    imat0 = ACCESS_INPUTS(3, void*);
    imat1 = ACCESS_INPUTS(4, void*);
    omat = ACCESS_OUTPUTS(0, void*);
    // hthread_printf("op_matmul_raw begin 1\n");
    int_t i, j, k;
    for(i=0; i<row_size; i++){
        for(j=0; j<col_size; j++){
            for(k=0; k<reduce_size; k++){
                omat[i*col_size+j] += imat0[i*reduce_size+k] * imat1[j*reduce_size+k];
            }
        }
    }
    // hthread_printf("op_matmul_raw end\n");
    return task_ret_done;
}

//A*BT
task_ret op_matmul(void** inputs, void**outputs){
    // hthread_printf("op_matmul %d\n", get_core_id());
    // int_t row_size, col_size, reduce_size;
    int_t* size_array = ACCESS_INPUTS(0, int_t*);
    int_t row_size = size_array[3];
    int_t col_size = size_array[4];
    int_t reduce_size = size_array[5];
    // int_t row_blk_size = size_array[3];
    // int_t col_blk_size = size_array[4];
    // int_t reduce_blk_size = size_array[5];
    double* imat0, * imat1, * omat;
    // row_size = ACCESS_INPUTS(0, int_t);
    // col_size = ACCESS_INPUTS(1, int_t);
    // reduce_size = ACCESS_INPUTS(2, int_t);

    imat0 = ACCESS_INPUTS(1, void*);
    imat1 = ACCESS_INPUTS(2, void*);
    omat = ACCESS_OUTPUTS(0, void*);

    int_t i, j, k;
    for(k=0; k<reduce_size; k++){
        for(i=0; i<row_size; i++){
            for(j=0; j<col_size; j++){
                omat[i * col_size + j] += imat0[k * row_size + i] * imat1[k * col_size + j];
            }
        }
    }

    // ddq_log0("imat0:\n");
    // for(i=0; i<row_size; i++){
    //     for(j=0; j<reduce_size; j++){
    //         ddq_log0("%f ", imat0[i * reduce_size + j]);
    //     }
    //     ddq_log0("\n");
    // }

    // ddq_log0("imat1:\n");
    // for(i=0; i<row_size; i++){
    //     for(j=0; j<reduce_size; j++){
    //         ddq_log0("%f ", imat1[i * reduce_size + j]);
    //     }
    //     ddq_log0("\n");
    // }
    // for(i=0; i<row_size; i++){
    //     for(j=0; j<col_size; j++){
    //         for(k=0; k<reduce_size; k++){
    //             omat[i*col_size+j] += imat0[i*reduce_size+k] * imat1[j*reduce_size+k];
    //         }
    //     }
    // }
    // hthread_printf("op_matmul finished %d\n", get_core_id());
    return task_ret_ok;
}

task_ret op_iter_to_ptr(void** inputs, void** outputs, void** attributes){
    // hthread_printf("op_iter_to_ptr begin 1\n");
    // ddq_log0("op_iter_to_ptr %d\n", get_core_id());
    int_t* size_array = ACCESS_INPUTS(0, int_t*);
    int_t row_size = size_array[0];
    int_t col_size = size_array[1];
    int_t reduce_size = size_array[2];
    int_t row_blk_size = size_array[3];
    int_t col_blk_size = size_array[4];
    int_t reduce_blk_size = size_array[5];
    // ddq_log0("row_size:%ld, col_size:%ld, reduce_size:%ld, row_blk_size:%ld, col_blk_size:%ld, reduce_blk_size:%ld\n", row_size, col_size, reduce_size, row_blk_size, col_blk_size, reduce_blk_size);
    int_t* idx_array = ACCESS_INPUTS(1, int_t*);
// hthread_printf("op_iter_to_ptr begin 1\n");
    int_t* thread_idx = ACCESS_INPUTS(2, int_t*);
    // hthread_printf("op_iter_to_ptr begin 2\n");
    int_t res_idx = idx_array[0];
// hthread_printf("op_iter_to_ptr begin 3 %p, %ld %ld\n", idx_array, *thread_idx, res_idx);
    int_t* iter_idx_ptr = ACCESS_INPUTS(3, int_t*);
    int_t iter_idx = iter_idx_ptr[0];
// hthread_printf("op_iter_to_ptr begin 4\n");
    double* imat0, * imat1, * omat;
    imat0 = ACCESS_INPUTS(4, void*);
    imat1 = ACCESS_INPUTS(5, void*);
    omat = ACCESS_INPUTS(6, void*);
// hthread_printf("op_iter_to_ptr begin 5\n");
    int_t row_blk_idx = res_idx / (col_size/ col_blk_size);
    int_t col_blk_idx = res_idx % (col_size/ col_blk_size);
    int_t blk_size = row_blk_size * col_blk_size;
    int_t imat0_blk_idx = row_blk_idx * (reduce_size / reduce_blk_size) + iter_idx;
    int_t imat1_blk_idx = iter_idx * (reduce_size / reduce_blk_size) + col_blk_idx;
    double* imat0_blk = imat0 + imat0_blk_idx * blk_size;
    double* imat1_blk = imat1 + imat1_blk_idx * blk_size;
    double* omat_blk = omat + res_idx * blk_size;
// hthread_printf("op_iter_to_ptr begin 6 %p %ld %p %p %d\n",imat0,imat0_blk_idx, imat0_blk, imat1_blk,get_thread_id());
    WRITE_OUTPUTS(0, double*, imat0_blk);
    WRITE_OUTPUTS(1, double*, imat1_blk);
    WRITE_OUTPUTS(2, double*, omat_blk);
    // hthread_printf("op_iter_to_ptr end\n");
    if(iter_idx + 1 < reduce_size / reduce_blk_size){
        iter_idx_ptr[0] = iter_idx + 1;
        // ddq_log0("op_iter_to_ptr finished %d\n", get_core_id());
        return task_ret_ok;
    }else{
        iter_idx_ptr[0] = 0;
        // ddq_log0("op_iter_to_ptr finished %d\n", get_core_id());
        return task_ret_done;
    }


}

// lvector double mov_s2v_df(double* src){
//     mov_to_svr0_df(src[0]);
//     mov_to_svr1_df(src[1]);
//     mov_to_svr2_df(src[2]);
//     mov_to_svr3_df(src[3]);
//     mov_to_svr4_df(src[4]);
//     mov_to_svr5_df(src[5]);
//     mov_to_svr6_df(src[6]);
//     mov_to_svr7_df(src[7]);
//     mov_to_svr8_df(src[8]);
//     mov_to_svr9_df(src[9]);
//     mov_to_svr10_df(src[10]);
//     mov_to_svr11_df(src[11]);
//     mov_to_svr12_df(src[12]);
//     mov_to_svr13_df(src[13]);
//     mov_to_svr14_df(src[14]);
//     mov_to_svr15_df(src[15]);
//     return mov_from_svr_v16df();
// }

void vec_print_df(lvector double vec){
    mov_to_svr_v16df(vec);
    hthread_printf("%f ", mov_from_svr0());
    hthread_printf("%f ", mov_from_svr1());
    hthread_printf("%f ", mov_from_svr2());
    hthread_printf("%f ", mov_from_svr3());
    hthread_printf("%f ", mov_from_svr4());
    hthread_printf("%f ", mov_from_svr5());
    hthread_printf("%f ", mov_from_svr6());
    hthread_printf("%f ", mov_from_svr7());
    hthread_printf("%f ", mov_from_svr8());
    hthread_printf("%f ", mov_from_svr9());
    hthread_printf("%f ", mov_from_svr10());
    hthread_printf("%f ", mov_from_svr11());
    hthread_printf("%f ", mov_from_svr12());
    hthread_printf("%f ", mov_from_svr13());
    hthread_printf("%f ", mov_from_svr14());
    hthread_printf("%f\n", mov_from_svr15());

}

task_ret op_gemm_old(void** inputs, void**outputs){
    // if(get_thread_id() == 1)
    //     hthread_printf("op_matmul %d\n", get_core_id());
    double* A;
    lvector double* B, * C, * res;

    A = ((obj_mem)inputs[0])->p;
    B = (lvector double*)((obj_mem)inputs[1])->p;
    C = (lvector double*)((obj_mem)inputs[2])->p;
    res = (lvector double*)((obj_mem)outputs[0])->p;
    // int_t gemm_offset = (int_t)inputs[3];

    int_t* size_array = (int_t*)inputs[3];
    int_t row_size = size_array[0];
    int_t col_size = size_array[1];
    int_t reduce_size = size_array[2];
    int_t iter = size_array[3];

    if(!res){
        res = vector_malloc(row_size * col_size * size_array[3] * sizeof(double));
        ((obj_mem)outputs[0])->p = res;
        ((obj_mem)outputs[0])->size = row_size * col_size * size_array[3] * sizeof(double);
        ((obj_mem)outputs[0])->bufsize = row_size * col_size * size_array[3] * sizeof(double);
    }
    
    int_t i, j, k;
    
    res += size_array[4] * row_size * col_size / 16;
    C += size_array[4] * row_size * col_size / 16;

    lvector double vec_a;
    lvector double vec_b0, vec_b1, vec_b2;
    lvector double vec_c0, vec_c1, vec_c2;
    
    for(k=0; k<1; k++){
        for(i=0; i<row_size; i++){
            vec_a = vec_svbcast(A[i*reduce_size + k]);
            for(j=0; j<col_size; j += 48){
                vec_b0 = vec_ld(j + k * col_size, B);
                vec_b1 = vec_ld(j + 16 + k * col_size, B);
                vec_b2 = vec_ld(j + 32 + k * col_size, B);

                vec_c0 = vec_ld(j + i * col_size, C);
                vec_c1 = vec_ld(j + 16 + i * col_size, C);
                vec_c2 = vec_ld(j + 32 + i * col_size, C);

                vec_c0 = vec_mula(vec_a, vec_b0, vec_c0);
                vec_c1 = vec_mula(vec_a, vec_b1, vec_c1);
                vec_c2 = vec_mula(vec_a, vec_b2, vec_c2);

                vec_st(vec_c0, j + i * col_size, res);
                vec_st(vec_c1, j + 16 + i * col_size, res);
                vec_st(vec_c2, j + 32 + i * col_size, res);
            }
        }
    }

    for(k=1; k<reduce_size; k++){
        for(i=0; i<row_size; i+=1){
            // hthread_printf("A: %ld %f\n", i, A[i*reduce_size + k]);
            vec_a = vec_svbcast(A[i*reduce_size + k]);
            for(j=0; j<col_size; j += 48){
                vec_b0 = vec_ld(j + k * col_size, B);
                vec_b1 = vec_ld(j + 16 + k * col_size, B);
                vec_b2 = vec_ld(j + 32 + k * col_size, B);

                vec_c0 = vec_ld(j + i * col_size, res);
                vec_c1 = vec_ld(j + 16 + i * col_size, res);
                vec_c2 = vec_ld(j + 32 + i * col_size, res);

                vec_c0 = vec_mula(vec_a, vec_b0, vec_c0);
                vec_c1 = vec_mula(vec_a, vec_b1, vec_c1);
                vec_c2 = vec_mula(vec_a, vec_b2, vec_c2);

                vec_st(vec_c0, j + i * col_size, res);
                vec_st(vec_c1, j + 16 + i * col_size, res);
                vec_st(vec_c2, j + 32 + i * col_size, res);
            }
        }
    }
    
    if((++size_array[4]) < size_array[3]){
        return task_ret_write_partial;
    }
    
    size_array[4] = 0;
    return task_ret_ok;
}

task_ret op_gemm(void** inputs, void**outputs){
    // if(get_thread_id() == 1)
    //     hthread_printf("op_matmul %d\n", get_core_id());
    double* A;
    lvector double* B, * C, * res;

    A = ((obj_mem)inputs[0])->p;
    B = (lvector double*)((obj_mem)inputs[1])->p;
    C = (lvector double*)((obj_mem)inputs[2])->p;
    res = (lvector double*)((obj_mem)outputs[0])->p;
    // int_t gemm_offset = (int_t)inputs[3];

    int_t* size_array = (int_t*)inputs[3];
    int_t row_size = size_array[0];
    int_t col_size = size_array[1];
    int_t reduce_size = size_array[2];
    int_t iter = size_array[3];

    if(!res){
        res = vector_malloc(row_size * col_size * size_array[3] * sizeof(double));
        ((obj_mem)outputs[0])->p = res;
        ((obj_mem)outputs[0])->size = row_size * col_size * size_array[3] * sizeof(double);
        ((obj_mem)outputs[0])->bufsize = row_size * col_size * size_array[3] * sizeof(double);
    }
    
    res += size_array[4] * row_size * col_size / 16;
    C += size_array[4] * row_size * col_size / 16;

    lvector double vec_a_0, vec_a_1, vec_a_2, vec_a_3, vec_a_4, vec_a_5;
    lvector double vec_b0, vec_b1, vec_b2;
    lvector double vec_c0_0, vec_c1_0, vec_c2_0;
    lvector double vec_c0_1, vec_c1_1, vec_c2_1;
    lvector double vec_c0_2, vec_c1_2, vec_c2_2;
    lvector double vec_c0_3, vec_c1_3, vec_c2_3;
    lvector double vec_c0_4, vec_c1_4, vec_c2_4;
    lvector double vec_c0_5, vec_c1_5, vec_c2_5;

    int_t i=0, j=0, k=0;
    // for(k=0; k<1; k++){
    //     for(i=0; i<row_size; i+=6){
            // hthread_printf("A: %ld %f\n", i, A[i*reduce_size + k]);
            vec_a_0 = vec_svbcast(A[i*reduce_size + k]);
            vec_a_1 = vec_svbcast(A[(i+1)*reduce_size + k]);
            vec_a_2 = vec_svbcast(A[(i+2)*reduce_size + k]);
            vec_a_3 = vec_svbcast(A[(i+3)*reduce_size + k]);
            vec_a_4 = vec_svbcast(A[(i+4)*reduce_size + k]);
            vec_a_5 = vec_svbcast(A[(i+5)*reduce_size + k]);

            // for(j=0; j<col_size; j += 48){
                vec_b0 = vec_ld(j + k * col_size, B);
                vec_b1 = vec_ld(j + 16 + k * col_size, B);
                vec_b2 = vec_ld(j + 32 + k * col_size, B);

                vec_c0_0 = vec_ld(j + i * col_size, C);
                vec_c1_0 = vec_ld(j + 16 + i * col_size, C);
                vec_c2_0 = vec_ld(j + 32 + i * col_size, C);

                vec_c0_1 = vec_ld(j + (i+1) * col_size, C);
                vec_c1_1 = vec_ld(j + 16 + (i+1) * col_size, C);
                vec_c2_1 = vec_ld(j + 32 + (i+1) * col_size, C);

                vec_c0_2 = vec_ld(j + (i+2) * col_size, C);
                vec_c1_2 = vec_ld(j + 16 + (i+2) * col_size, C);
                vec_c2_2 = vec_ld(j + 32 + (i+2) * col_size, C);

                vec_c0_3 = vec_ld(j + (i+3) * col_size, C);
                vec_c1_3 = vec_ld(j + 16 + (i+3) * col_size, C);
                vec_c2_3 = vec_ld(j + 32 + (i+3) * col_size, C);

                vec_c0_4 = vec_ld(j + (i+4) * col_size, C);
                vec_c1_4 = vec_ld(j + 16 + (i+4) * col_size, C);
                vec_c2_4 = vec_ld(j + 32 + (i+4) * col_size, C);

                vec_c0_5 = vec_ld(j + (i+5) * col_size, C);
                vec_c1_5 = vec_ld(j + 16 + (i+5) * col_size, C);
                vec_c2_5 = vec_ld(j + 32 + (i+5) * col_size, C);

                vec_c0_0 = vec_mula(vec_a_0, vec_b0, vec_c0_0);
                vec_c1_0 = vec_mula(vec_a_0, vec_b1, vec_c1_0);
                vec_c2_0 = vec_mula(vec_a_0, vec_b2, vec_c2_0);

                vec_c0_1 = vec_mula(vec_a_1, vec_b0, vec_c0_1);
                vec_c1_1 = vec_mula(vec_a_1, vec_b1, vec_c0_1);
                vec_c2_1 = vec_mula(vec_a_1, vec_b2, vec_c0_1);

                vec_c0_2 = vec_mula(vec_a_2, vec_b0, vec_c0_2);
                vec_c1_2 = vec_mula(vec_a_2, vec_b1, vec_c0_2);
                vec_c2_2 = vec_mula(vec_a_2, vec_b2, vec_c0_2);

                vec_c0_3 = vec_mula(vec_a_3, vec_b0, vec_c0_3);
                vec_c1_3 = vec_mula(vec_a_3, vec_b1, vec_c0_3);
                vec_c2_3 = vec_mula(vec_a_3, vec_b2, vec_c0_3);

                vec_c0_4 = vec_mula(vec_a_4, vec_b0, vec_c0_4);
                vec_c1_4 = vec_mula(vec_a_4, vec_b1, vec_c0_4);
                vec_c2_4 = vec_mula(vec_a_4, vec_b2, vec_c0_4);

                vec_c0_5 = vec_mula(vec_a_5, vec_b0, vec_c0_5);
                vec_c1_5 = vec_mula(vec_a_5, vec_b1, vec_c0_5);
                vec_c2_5 = vec_mula(vec_a_5, vec_b2, vec_c0_5);

                vec_st(vec_c0_0, j + i * col_size, res);
                vec_st(vec_c1_0, j + 16 + i * col_size, res);
                vec_st(vec_c2_0, j + 32 + i * col_size, res);

                vec_st(vec_c0_1, j + (i+1) * col_size, res);
                vec_st(vec_c1_1, j + 16 + (i+1) * col_size, res);
                vec_st(vec_c2_1, j + 32 + (i+1) * col_size, res);

                vec_st(vec_c0_2, j + (i+2) * col_size, res);
                vec_st(vec_c1_2, j + 16 + (i+2) * col_size, res);
                vec_st(vec_c2_2, j + 32 + (i+2) * col_size, res);

                vec_st(vec_c0_3, j + (i+3) * col_size, res);
                vec_st(vec_c1_3, j + 16 + (i+3) * col_size, res);
                vec_st(vec_c2_3, j + 32 + (i+3) * col_size, res);

                vec_st(vec_c0_4, j + (i+4) * col_size, res);
                vec_st(vec_c1_4, j + 16 + (i+4) * col_size, res);
                vec_st(vec_c2_4, j + 32 + (i+4) * col_size, res);

                vec_st(vec_c0_5, j + (i+5) * col_size, res);
                vec_st(vec_c1_5, j + 16 + (i+5) * col_size, res);
                vec_st(vec_c2_5, j + 32 + (i+5) * col_size, res);
    //         }
    //     }
    // }

    for(k=1; k<reduce_size; k++){
        // for(i=0; i<row_size; i+=6){
            // hthread_printf("A: %ld %f\n", i, A[i*reduce_size + k]);
            vec_a_0 = vec_svbcast(A[i*reduce_size + k]);
            vec_a_1 = vec_svbcast(A[(i+1)*reduce_size + k]);
            vec_a_2 = vec_svbcast(A[(i+2)*reduce_size + k]);
            vec_a_3 = vec_svbcast(A[(i+3)*reduce_size + k]);
            vec_a_4 = vec_svbcast(A[(i+4)*reduce_size + k]);
            vec_a_5 = vec_svbcast(A[(i+5)*reduce_size + k]);
            // for(j=0; j<col_size; j += 48){
                vec_b0 = vec_ld(j + k * col_size, B);
                vec_b1 = vec_ld(j + 16 + k * col_size, B);
                vec_b2 = vec_ld(j + 32 + k * col_size, B);

                vec_c0_0 = vec_ld(j + i * col_size, res);
                vec_c1_0 = vec_ld(j + 16 + i * col_size, res);
                vec_c2_0 = vec_ld(j + 32 + i * col_size, res);

                vec_c0_1 = vec_ld(j + (i+1) * col_size, res);
                vec_c1_1 = vec_ld(j + 16 + (i+1) * col_size, res);
                vec_c2_1 = vec_ld(j + 32 + (i+1) * col_size, res);

                vec_c0_2 = vec_ld(j + (i+2) * col_size, res);
                vec_c1_2 = vec_ld(j + 16 + (i+2) * col_size, res);
                vec_c2_2 = vec_ld(j + 32 + (i+2) * col_size, res);

                vec_c0_3 = vec_ld(j + (i+3) * col_size, res);
                vec_c1_3 = vec_ld(j + 16 + (i+3) * col_size, res);
                vec_c2_3 = vec_ld(j + 32 + (i+3) * col_size, res);

                vec_c0_4 = vec_ld(j + (i+4) * col_size, res);
                vec_c1_4 = vec_ld(j + 16 + (i+4) * col_size, res);
                vec_c2_4 = vec_ld(j + 32 + (i+4) * col_size, res);

                vec_c0_5 = vec_ld(j + (i+5) * col_size, res);
                vec_c1_5 = vec_ld(j + 16 + (i+5) * col_size, res);
                vec_c2_5 = vec_ld(j + 32 + (i+5) * col_size, res);

                vec_c0_0 = vec_mula(vec_a_0, vec_b0, vec_c0_0);
                vec_c1_0 = vec_mula(vec_a_0, vec_b1, vec_c1_0);
                vec_c2_0 = vec_mula(vec_a_0, vec_b2, vec_c2_0);

                vec_c0_1 = vec_mula(vec_a_1, vec_b0, vec_c0_1);
                vec_c1_1 = vec_mula(vec_a_1, vec_b1, vec_c0_1);
                vec_c2_1 = vec_mula(vec_a_1, vec_b2, vec_c0_1);

                vec_c0_2 = vec_mula(vec_a_2, vec_b0, vec_c0_2);
                vec_c1_2 = vec_mula(vec_a_2, vec_b1, vec_c0_2);
                vec_c2_2 = vec_mula(vec_a_2, vec_b2, vec_c0_2);

                vec_c0_3 = vec_mula(vec_a_3, vec_b0, vec_c0_3);
                vec_c1_3 = vec_mula(vec_a_3, vec_b1, vec_c0_3);
                vec_c2_3 = vec_mula(vec_a_3, vec_b2, vec_c0_3);

                vec_c0_4 = vec_mula(vec_a_4, vec_b0, vec_c0_4);
                vec_c1_4 = vec_mula(vec_a_4, vec_b1, vec_c0_4);
                vec_c2_4 = vec_mula(vec_a_4, vec_b2, vec_c0_4);

                vec_c0_5 = vec_mula(vec_a_5, vec_b0, vec_c0_5);
                vec_c1_5 = vec_mula(vec_a_5, vec_b1, vec_c0_5);
                vec_c2_5 = vec_mula(vec_a_5, vec_b2, vec_c0_5);

                vec_st(vec_c0_0, j + i * col_size, res);
                vec_st(vec_c1_0, j + 16 + i * col_size, res);
                vec_st(vec_c2_0, j + 32 + i * col_size, res);

                vec_st(vec_c0_1, j + (i+1) * col_size, res);
                vec_st(vec_c1_1, j + 16 + (i+1) * col_size, res);
                vec_st(vec_c2_1, j + 32 + (i+1) * col_size, res);

                vec_st(vec_c0_2, j + (i+2) * col_size, res);
                vec_st(vec_c1_2, j + 16 + (i+2) * col_size, res);
                vec_st(vec_c2_2, j + 32 + (i+2) * col_size, res);

                vec_st(vec_c0_3, j + (i+3) * col_size, res);
                vec_st(vec_c1_3, j + 16 + (i+3) * col_size, res);
                vec_st(vec_c2_3, j + 32 + (i+3) * col_size, res);

                vec_st(vec_c0_4, j + (i+4) * col_size, res);
                vec_st(vec_c1_4, j + 16 + (i+4) * col_size, res);
                vec_st(vec_c2_4, j + 32 + (i+4) * col_size, res);

                vec_st(vec_c0_5, j + (i+5) * col_size, res);
                vec_st(vec_c1_5, j + 16 + (i+5) * col_size, res);
                vec_st(vec_c2_5, j + 32 + (i+5) * col_size, res);
            // }
        // }
    }
    
    if((++size_array[4]) < size_array[3]){
        return task_ret_write_partial;
    }
    
    size_array[4] = 0;
    return task_ret_ok;
}

//inputs: sizes,steps,offsets outputs:iter
task_ret op_generator_row(void** inputs, void** outputs, void** attributes){
    int_t* array = (int_t*)(inputs[0]);
    int_t* start = (int_t*)(inputs[1]);
    int_t len = array[0];
    int_t* sizes = array + 1;
    int_t* steps = sizes + len;
    int_t* offsets = steps + len; 
    int_t i=0;
    // ddq_log0("op_generator_row begin len %ld, start:%ld, sizes:%ld %ld, steps:%ld %ld offsets:%ld %ld\n", len, *start, sizes[0], sizes[1], steps[0], steps[1],offsets[0], offsets[1]);
    *(int_t*)outputs[0] = *start;
    for(i=0; i<len; i++){
        *(int_t*)outputs[0] += (steps[i] * offsets[i]);
    }
    offsets[0] += 1;
    for(i=0; i<len-1; i++){
        offsets[i+1] += (offsets[i] / sizes[i]);
        offsets[i] = offsets[i] % sizes[i];
    }
    if(offsets[i] < sizes[i]){
        return task_ret_read_partial;
    }
    for(i=0; i<len; i++){
        offsets[i] = 0;
    }
    return task_ret_ok;
}

//inputs: sizes,steps,offsets outputs:iter
task_ret op_generator_col(void** inputs, void** outputs, void** attributes){
    int_t* array = (int_t*)(inputs[0]);
    int_t* start = (int_t*)(inputs[1]);
    int_t len = array[0];
    int_t* sizes = array + 1;
    int_t* steps = sizes + len;
    int_t* offsets = steps + len; 
    int_t i=0;
    // ddq_log0("op_generator_col begin len %ld, start:%ld, sizes:%ld %ld, steps:%ld %ld offsets:%ld %ld\n", len, *start, sizes[0], sizes[1], steps[0], steps[1],offsets[0], offsets[1]);
    *(int_t*)outputs[0] = *start;
    for(i=0; i<len; i++){
        *(int_t*)outputs[0] += (steps[i] * offsets[i]);
    }
    // ddq_log0("op_generator_col 1\n");
    offsets[len-1] += 1;
    for(i=len-1; i>0; i--){
        offsets[i-1] += (offsets[i] / sizes[i]);
        offsets[i] = offsets[i] % sizes[i];
    }
    // ddq_log0("op_generator_col after len %ld, start:%ld, sizes:%ld %ld, steps:%ld %ld offsets:%ld %ld\n", len, *start, sizes[0], sizes[1], steps[0], steps[1],offsets[0], offsets[1]);
    if(offsets[0] < sizes[0]){
        return task_ret_read_partial;
    }
    for(i=0; i<len; i++){
        offsets[i] = 0;
    }
    return task_ret_ok;
}

task_ret op_shift_with_thread_id(void** inputs, void** outputs, void** attributes){
    int_t* shift = (int_t*)(inputs[0]);
    *(int_t*)outputs[0] = *shift * get_thread_id();
    return task_ret_done;
}

task_ret op_finish(void** inputs, void** outputs, void** attributes){
    void* ptr = inputs[0];
    free(ptr);
    return task_ret_done;
}

task_ret op_nothing(void** inputs, void** outputs, void** attributes){return task_ret_ok;}

task_ret op_my_initial(void** inputs, void** outputs, void** attributes){
    int_t* initial_params = inputs[0];
    int_t row_size = initial_params[0];
    int_t col_size = initial_params[1];
    int_t reduce_size = initial_params[2];
    int_t Mg = initial_params[3];
    int_t Ng = initial_params[4];
    int_t Kg = initial_params[5];
    int_t Ma = initial_params[6];
    int_t Na = initial_params[7];
    int_t Ka = initial_params[8];
    int_t Ms = initial_params[9];

    void* ptr = malloc(sizeof(int_t) * 87);
    int_t* params_A2Agsm = ptr;
    params_A2Agsm[0] = Mg;
    params_A2Agsm[1] = Kg*sizeof(double);
    params_A2Agsm[2] = (reduce_size-Kg)*sizeof(double);
    params_A2Agsm[3] = 1;
    params_A2Agsm[4] = Mg*Kg*sizeof(double);
    params_A2Agsm[5] = 0;
    outputs[0] = params_A2Agsm;

    int_t* params_B2Bam = params_A2Agsm + 6;
    params_B2Bam[0] = Ka;
    params_B2Bam[1] = Na*sizeof(double);
    params_B2Bam[2] = (col_size-Na) * sizeof(double);
    params_B2Bam[3] = 1;
    params_B2Bam[4] = Ka * Na * sizeof(double);
    params_B2Bam[5] = 0;
    outputs[1] = params_B2Bam;

    int_t* params_C2Cam = params_B2Bam + 6;
    params_C2Cam[0] = Ma;
    params_C2Cam[1] = Na * sizeof(double);
    params_C2Cam[2] = (col_size-Na) * sizeof(double);
    params_C2Cam[3] = 1;
    params_C2Cam[4] = Ma*Na*sizeof(double);
    params_C2Cam[5] = 0;
    outputs[2] = params_C2Cam;

    int_t* params_gsm2Asm = params_C2Cam + 6;
    params_gsm2Asm[0] = Ms;
    params_gsm2Asm[1] = Ka * sizeof(double);
    params_gsm2Asm[2] = (Kg-Ka) * sizeof(double);
    params_gsm2Asm[3] = 1;
    params_gsm2Asm[4] = Ms * Ka * sizeof(double);
    params_gsm2Asm[5] = 0;
    outputs[3] = params_gsm2Asm;

    int_t* params_Cam2C = params_gsm2Asm + 6;
    params_Cam2C[0] = 1;
    params_Cam2C[1] = Ma*Na*sizeof(double);
    params_Cam2C[2] = 0;
    params_Cam2C[3] = Ma;
    params_Cam2C[4] = Na * sizeof(double);
    params_Cam2C[5] = (col_size-Na) * sizeof(double);
    outputs[4] = params_Cam2C;

    int_t* size_array = params_Cam2C + 6;
    size_array[0] = Ms;
    size_array[1] = Na;
    size_array[2] = Ka;
    size_array[3] = Ma/Ms;
    size_array[4] = 0;
    outputs[5] = size_array;

    //gen_array的step有误
   int_t* gen_array_A = size_array + 5;
   gen_array_A[0] = 2;
   gen_array_A[1] = reduce_size/Kg;
   gen_array_A[2] = row_size/Mg;
   gen_array_A[3] = Kg*sizeof(double); //Kg
   gen_array_A[4] = Mg*reduce_size*sizeof(double); //Mg
   gen_array_A[5] = 0;
   gen_array_A[6] = 0;
   outputs[6] = gen_array_A;

   int_t* gen_array_B = gen_array_A + 7;
   gen_array_B[0] = 2;
   gen_array_B[1] = 1;
   gen_array_B[2] = reduce_size/Kg;
   gen_array_B[3] = Ng*sizeof(double); //Ng
   gen_array_B[4] = Kg*col_size*sizeof(double); //Kg
   gen_array_B[5] = 0;
   gen_array_B[6] = 0;
   outputs[7] = gen_array_B;

   int_t* gen_array_C = gen_array_B + 7;
   gen_array_C[0] = 2;
   gen_array_C[1] = 1;
   gen_array_C[2] = row_size/Mg;
   gen_array_C[3] = Ng*sizeof(double); //Ng
   gen_array_C[4] = Mg*col_size*sizeof(double); //Mg
   gen_array_C[5] = 0;
   gen_array_C[6] = 0;
   outputs[8] = gen_array_C;

   int_t* gen_array_Agsm_1 = gen_array_C + 7;
   gen_array_Agsm_1[0] = 2;
   gen_array_Agsm_1[1] = Kg/Ka;
   gen_array_Agsm_1[2] = 1;
   gen_array_Agsm_1[3] = Ka*sizeof(double);
   gen_array_Agsm_1[4] = Mg*Kg*sizeof(double); //gaile Ma*Kg*sizeof(double)
   gen_array_Agsm_1[5] = 0;
   gen_array_Agsm_1[6] = 0;
   outputs[9] = gen_array_Agsm_1;

   int_t* gen_array_Agsm_2 = gen_array_Agsm_1 + 7;
   gen_array_Agsm_2[0] = 2;
   gen_array_Agsm_2[1] = 1;
   gen_array_Agsm_2[2] = Mg/Ms;
   gen_array_Agsm_2[3] = Ka*sizeof(double); //Ka
   gen_array_Agsm_2[4] = Ms*Kg*sizeof(double); //Ms gaile Ms*Ka*sizeof(double)
   gen_array_Agsm_2[5] = 0;
   gen_array_Agsm_2[6] = 0;
   outputs[10] = gen_array_Agsm_2;

   int_t* gen_array_Bgsm = gen_array_Agsm_2 + 7;
   gen_array_Bgsm[0] = 2;
   gen_array_Bgsm[1] = Ng/Na;
   gen_array_Bgsm[2] = Kg/Ka;
   gen_array_Bgsm[3] = Na*sizeof(double); //Na
   gen_array_Bgsm[4] = Ka*col_size*sizeof(double); //Ka
   gen_array_Bgsm[5] = 0;
   gen_array_Bgsm[6] = 0;
   outputs[11] = gen_array_Bgsm;

   int_t* gen_array_Cgsm = gen_array_Bgsm + 7;
   gen_array_Cgsm[0] = 2;
   gen_array_Cgsm[1] = Ng/Na;
   gen_array_Cgsm[2] = Mg/Ma;
   gen_array_Cgsm[3] = Na*sizeof(double); //Na
   gen_array_Cgsm[4] = Ma*col_size*sizeof(double); //Ma
   gen_array_Cgsm[5] = 0;
   gen_array_Cgsm[6] = 0;
   outputs[12] = gen_array_Cgsm;

   int_t* iter_start = outputs[13];
   *iter_start = 0;

   int_t* shift_B = outputs[14];
   *shift_B = Ng*sizeof(double);

   int_t* shift_C = outputs[15];
   *shift_C = Ng*sizeof(double);

   return task_ret_done;
}

//mid_base ==0 && gsm_mid_base ==0
// task_ret op_linear_0(void** inputs, void** outputs, void** attributes){
//     lvector float* x = (lvector float*)inputs[0];
//     lvector float* y = (lvector float*)inputs[1];
//     lvector float* z = (lvector float*)inputs[2];
//     lvector float* b = (lvector float*)inputs[3];
//     int col_base = inputs[4];
//     int tmp_step = (inner - col_step) / 32;

//     lvector float x_0, x_1, x_2, x_3, x_4, x_5, x_6, x_7, x_8, x_9, x_10, x_11;
//     lvector float y_0, y_1, y_2, y_3, y_4, y_5, y_6, y_7;
//     lvector float z_0, z_1, z_2, z_3, z_4, z_5, z_6, z_7, z_8, z_9, z_10, z_11, z_12, z_13, z_14, z_15, z_16, z_17, z_18, z_19, z_20, z_21, z_22, z_23;
    
//     int outer = 30;
//     int inner = 256;
//     int row_step = 6;
//     int col_step = 128;
//     int mid_step = 256;
//     for (int row_group = 0; row_group < outer / row_step; row_group++) {
//         for (int col_group = 0; col_group < inner / col_step; col_group++) {
//             int tmp = row_group * row_step * inner / 32 + col_group * 4;
//             int locy = col_group * 4;
//             {
//                 int tmp_ = col_group * 4 + col_base / 32;
//                 z_0 = vec_ld(0, b + tmp_);
//                 z_1 = vec_ld(0, b + tmp_ + 1);
//                 z_2 = vec_ld(0, b + tmp_ + 2);
//                 z_3 = vec_ld(0, b + tmp_ + 3);
//                 z_4 = vec_ld(0, b + tmp_);
//                 z_5 = vec_ld(0, b + tmp_ + 1);
//                 z_6 = vec_ld(0, b + tmp_ + 2);
//                 z_7 = vec_ld(0, b + tmp_ + 3);
//                 z_8 = vec_ld(0, b + tmp_);
//                 z_9 = vec_ld(0, b + tmp_ + 1);
//                 z_10 = vec_ld(0, b + tmp_ + 2);
//                 z_11 = vec_ld(0, b + tmp_ + 3);
//                 z_12 = vec_ld(0, b + tmp_);
//                 z_13 = vec_ld(0, b + tmp_ + 1);
//                 z_14 = vec_ld(0, b + tmp_ + 2);
//                 z_15 = vec_ld(0, b + tmp_ + 3);
//                 z_16 = vec_ld(0, b + tmp_);
//                 z_17 = vec_ld(0, b + tmp_ + 1);
//                 z_18 = vec_ld(0, b + tmp_ + 2);
//                 z_19 = vec_ld(0, b + tmp_ + 3);
//                 z_20 = vec_ld(0, b + tmp_);
//                 z_21 = vec_ld(0, b + tmp_ + 1);
//                 z_22 = vec_ld(0, b + tmp_ + 2);
//                 z_23 = vec_ld(0, b + tmp_ + 3);
//             }
//             int locx = row_group * row_step * mid_step;
//             y_0 = vec_ld(0, y + locy);
//             y_1 = vec_ld(0, y + locy + 1);
//             y_2 = vec_ld(0, y + locy + 2);
//             y_3 = vec_ld(0, y + locy + 3);
//             y_4 = vec_ld(0, y + locy + inner / 32);
//             y_5 = vec_ld(0, y + locy + inner / 32 + 1);
//             y_6 = vec_ld(0, y + locy + inner / 32 + 2);
//             y_7 = vec_ld(0, y + locy + inner / 32 + 3);

//             x_0 = vec_fexts32l(x[locx]);
//             x_1 = vec_fexts32l(x[locx + mid_step]);
//             x_2 = vec_fexts32l(x[locx + mid_step * 2]);
//             x_3 = vec_fexts32l(x[locx + mid_step * 3]);
//             x_4 = vec_fexts32l(x[locx + mid_step * 4]);
//             x_5 = vec_fexts32l(x[locx + mid_step * 5]);
//             x_6 = vec_fexts32l(x[locx + 1]);
//             x_7 = vec_fexts32l(x[locx + mid_step + 1]);
//             x_8 = vec_fexts32l(x[locx + mid_step * 2 + 1]);
//             x_9 = vec_fexts32l(x[locx + mid_step * 3 + 1]);
//             x_10 = vec_fexts32l(x[locx + mid_step * 4 + 1]);
//             x_11 = vec_fexts32l(x[locx + mid_step * 5 + 1]);

//             tmp = row_group * row_step * inner / 32 + col_group * 4;
//             for (int i = 2; i <= mid_step; i += 2) {
//                 locx += 2;
//                 locy += inner / 16;
//                 z_0 = vec_mula(x_0, y_0, z_0);
//                 z_1 = vec_mula(x_0, y_1, z_1);
//                 z_2 = vec_mula(x_0, y_2, z_2);
//                 z_3 = vec_mula(x_0, y_3, z_3);

//                 z_4 = vec_mula(x_1, y_0, z_4);
//                 z_5 = vec_mula(x_1, y_1, z_5);
//                 z_6 = vec_mula(x_1, y_2, z_6);
//                 z_7 = vec_mula(x_1, y_3, z_7);

//                 z_8 = vec_mula(x_2, y_0, z_8);
//                 z_9 = vec_mula(x_2, y_1, z_9);
//                 z_10 = vec_mula(x_2, y_2, z_10);
//                 z_11 = vec_mula(x_2, y_3, z_11);

//                 z_12 = vec_mula(x_3, y_0, z_12);
//                 z_13 = vec_mula(x_3, y_1, z_13);
//                 z_14 = vec_mula(x_3, y_2, z_14);
//                 z_15 = vec_mula(x_3, y_3, z_15);

//                 z_16 = vec_mula(x_4, y_0, z_16);
//                 z_17 = vec_mula(x_4, y_1, z_17);
//                 z_18 = vec_mula(x_4, y_2, z_18);
//                 z_19 = vec_mula(x_4, y_3, z_19);

//                 z_20 = vec_mula(x_5, y_0, z_20);
//                 z_21 = vec_mula(x_5, y_1, z_21);
//                 z_22 = vec_mula(x_5, y_2, z_22);
//                 z_23 = vec_mula(x_5, y_3, z_23);
                
//                 z_0 = vec_mula(x_6, y_4, z_0);
//                 z_1 = vec_mula(x_6, y_5, z_1);
//                 z_2 = vec_mula(x_6, y_6, z_2);
//                 z_3 = vec_mula(x_6, y_7, z_3);

//                 z_4 = vec_mula(x_7, y_4, z_4);
//                 z_5 = vec_mula(x_7, y_5, z_5);
//                 z_6 = vec_mula(x_7, y_6, z_6);
//                 z_7 = vec_mula(x_7, y_7, z_7);

//                 z_8 = vec_mula(x_8, y_4, z_8);
//                 z_9 = vec_mula(x_8, y_5, z_9);
//                 z_10 = vec_mula(x_8, y_6, z_10);
//                 z_11 = vec_mula(x_8, y_7, z_11);

//                 z_12 = vec_mula(x_9, y_4, z_12);
//                 z_13 = vec_mula(x_9, y_5, z_13);
//                 z_14 = vec_mula(x_9, y_6, z_14);
//                 z_15 = vec_mula(x_9, y_7, z_15);

//                 z_16 = vec_mula(x_10, y_4, z_16);
//                 z_17 = vec_mula(x_10, y_5, z_17);
//                 z_18 = vec_mula(x_10, y_6, z_18);
//                 z_19 = vec_mula(x_10, y_7, z_19);

//                 z_20 = vec_mula(x_11, y_4, z_20);
//                 z_21 = vec_mula(x_11, y_5, z_21);
//                 z_22 = vec_mula(x_11, y_6, z_22);
//                 z_23 = vec_mula(x_11, y_7, z_23);

//                 x_0 = vec_fexts32l(x[locx]);
//                 x_1 = vec_fexts32l(x[locx + mid_step]);
//                 x_2 = vec_fexts32l(x[locx + mid_step * 2]);
//                 x_3 = vec_fexts32l(x[locx + mid_step * 3]);
//                 x_4 = vec_fexts32l(x[locx + mid_step * 4]);
//                 x_5 = vec_fexts32l(x[locx + mid_step * 5]);
//                 x_6 = vec_fexts32l(x[locx + 1]);
//                 x_7 = vec_fexts32l(x[locx + mid_step + 1]);
//                 x_8 = vec_fexts32l(x[locx + mid_step * 2 + 1]);
//                 x_9 = vec_fexts32l(x[locx + mid_step * 3 + 1]);
//                 x_10 = vec_fexts32l(x[locx + mid_step * 4 + 1]);
//                 x_11 = vec_fexts32l(x[locx + mid_step * 5 + 1]);
                    
//                 y_0 = vec_ld(0, y + locy);
//                 y_1 = vec_ld(0, y + locy + 1);
//                 y_2 = vec_ld(0, y + locy + 2);
//                 y_3 = vec_ld(0, y + locy + 3);
//                 y_4 = vec_ld(0, y + locy + inner / 32);
//                 y_5 = vec_ld(0, y + locy + inner / 32 + 1);
//                 y_6 = vec_ld(0, y + locy + inner / 32 + 2);
//                 y_7 = vec_ld(0, y + locy + inner / 32 + 3);
//             }
//             {
//                 z[tmp++] = z_0;
//                 z[tmp++] = z_1;
//                 z[tmp++] = z_2;
//                 z[tmp++] = z_3;
//                 tmp += tmp_step;
//                 z[tmp++] = z_4;
//                 z[tmp++] = z_5;
//                 z[tmp++] = z_6;
//                 z[tmp++] = z_7;
//                 tmp += tmp_step;
//                 z[tmp++] = z_8;
//                 z[tmp++] = z_9;
//                 z[tmp++] = z_10;
//                 z[tmp++] = z_11;
//                 tmp += tmp_step;
//                 z[tmp++] = z_12;
//                 z[tmp++] = z_13;
//                 z[tmp++] = z_14;
//                 z[tmp++] = z_15;
//                 tmp += tmp_step;
//                 z[tmp++] = z_16;
//                 z[tmp++] = z_17;
//                 z[tmp++] = z_18;
//                 z[tmp++] = z_19;
//                 tmp += tmp_step;
//                 z[tmp++] = z_20;
//                 z[tmp++] = z_21;
//                 z[tmp++] = z_22;
//                 z[tmp++] = z_23;
//                 tmp += tmp_step;
//             }
//         }
//     }
//     outputs[0] = z;

//     return task_ret_ok;
// }

//z=x*y+z 30*256*256
task_ret op_linear_1(void** inputs, void** outputs, void** attributes){
    float* x = (float*)inputs[0];
    lvector float* y = (lvector float*)inputs[1];
    lvector float* z = (lvector float*)inputs[2];

    lvector float x_0, x_1, x_2, x_3, x_4, x_5, x_6, x_7, x_8, x_9, x_10, x_11;
    lvector float y_0, y_1, y_2, y_3, y_4, y_5, y_6, y_7;
    lvector float z_0, z_1, z_2, z_3, z_4, z_5, z_6, z_7, z_8, z_9, z_10, z_11, z_12, z_13, z_14, z_15, z_16, z_17, z_18, z_19, z_20, z_21, z_22, z_23;
    
    int outer = 30;
    int inner = 256;
    int row_step = 6;
    int col_step = 128;
    int mid_step = 256;
    int tmp_step = (inner - col_step) / 32;
    for (int row_group = 0; row_group < outer / row_step; row_group++) {
        for (int col_group = 0; col_group < inner / col_step; col_group++) {
            int tmp = row_group * row_step * inner / 32 + col_group * 4;
            int locy = col_group * 4;
            {
                z_0 = z[tmp++];
                z_1 = z[tmp++];
                z_2 = z[tmp++];
                z_3 = z[tmp++];
                tmp += tmp_step;
                z_4 = z[tmp++];
                z_5 = z[tmp++];
                z_6 = z[tmp++];
                z_7 = z[tmp++];
                tmp += tmp_step;
                z_8 = z[tmp++];
                z_9 = z[tmp++];
                z_10 = z[tmp++];
                z_11 = z[tmp++];
                tmp += tmp_step;
                z_12 = z[tmp++];
                z_13 = z[tmp++];
                z_14 = z[tmp++];
                z_15 = z[tmp++];
                tmp += tmp_step;
                z_16 = z[tmp++];
                z_17 = z[tmp++];
                z_18 = z[tmp++];
                z_19 = z[tmp++];
                tmp += tmp_step;
                z_20 = z[tmp++];
                z_21 = z[tmp++];
                z_22 = z[tmp++];
                z_23 = z[tmp++];
            } 
            int locx = row_group * row_step * mid_step;
            y_0 = vec_ld(0, y + locy);
            y_1 = vec_ld(0, y + locy + 1);
            y_2 = vec_ld(0, y + locy + 2);
            y_3 = vec_ld(0, y + locy + 3);
            y_4 = vec_ld(0, y + locy + inner / 32);
            y_5 = vec_ld(0, y + locy + inner / 32 + 1);
            y_6 = vec_ld(0, y + locy + inner / 32 + 2);
            y_7 = vec_ld(0, y + locy + inner / 32 + 3);

            x_0 = vec_fexts32l(x[locx]);
            x_1 = vec_fexts32l(x[locx + mid_step]);
            x_2 = vec_fexts32l(x[locx + mid_step * 2]);
            x_3 = vec_fexts32l(x[locx + mid_step * 3]);
            x_4 = vec_fexts32l(x[locx + mid_step * 4]);
            x_5 = vec_fexts32l(x[locx + mid_step * 5]);
            x_6 = vec_fexts32l(x[locx + 1]);
            x_7 = vec_fexts32l(x[locx + mid_step + 1]);
            x_8 = vec_fexts32l(x[locx + mid_step * 2 + 1]);
            x_9 = vec_fexts32l(x[locx + mid_step * 3 + 1]);
            x_10 = vec_fexts32l(x[locx + mid_step * 4 + 1]);
            x_11 = vec_fexts32l(x[locx + mid_step * 5 + 1]);

            tmp = row_group * row_step * inner / 32 + col_group * 4;
            for (int i = 2; i <= mid_step; i += 2) {
                locx += 2;
                locy += inner / 16;
                z_0 = vec_mula(x_0, y_0, z_0);
                z_1 = vec_mula(x_0, y_1, z_1);
                z_2 = vec_mula(x_0, y_2, z_2);
                z_3 = vec_mula(x_0, y_3, z_3);

                z_4 = vec_mula(x_1, y_0, z_4);
                z_5 = vec_mula(x_1, y_1, z_5);
                z_6 = vec_mula(x_1, y_2, z_6);
                z_7 = vec_mula(x_1, y_3, z_7);

                z_8 = vec_mula(x_2, y_0, z_8);
                z_9 = vec_mula(x_2, y_1, z_9);
                z_10 = vec_mula(x_2, y_2, z_10);
                z_11 = vec_mula(x_2, y_3, z_11);

                z_12 = vec_mula(x_3, y_0, z_12);
                z_13 = vec_mula(x_3, y_1, z_13);
                z_14 = vec_mula(x_3, y_2, z_14);
                z_15 = vec_mula(x_3, y_3, z_15);

                z_16 = vec_mula(x_4, y_0, z_16);
                z_17 = vec_mula(x_4, y_1, z_17);
                z_18 = vec_mula(x_4, y_2, z_18);
                z_19 = vec_mula(x_4, y_3, z_19);

                z_20 = vec_mula(x_5, y_0, z_20);
                z_21 = vec_mula(x_5, y_1, z_21);
                z_22 = vec_mula(x_5, y_2, z_22);
                z_23 = vec_mula(x_5, y_3, z_23);
                
                z_0 = vec_mula(x_6, y_4, z_0);
                z_1 = vec_mula(x_6, y_5, z_1);
                z_2 = vec_mula(x_6, y_6, z_2);
                z_3 = vec_mula(x_6, y_7, z_3);

                z_4 = vec_mula(x_7, y_4, z_4);
                z_5 = vec_mula(x_7, y_5, z_5);
                z_6 = vec_mula(x_7, y_6, z_6);
                z_7 = vec_mula(x_7, y_7, z_7);

                z_8 = vec_mula(x_8, y_4, z_8);
                z_9 = vec_mula(x_8, y_5, z_9);
                z_10 = vec_mula(x_8, y_6, z_10);
                z_11 = vec_mula(x_8, y_7, z_11);

                z_12 = vec_mula(x_9, y_4, z_12);
                z_13 = vec_mula(x_9, y_5, z_13);
                z_14 = vec_mula(x_9, y_6, z_14);
                z_15 = vec_mula(x_9, y_7, z_15);

                z_16 = vec_mula(x_10, y_4, z_16);
                z_17 = vec_mula(x_10, y_5, z_17);
                z_18 = vec_mula(x_10, y_6, z_18);
                z_19 = vec_mula(x_10, y_7, z_19);

                z_20 = vec_mula(x_11, y_4, z_20);
                z_21 = vec_mula(x_11, y_5, z_21);
                z_22 = vec_mula(x_11, y_6, z_22);
                z_23 = vec_mula(x_11, y_7, z_23);

                x_0 = vec_fexts32l(x[locx]);
                x_1 = vec_fexts32l(x[locx + mid_step]);
                x_2 = vec_fexts32l(x[locx + mid_step * 2]);
                x_3 = vec_fexts32l(x[locx + mid_step * 3]);
                x_4 = vec_fexts32l(x[locx + mid_step * 4]);
                x_5 = vec_fexts32l(x[locx + mid_step * 5]);
                x_6 = vec_fexts32l(x[locx + 1]);
                x_7 = vec_fexts32l(x[locx + mid_step + 1]);
                x_8 = vec_fexts32l(x[locx + mid_step * 2 + 1]);
                x_9 = vec_fexts32l(x[locx + mid_step * 3 + 1]);
                x_10 = vec_fexts32l(x[locx + mid_step * 4 + 1]);
                x_11 = vec_fexts32l(x[locx + mid_step * 5 + 1]);
                    
                y_0 = vec_ld(0, y + locy);
                y_1 = vec_ld(0, y + locy + 1);
                y_2 = vec_ld(0, y + locy + 2);
                y_3 = vec_ld(0, y + locy + 3);
                y_4 = vec_ld(0, y + locy + inner / 32);
                y_5 = vec_ld(0, y + locy + inner / 32 + 1);
                y_6 = vec_ld(0, y + locy + inner / 32 + 2);
                y_7 = vec_ld(0, y + locy + inner / 32 + 3);
            }
            {
                z[tmp++] = z_0;
                z[tmp++] = z_1;
                z[tmp++] = z_2;
                z[tmp++] = z_3;
                tmp += tmp_step;
                z[tmp++] = z_4;
                z[tmp++] = z_5;
                z[tmp++] = z_6;
                z[tmp++] = z_7;
                tmp += tmp_step;
                z[tmp++] = z_8;
                z[tmp++] = z_9;
                z[tmp++] = z_10;
                z[tmp++] = z_11;
                tmp += tmp_step;
                z[tmp++] = z_12;
                z[tmp++] = z_13;
                z[tmp++] = z_14;
                z[tmp++] = z_15;
                tmp += tmp_step;
                z[tmp++] = z_16;
                z[tmp++] = z_17;
                z[tmp++] = z_18;
                z[tmp++] = z_19;
                tmp += tmp_step;
                z[tmp++] = z_20;
                z[tmp++] = z_21;
                z[tmp++] = z_22;
                z[tmp++] = z_23;
                tmp += tmp_step;
            }
        }
    }
    outputs[0] = z;

    return task_ret_ok;
}

task_ret op_input_retention(void** inputs, void**outputs, void** attributes){
    ddq_log0("op_input_retention begin\n");
    outputs[0] = inputs[0];
    int_t retention_count = *(int_t*)inputs[1];
    int_t* current_count = (int_t*)inputs[2];
    if(++(*current_count) < retention_count){
        ddq_log0("op_input_retention task_ret_read_partial\n");
        return task_ret_read_partial;
    }
    ddq_log0("op_input_retention task_ret_ok\n");
    return task_ret_ok;
}

task_ret op_output_retention(void** inputs, void**outputs, void** attributes){
    outputs[0] = inputs[0];
    int_t retention_count = *(int_t*)inputs[1];
    int_t* current_count = (int_t*)inputs[2];
    if(++(*current_count) < retention_count){
        return task_ret_write_partial;
    }
    return task_ret_ok;
}