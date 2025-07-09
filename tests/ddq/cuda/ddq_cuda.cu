#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include <string.h>

#include	"ddq.h"
#include	"oplib.h"
#include    "error.h"

#include    "std/std_ops/std_ops.h"
#include    "task_types.h"
#include "dag.h"
#include "matrix.h"
#include "ddq_dag.h"

#include <cuda_runtime.h>
#include <cublas_v2.h>
#include "task_types.h"

#include "ddq_cuda.h"

// #define SIZE 256
extern int row_size;
extern int col_size;
extern int reduce_size;

// 随机生成矩阵
void generate_random_matrix(double* matrix, int rows, int cols) {
    for (int i = 0; i < rows * cols; i++) {
        matrix[i] = (double)rand() / RAND_MAX; // 生成 [0, 1) 的随机数
    }
}

void* cuMatrix_new(){
    double* res;
    cudaMalloc((void**)&res, row_size * reduce_size * sizeof(double));
    return res;
}
void cuMatrix_delete(void* p){
    cudaFree(p);
}

task_ret op_cu_matmul_optimized(void** inputs, void** outputs, void** attributes, void* stream) {
    // 输入矩阵和输出矩阵
    
    double* imat0 = (double*)inputs[0]; // 输入矩阵 A
    double* imat1 = (double*)inputs[1]; // 输入矩阵 B
    double* omat = (double*)outputs[0]; // 输出矩阵 C

    double* d_imat0 = (double*)inputs[2];
    double* d_imat1 = (double*)inputs[3];
    double* d_omat = (double*)outputs[1];

    // 分配 GPU 内存
    // double *d_imat0, *d_imat1, *d_omat;
    // cudaMalloc((void**)&d_imat0, row_size * reduce_size * sizeof(double));
    // cudaMalloc((void**)&d_imat1, reduce_size * col_size * sizeof(double));
    // cudaMalloc((void**)&d_omat, row_size * col_size * sizeof(double));

    // 将输入矩阵从 Host 拷贝到 Device
    cudaMemcpyAsync(d_imat0, imat0,row_size * reduce_size * sizeof(double), cudaMemcpyHostToDevice, (cudaStream_t)stream);
    cudaMemcpyAsync(d_imat1, imat1, reduce_size * col_size * sizeof(double), cudaMemcpyHostToDevice, (cudaStream_t)stream);
    cudaMemcpyAsync(d_omat, omat, row_size * col_size * sizeof(double), cudaMemcpyHostToDevice, (cudaStream_t)stream);

    // 创建 cuBLAS 句柄并绑定流
    cublasHandle_t handle;
    cublasCreate(&handle);
    cublasSetStream(handle, (cudaStream_t)stream);

    // 执行矩阵乘法 C = alpha * A * B + beta * C
    double alpha = 1.0;
    double beta = 1.0;
    cublasDgemm(handle,
                CUBLAS_OP_N, CUBLAS_OP_N, // 不转置 A 和 B
                col_size, row_size, reduce_size,                  // 矩阵维度
                &alpha,                   // alpha
                d_imat1, col_size,               // B 和其 leading dimension
                d_imat0, reduce_size,               // A 和其 leading dimension
                &beta,                    // beta
                d_omat, col_size);               // C 和其 leading dimension

    // 将结果从 Device 拷贝回 Host
    cudaMemcpyAsync(omat, d_omat, row_size * col_size * sizeof(double), cudaMemcpyDeviceToHost, (cudaStream_t)stream);
    
    // 同步流，确保操作完成
    // cudaStreamSynchronize((cudaStream_t)stream);

    // 释放 GPU 内存和 cuBLAS 句柄
    // cudaFree(d_imat0);
    // cudaFree(d_imat1);
    // cudaFree(d_omat);
    // cublasDestroy(handle);
        // printf("op_cu_matmul_optimized: %p %p %p %p\n", imat0, imat1, omat, stream);
    return task_ret_ok;
}

ddq_ring matmul_ring(double* imat0, double* imat1, double* omat) {

    ddq_ring ring = ddq_new(NULL, 0, 0);
    
    obj f_matmul;
    f_matmul = obj_import(ring, (void*)op_cu_matmul_optimized, NULL, obj_prop_ready);
    
    obj obj_imat0, obj_imat1, obj_omat;
    obj_imat0 = obj_import(ring, imat0, NULL, obj_prop_ready | obj_prop_consumable);
    obj_imat1 = obj_import(ring, imat1, NULL, obj_prop_ready | obj_prop_consumable);
    obj_omat = obj_import(ring, omat, NULL, obj_prop_consumable);
    obj obj_d_imat0, obj_d_imat1, obj_d_omat;
    obj_d_imat0 = obj_new(ring, cuMatrix_new, cuMatrix_delete, obj_prop_consumable);
    obj_d_imat1 = obj_new(ring, cuMatrix_new, cuMatrix_delete, obj_prop_consumable);
    obj_d_omat = obj_new(ring, cuMatrix_new, cuMatrix_delete, obj_prop_consumable);

    ddq_op matmul = ddq_spawn(ring, processor_cuda, 4, 2);
    ddq_add_f(matmul, f_matmul);
    ddq_add_inputs(matmul, 0, obj_imat0);
    ddq_add_inputs(matmul, 1, obj_imat1);
    ddq_add_inputs(matmul, 2, obj_d_imat0);
    ddq_add_inputs(matmul, 3, obj_d_imat1);
    ddq_add_outputs(matmul, 0, obj_omat);
    ddq_add_outputs(matmul, 1, obj_d_omat);

    return ring;
}

// 根据 DAG 自动生成 op_cu_matmul_optimized 算子图
ddq_ring generate_ddq_from_dag_cuda(int num_nodes, const NodeInfo* nodes, double** matrices) {
    printf("生成基于 CUDA 的 DAG 算子图\n");

    // 创建一个新的 ddq_ring
    ddq_ring ring = ddq_new(NULL, 0, 0);

    // 导入 op_cu_matmul_optimized 算子
    obj matmul = obj_import(ring, (void*)op_cu_matmul_optimized, NULL, obj_prop_ready);

    // 创建 op 和输入输出对象
    ddq_op* ops = (ddq_op*)malloc(num_nodes * sizeof(ddq_op));
    int* inputs = (int*)malloc(num_nodes * sizeof(int));
    int nothing_inputs = 0;

    for (int i = 0; i < num_nodes; i++) {
        // 为每个节点创建一个 CUDA 处理器的 op
        ops[i] = ddq_spawn(ring, processor_cuda, 4 + nodes[i].input_count, 2);

        // 创建输入和输出对象
        obj imat0 = obj_import(ring, matrices[i * 3], NULL, obj_prop_ready | obj_prop_consumable);
        obj imat1 = obj_import(ring, matrices[i * 3 + 1], NULL, obj_prop_ready | obj_prop_consumable);
        obj omat = obj_import(ring, matrices[i * 3 + 2], NULL, obj_prop_consumable);

        // 创建 GPU 上的矩阵对象
        obj d_imat0 = obj_new(ring, cuMatrix_new, cuMatrix_delete, obj_prop_consumable);
        obj d_imat1 = obj_new(ring, cuMatrix_new, cuMatrix_delete, obj_prop_consumable);
        obj d_omat = obj_new(ring, cuMatrix_new, cuMatrix_delete, obj_prop_consumable);

        // 添加算子函数和输入输出
        ddq_add_f(ops[i], matmul);
        ddq_add_inputs(ops[i], 0, imat0);
        ddq_add_inputs(ops[i], 1, imat1);
        ddq_add_inputs(ops[i], 2, d_imat0);
        ddq_add_inputs(ops[i], 3, d_imat1);
        ddq_add_outputs(ops[i], 0, omat);
        ddq_add_outputs(ops[i], 1, d_omat);

        // inputs[i] = 4;

        // 如果节点没有输出，则计入 nothing_inputs
        // if (nodes[i].output_count == 0) {
        //     nothing_inputs++;
        // }
    }

    // 创建一个 "nothing" 算子，用于处理没有输出的节点
    // obj nothing = obj_import(ring, op_nothing, NULL, obj_prop_ready);
    // ddq_op nothing_op = ddq_spawn(ring, processor_cuda, nothing_inputs, 0);
    // ddq_add_f(nothing_op, nothing);

    // 连接算子之间的依赖关系
    for (int i = 0; i < num_nodes; i++) {
        // if (nodes[i].output_count == 0) {
        //     ddq_add_inputs(nothing_op, --nothing_inputs, ops[i]->outputs[0]);
        // }
        for (int j = 0; j < nodes[i].output_count; j++) {
            int to_node = nodes[i].outputs[j];
            ddq_add_inputs(ops[to_node], inputs[to_node]++, ops[i]->outputs[0]);
        }
    }

    // 释放临时分配的内存
    free(ops);
    free(inputs);

    return ring;
}

// int main(){
//     // 初始化随机数种子
//     srand(time(NULL));

//     double* imat0 = (double*)malloc(row_size * reduce_size * sizeof(double));
//     double* imat1 = (double*)malloc(reduce_size * col_size * sizeof(double));
//     double* omat = (double*)malloc(row_size * col_size * sizeof(double));
//     // 随机生成矩阵数据
//     generate_random_matrix(imat0, row_size, reduce_size);
//     generate_random_matrix(imat1, reduce_size, col_size);

//     // 初始化输出矩阵
//     for (int i = 0; i < row_size * col_size; i++) {
//         omat[i] = 0;
//     }
//     ddq_ring ring = matmul_ring(imat0, imat1, omat);
//     ddq_loop_init();
//     ddq_update(ring);
//     ddq_loop(ring, 0);
//     ddq_delete(ring);
    
//     //检查结果是否正确
//     double* check = (double*)malloc(row_size * col_size * sizeof(double));
//     for (int i = 0; i < row_size; i++) {
//         for (int j = 0; j < col_size; j++) {
//             check[i * col_size + j] = 0;
//             for (int k = 0; k < reduce_size; k++) {
//                 check[i * col_size + j] += imat0[i * reduce_size + k] * imat1[k * col_size + j];
//             }
//         }
//     }
//     for (int i = 0; i < row_size * col_size; i++) {
//         if (fabs(omat[i] - check[i]) > 1e-6) {
//             printf("Error: %f != %f\n", omat[i], check[i]);
//             break;
//         }
//     }
//     printf("Result is correct!\n");
//     free(check);
//     free(imat0);
//     free(imat1);
//     free(omat);
//     return 0;
// }

// int main() {
//     int num_nodes = 10;
//     int num_edges = 20;

//     // 生成随机 DAG
//     NodeInfo* nodes = generate_random_dag(num_nodes, num_edges);

//     // 分配矩阵数据
//     double** matrices = (double**)malloc(num_nodes * 3 * sizeof(double*));
//     for (int i = 0; i < num_nodes * 3; i++) {
//         matrices[i] = (double*)malloc(SIZE * SIZE * sizeof(double));
//         generate_random_matrix(matrices[i], SIZE, SIZE);
//     }

//     // 生成基于 CUDA 的算子图
//     ddq_ring ring = generate_ddq_from_dag_cuda(num_nodes, nodes, matrices);

//     // 执行任务流
//     ddq_loop_init();
//     ddq_update(ring);
//     ddq_loop(ring, 0);
//     ddq_delete(ring);

//     // 检查结果（可选）
//     // ...

//     // 释放内存
//     for (int i = 0; i < num_nodes * 3; i++) {
//         free(matrices[i]);
//     }
//     free(matrices);
//     free_node_info(nodes, num_nodes);

//     return 0;
// }