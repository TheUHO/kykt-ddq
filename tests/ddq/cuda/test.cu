#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <chrono>
//nvcc -std=c++17 -O2 -o test test.cu -lcublas
// 随机生成矩阵
void generate_random_matrix(double* matrix, int rows, int cols) {
    for (int i = 0; i < rows * cols; i++) {
        matrix[i] = (double)rand() / RAND_MAX; // 生成 [0, 1) 的随机数
    }
}

// 执行单个 CUDA 节点的计算
void execute_cuda_node(double* h_A, double* h_B, double* h_C, int m, int n, int k, cudaStream_t stream) {
    // 分配 GPU 内存
    double *d_A, *d_B, *d_C;
    cudaMalloc((void**)&d_A, m * k * sizeof(double));
    cudaMalloc((void**)&d_B, k * n * sizeof(double));
    cudaMalloc((void**)&d_C, m * n * sizeof(double));

    // 将 Host 数据拷贝到 Device
    cudaMemcpyAsync(d_A, h_A, m * k * sizeof(double), cudaMemcpyHostToDevice, stream);
    cudaMemcpyAsync(d_B, h_B, k * n * sizeof(double), cudaMemcpyHostToDevice, stream);
    cudaMemcpyAsync(d_C, h_C, m * n * sizeof(double), cudaMemcpyHostToDevice, stream);

    // 创建 cuBLAS 句柄并绑定流
    cublasHandle_t handle;
    cublasCreate(&handle);
    cublasSetStream(handle, stream);

    // 设置矩阵乘法参数
    double alpha = 1.0;
    double beta = 1.0;

    // 执行矩阵乘法 C = alpha * A * B + beta * C
    cublasDgemm(handle,
                CUBLAS_OP_N, CUBLAS_OP_N, // 不转置 A 和 B
                n, m, k,                  // 矩阵维度
                &alpha,                   // alpha
                d_B, n,                   // B 和其 leading dimension
                d_A, k,                   // A 和其 leading dimension
                &beta,                    // beta
                d_C, n);                  // C 和其 leading dimension

    // 将结果从 Device 拷贝回 Host
    cudaMemcpyAsync(h_C, d_C, m * n * sizeof(double), cudaMemcpyDeviceToHost, stream);

    // 同步流，确保操作完成
    cudaStreamSynchronize(stream);

    // 释放 GPU 内存和 cuBLAS 句柄
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);
    cublasDestroy(handle);
}

// 执行完全并行的 CUDA 节点计算
void execute_parallel_nodes(int num_nodes, double** matrices, int m, int n, int k) {
    // 为每个节点分配一个 CUDA 流
    cudaStream_t* streams = (cudaStream_t*)malloc(num_nodes * sizeof(cudaStream_t));
    for (int i = 0; i < num_nodes; i++) {
        cudaStreamCreate(&streams[i]);
    }

    // 记录总执行时间
    auto start_time = std::chrono::high_resolution_clock::now();

    // 并行执行每个节点的计算
    for (int i = 0; i < num_nodes; i++) {
        // printf("执行节点 %d 的计算\n", i);

        // 获取当前节点的输入和输出矩阵
        double* h_A = matrices[i * 3];       // 输入矩阵 A
        double* h_B = matrices[i * 3 + 1];   // 输入矩阵 B
        double* h_C = matrices[i * 3 + 2];   // 输出矩阵 C

        // 执行 CUDA 节点计算
        execute_cuda_node(h_A, h_B, h_C, m, n, k, streams[i]);
    }

    // 等待所有流完成
    for (int i = 0; i < num_nodes; i++) {
        cudaStreamSynchronize(streams[i]);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_elapsed_time = end_time - start_time;

    // 计算总性能（GFLOPS）
    double flops_per_task = 2.0 * m * n * k;
    double total_flops = num_nodes * flops_per_task;
    double gflops = total_flops / (total_elapsed_time.count() * 1e9);

    printf("总执行时间: %.6f 秒\n", total_elapsed_time.count());
    printf("总性能: %.2f GFLOPS\n", gflops);

    // 销毁 CUDA 流
    for (int i = 0; i < num_nodes; i++) {
        cudaStreamDestroy(streams[i]);
    }
    free(streams);
}

int main() {
    // 初始化随机数种子
    srand(time(NULL));

    // 矩阵维度
    const int m = 4096; // A 的行数和 C 的行数
    const int n = 4096; // B 的列数和 C 的列数
    const int k = 4096; // A 的列数和 B 的行数

    // 节点数量
    int num_nodes = 10;

    // 分配矩阵数据
    double** matrices = (double**)malloc(num_nodes * 3 * sizeof(double*));
    for (int i = 0; i < num_nodes * 3; i++) {
        matrices[i] = (double*)malloc(m * n * sizeof(double));
        generate_random_matrix(matrices[i], m, n);
    }

    // 执行完全并行的节点计算
    execute_parallel_nodes(num_nodes, matrices, m, n, k);

    // 打印结果（仅用于调试）
    // for (int i = 0; i < num_nodes; i++) {
    //     char name[32];
    //     snprintf(name, sizeof(name), "节点 %d 的输出矩阵", i);
    //     print_matrix(name, matrices[i * 3 + 2], m, n);
    // }

    // 释放内存
    for (int i = 0; i < num_nodes * 3; i++) {
        free(matrices[i]);
    }
    free(matrices);

    return 0;
}