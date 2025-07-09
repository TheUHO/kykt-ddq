#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "task_types.h"
#include "basic_types.h"
#include    <cblas.h> // 添加 BLAS 库头文件

task_ret op_matmul_raw(void** inputs, void** outputs, void** attributes){
    int_t row_size, col_size, reduce_size;
    double* imat0, * imat1, * omat;
    row_size = (int_t)inputs[0];
    col_size = (int_t)inputs[1];
    reduce_size = (int_t)inputs[2];

    imat0 = (void*)inputs[3];
    imat1 = (void*)inputs[4];
    omat = (void*)outputs[0];

    memset(omat, 0, sizeof(double) * row_size * col_size);

    int_t i, j, k;
    for(i=0; i<row_size; i++){
        for(j=0; j<col_size; j++){
            for(k=0; k<reduce_size; k++){
                omat[i*col_size+j] += imat0[i*reduce_size+k] * imat1[j*reduce_size+k];
            }
        }
    }
    return task_ret_ok;
}

#define BLOCK_SIZE 64

task_ret op_matmul_optimized(void** inputs, void** outputs, void** attributes){
    double* imat0 = (double*)inputs[0];
    double* imat1 = (double*)inputs[1];
    double* omat = (double*)outputs[0];

    // 使用 BLAS 的 dgemm 进行矩阵乘法
    // C = alpha * A * B + beta * C
    double alpha = 1.0;
    double beta = 1.0;
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                row_size, col_size, reduce_size,
                alpha, imat0, reduce_size, imat1, col_size,
                beta, omat, col_size);
    // static int count = 0;
    // printf("node %d op_matmul_optimized done! row_size:%d\n", count++, row_size);
    return task_ret_ok;
}