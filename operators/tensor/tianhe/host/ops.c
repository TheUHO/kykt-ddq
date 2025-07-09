#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "task_types.h"
#include "basic_types.h"

#include "ops.h"
#include "hthread_host.h"

task_ret op_iter_blk_raw(void** inputs, void** outputs){
    // printf("op_iter_blk_raw begin\n");
    int_t* size_array = ACCESS_INPUTS(0, int_t*);
    int_t row_size = size_array[0];
    int_t col_size = size_array[1];
    int_t reduce_size = size_array[2];
    int_t row_blk_size = size_array[3];
    int_t col_blk_size = size_array[4];
    int_t reduce_blk_size = size_array[5];
    int_t iter_size = size_array[6];

    int_t col_blk_len = col_size/col_blk_size;

    int_t* iter_idx_ptr = ACCESS_INPUTS(1, int_t*);
    int_t iter_idx = *iter_idx_ptr;

    int_t* idx_array = ACCESS_OUTPUTS(0, int_t);
    for(int i=0 ; i < iter_size; i++){
        idx_array[i] = iter_idx++;
    }
    // printf("op_iter_blk_raw begin 1\n");
    if(iter_idx >= row_size * col_size / row_blk_size /col_blk_size){
        return task_ret_done;
    }else{
        iter_idx_ptr[0] = iter_idx;
        return task_ret_ok;
    }

}

task_ret op_imat_to_blk(void** inputs, void** outputs){
    printf("op_imat_to_blk\n");
    int_t* size_array = ACCESS_INPUTS(0, int_t*);
    int_t row_size = size_array[0];
    int_t col_size = size_array[1];
    int_t reduce_size = size_array[2];
    int_t row_blk_size = size_array[3];
    int_t col_blk_size = size_array[4];
    int_t reduce_blk_size = size_array[5];
    int_t iter_size = size_array[6];

    double* imat0, *imat1;
    imat0 = ACCESS_INPUTS(1, double*);
    imat1 = ACCESS_INPUTS(2, double*);

    double* imat0_blk, *imat1_blk;
    imat0_blk = ACCESS_OUTPUTS(0, double*);
    imat1_blk = ACCESS_OUTPUTS(1, double*);
    //matrix按块存储，且按列存储
    int_t blk_size = col_blk_size * row_blk_size;

    int_t blk_idx,i,j;

    for(blk_idx = 0; blk_idx < row_size * reduce_size / row_blk_size / reduce_blk_size; blk_idx++){
        for(int_t i = 0; i < reduce_blk_size; i++){
            for(int_t j = 0; j < row_blk_size; j++){
                int_t row_idx = blk_idx / (reduce_size/reduce_blk_size) * row_blk_size + j;
                int_t reduce_idx = blk_idx % (reduce_size/reduce_blk_size) * reduce_blk_size + i;
                // ddq_log0("row_idx:%ld, reduece_idx:%ld\n", row_idx, reduce_idx);
                imat0_blk[blk_idx * blk_size + row_blk_size * i + j] = imat0[row_idx * reduce_size + reduce_idx];
                // ddq_log0("imat0_blk[%ld]:%f, imat0[%ld]:%f\n", blk_idx * blk_size + row_blk_size * i + j, imat0_blk[blk_idx * blk_size + row_blk_size * i + j], row_idx * reduce_size + reduce_idx, imat0[row_idx * reduce_size + reduce_idx]);
            }
        }
    }

    for(blk_idx = 0; blk_idx < col_size * reduce_size / col_blk_size / reduce_blk_size; blk_idx++){
        for(int_t i = 0; i < reduce_blk_size; i++){
            for(int_t j = 0; j < col_blk_size; j++){
                int_t reduce_idx = blk_idx / (col_size/col_blk_size) * reduce_blk_size + i;
                int_t col_idx = blk_idx % (col_size/col_blk_size) * col_blk_size + j;
                imat1_blk[blk_idx * blk_size + col_blk_size * i + j] = imat1[reduce_idx * col_size + col_idx];
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

    // ddq_log0("imat0_blk:\n");
    // for(i=0; i<row_size; i++){
    //     for(j=0; j<reduce_size; j++){
    //         ddq_log0("%f ", imat0_blk[i * reduce_size + j]);
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

    // ddq_log0("imat1_blk:\n");
    // for(i=0; i<row_size; i++){
    //     for(j=0; j<reduce_size; j++){
    //         ddq_log0("%f ", imat1_blk[i * reduce_size + j]);
    //     }
    //     ddq_log0("\n");
    // }

    return task_ret_done;
    
}

task_ret op_blk_to_omat(void** inputs, void** outputs){
    int_t* size_array = ACCESS_INPUTS(0, int_t*);
    int_t row_size = size_array[0];
    int_t col_size = size_array[1];
    int_t reduce_size = size_array[2];
    int_t row_blk_size = size_array[3];
    int_t col_blk_size = size_array[4];
    int_t reduce_blk_size = size_array[5];
    int_t iter_size = size_array[6];

    int_t* iter_idx_ptr = ACCESS_INPUTS(2, int_t*);
    int_t iter_idx = *iter_idx_ptr;
    // ddq_log0("op_blk_to_omat begin %ld\n", iter_idx);
    if(iter_idx < row_size * col_size / row_blk_size /col_blk_size)
        return task_ret_again;

    double* omat_blk;
    omat_blk = ACCESS_INPUTS(1, double*);

    double* omat;
    omat = ACCESS_OUTPUTS(0, double*);

    int_t blk_size = col_blk_size * row_blk_size;

    int_t i, j, blk_idx;
    for(i = 0; i < row_size; i++){
        for(j = 0; j < col_size; j++){
            blk_idx = i / row_blk_size * col_size / col_blk_size + j / col_blk_size;
            omat[i * col_size + j] = omat_blk[blk_idx * blk_size + i % row_blk_size * col_blk_size + j % col_blk_size];
        }
    }

    // ddq_log0("omat:\n");
    // for(i=0; i<row_size; i++){
    //     for(j=0; j<reduce_size; j++){
    //         ddq_log0("%f ", omat[i * reduce_size + j]);
    //     }
    //     ddq_log0("\n");
    // }

    // ddq_log0("omat_blk:\n");
    // for(i=0; i<row_size; i++){
    //     for(j=0; j<reduce_size; j++){
    //         ddq_log0("%f ", omat_blk[i * reduce_size + j]);
    //     }
    //     ddq_log0("\n");
    // }

    return task_ret_done;
}

task_ret op_iter_finish(void** inputs, void** outputs){
    int_t* size_array = ACCESS_INPUTS(0, int_t*);
    int_t row_size = size_array[0];
    int_t col_size = size_array[1];
    int_t reduce_size = size_array[2];
    int_t row_blk_size = size_array[3];
    int_t col_blk_size = size_array[4];
    int_t reduce_blk_size = size_array[5];
    int_t iter_size = size_array[6];

    int_t* iter_idx_ptr = ACCESS_INPUTS(1, int_t*);
    int_t iter_idx = *iter_idx_ptr;
    // ddq_log0("op_iter_finish begin %ld\n", iter_idx);
    if(iter_idx >= row_size * col_size / row_blk_size /col_blk_size){
        return task_ret_done;
    }else{
        iter_idx_ptr[0] = iter_idx + 1;
        return task_ret_ok;
    }
}