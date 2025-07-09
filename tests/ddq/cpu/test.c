#include <omp.h> // 添加 OpenMP 支持
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "ddq.h"
#include "oplib.h"
#include "error.h"
#include "std/std_ops/std_ops.h"
#include "dag.h"
#include "matrix.h"
#include "ddq_dag.h"
#include "../cuda/ddq_cuda.h"
#include <cblas.h>
#include <openblas_config.h>
#define __USE_GNU
#include <sched.h>
#define SIZE 256
int row_size = SIZE;
int col_size = SIZE;
int reduce_size = SIZE;

// 获取物理核心数
int get_physical_core_count() {
    return sysconf(_SC_NPROCESSORS_ONLN); // 返回可用的物理核心数
}

// 测试不使用 DDQ 的单线程矩阵乘法
void test_base(int num_nodes, double** matrices, const NodeInfo* node_info) {
    struct timespec start, end;
    double alpha = 1.0, beta = 1.0;

    openblas_set_num_threads(1);
    printf("OpenBLAS 使用的线程数: %d\n", openblas_get_num_threads());
    clear_cpu_cache();
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 初始化节点状态数组
    int* node_status = (int*)calloc(num_nodes, sizeof(int)); // 0: 未执行, 1: 正在执行, 2: 已完成
    int completed_nodes = 0;

    while (completed_nodes < num_nodes) {
        for (int i = 0; i < num_nodes; i++) {
            if (node_status[i] == 2) {
                continue; // 已完成的节点跳过
            }

            // 检查当前节点的所有输入节点是否已完成
            int ready = 1;
            for (int j = 0; j < node_info[i].input_count; j++) {
                int input_node = node_info[i].inputs[j];
                if (node_status[input_node] != 2) {
                    ready = 0; // 如果有未完成的输入节点，则当前节点不能执行
                    break;
                }
            }

            if (!ready) {
                continue; // 跳过当前节点，检查下一个节点
            }

            // 执行当前节点的 GEMM 操作
            node_status[i] = 1; // 标记为正在执行
            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                        row_size, col_size, reduce_size,
                        alpha, matrices[i * 3], reduce_size, matrices[i * 3 + 1], col_size,
                        beta, matrices[i * 3 + 2], col_size);
            node_status[i] = 2; // 标记为已完成
            completed_nodes++;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double gflops = (2.0 * row_size * col_size * reduce_size * num_nodes) / (elapsed_time * 1e9);
    printf("Base单线程 测试运行时间: %.6f 秒\n", elapsed_time);
    printf("性能: %.2f GFLOPS\n", gflops);
    fflush(stdout);

    free(node_status); // 释放节点状态数组
}

// 测试不使用 DDQ 的矩阵乘法
void test_base_parallel(int num_nodes, double** matrices, const NodeInfo* node_info) {
    struct timespec start, end;
    double alpha = 1.0, beta = 1.0;

    openblas_set_num_threads(23);
    printf("OpenBLAS 使用的线程数: %d\n", openblas_get_num_threads());
    clear_cpu_cache();
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 初始化节点状态数组
    int* node_status = (int*)calloc(num_nodes, sizeof(int)); // 0: 未执行, 1: 正在执行, 2: 已完成
    int completed_nodes = 0;

    while (completed_nodes < num_nodes) {
        for (int i = 0; i < num_nodes; i++) {
            if (node_status[i] == 2) {
                continue; // 已完成的节点跳过
            }

            // 检查当前节点的所有输入节点是否已完成
            int ready = 1;
            for (int j = 0; j < node_info[i].input_count; j++) {
                int input_node = node_info[i].inputs[j];
                if (node_status[input_node] != 2) {
                    ready = 0; // 如果有未完成的输入节点，则当前节点不能执行
                    break;
                }
            }

            if (!ready) {
                continue; // 跳过当前节点，检查下一个节点
            }

            // 执行当前节点的 GEMM 操作
            node_status[i] = 1; // 标记为正在执行
            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                        row_size, col_size, reduce_size,
                        alpha, matrices[i * 3], reduce_size, matrices[i * 3 + 1], col_size,
                        beta, matrices[i * 3 + 2], col_size);
            node_status[i] = 2; // 标记为已完成
            completed_nodes++;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double gflops = (2.0 * row_size * col_size * reduce_size * num_nodes) / (elapsed_time * 1e9);
    printf("Base(openblas内部多线程) 测试运行时间: %.6f 秒\n", elapsed_time);
    printf("性能: %.2f GFLOPS\n", gflops);
    fflush(stdout);

    free(node_status); // 释放节点状态数组
}

// 测试不使用 DDQ 的矩阵乘法（并行版本）
void test_base_openmp(int num_nodes, double** matrices, const NodeInfo* node_info) {
    struct timespec start, end;
    double alpha = 1.0, beta = 1.0;

    // 获取物理核心数
    int num_cores = get_physical_core_count() - 1;
    printf("物理核心数: %d\n", num_cores);

    // 设置 OpenBLAS 使用单线程（避免 OpenBLAS 内部多线程与 OpenMP 冲突）
    openblas_set_num_threads(1);
    // printf("OpenBLAS 使用的线程数: %d\n", openblas_get_num_threads());

    omp_set_num_threads(num_cores); // 设置 OpenMP 使用的线程数
    int num_threads = omp_get_max_threads();
    printf("openmp使用的线程数: %d\n", num_threads);

    // 初始化节点状态数组
    int* node_status = (int*)calloc(num_nodes, sizeof(int)); // 0: 未执行, 1: 正在执行, 2: 已完成
    int completed_nodes = 0;

    // 清空 CPU 缓存
    clear_cpu_cache();

    // 记录开始时间
    clock_gettime(CLOCK_MONOTONIC, &start);

    
    while (completed_nodes < num_nodes) {
        #pragma omp parallel for num_threads(num_cores) schedule(dynamic)
        for (int i = 0; i < num_nodes; i++) {
            // int thread_id = omp_get_thread_num();
            // printf("线程 %d 正在处理节点 %d\n", thread_id, i);
            if (node_status[i] == 2) {
                continue; // 已完成的节点跳过
            }

            // 检查当前节点的所有输入节点是否已完成
            int ready = 1;
            for (int j = 0; j < node_info[i].input_count; j++) {
                int input_node = node_info[i].inputs[j];
                if (node_status[input_node] != 2) {
                    ready = 0; // 如果有未完成的输入节点，则当前节点不能执行
                    break;
                }
            }

            if (!ready) {
                continue; // 跳过当前节点，检查下一个节点
            }

            // 执行当前节点的 GEMM 操作
            node_status[i] = 1; // 标记为正在执行
            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                        row_size, col_size, reduce_size,
                        alpha, matrices[i * 3], reduce_size, matrices[i * 3 + 1], col_size,
                        beta, matrices[i * 3 + 2], col_size);
            node_status[i] = 2; // 标记为已完成

            #pragma omp atomic
            completed_nodes++; // 原子操作，更新已完成节点数
        }
    }

    // 记录结束时间
    clock_gettime(CLOCK_MONOTONIC, &end);

    // 计算运行时间
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // 计算 GFLOPS
    double gflops = (2.0 * row_size * col_size * reduce_size * num_nodes) / (elapsed_time * 1e9);

    // 输出结果
    printf("Base openmp static并行测试运行时间: %.6f 秒\n", elapsed_time);
    printf("性能: %.2f GFLOPS\n", gflops);
    fflush(stdout);

    free(node_status);
}

// 测试不使用 DDQ 的矩阵乘法（并行版本）
void test_base_openmp_2(int num_nodes, double** matrices, const NodeInfo* node_info) {
    struct timespec start, end;
    double alpha = 1.0, beta = 1.0;

    // 获取物理核心数
    int num_cores = get_physical_core_count() - 1;
    printf("物理核心数: %d\n", num_cores);

    // 设置 OpenBLAS 使用单线程（避免 OpenBLAS 内部多线程与 OpenMP 冲突）
    openblas_set_num_threads(1);
    // printf("OpenBLAS 使用的线程数: %d\n", openblas_get_num_threads());

    omp_set_num_threads(num_cores); // 设置 OpenMP 使用的线程数
    int num_threads = omp_get_max_threads();
    printf("openmp使用的线程数: %d\n", num_threads);

    // 初始化节点状态数组
    // 记录开始时间
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 提前绑定线程到核心
    // #pragma omp parallel num_threads(num_cores)
    // {
    //     int thread_id = omp_get_thread_num();

    //     // 设置线程绑定到特定的 CPU 核心
    //     cpu_set_t cpuset;
    //     CPU_ZERO(&cpuset);
    //     CPU_SET(thread_id % num_cores, &cpuset); // 绑定到物理核心

    //     if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
    //         perror("sched_setaffinity 失败");
    //     }
    // }
    int* node_status = (int*)calloc(num_nodes, sizeof(int)); // 0: 未执行, 1: 正在执行, 2: 已完成
    int completed_nodes = 0;
    int* ready_nodes = (int*)malloc(num_nodes * sizeof(int));
    while (completed_nodes < num_nodes) {
        // 收集本批次可执行的节点
        int ready_count = 0;

        // 查找所有就绪节点
        for (int i = 0; i < num_nodes; i++) {
            if (node_status[i] != 0) continue;
            
            int ready = 1;
            for (int j = 0; j < node_info[i].input_count; j++) {
                if (node_status[node_info[i].inputs[j]] != 2) {
                    ready = 0;
                    break;
                }
            }
            if (ready) {
                ready_nodes[ready_count++] = i;
                node_status[i] = 1; // 标记为执行中
            }
        }

        // 并行执行本批次的节点
        #pragma omp parallel for schedule(dynamic)
        for (int k = 0; k < ready_count; k++) {
            int i = ready_nodes[k];
            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                        row_size, col_size, reduce_size,
                        alpha, matrices[i * 3], reduce_size,
                        matrices[i * 3 + 1], col_size,
                        beta, matrices[i * 3 + 2], col_size);
        }

        // 标记完成
        #pragma omp parallel for
        for (int k = 0; k < ready_count; k++) {
            int i = ready_nodes[k];
            node_status[i] = 2;
        }
        completed_nodes += ready_count;
    }
    free(ready_nodes);
    free(node_status);

    // 记录结束时间
    clock_gettime(CLOCK_MONOTONIC, &end);

    // 计算运行时间
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // 计算 GFLOPS
    double gflops = (2.0 * row_size * col_size * reduce_size * num_nodes) / (elapsed_time * 1e9);

    // 输出结果
    printf("Base openmp_2 测试运行时间: %.6f 秒\n", elapsed_time);
    printf("性能: %.2f GFLOPS\n", gflops);
    fflush(stdout);
}

// 测试使用 DDQ 的 ddq_loop
// void test_ddq_loop(int num_nodes, const NodeInfo* node_info, double** matrices) {
//     struct timespec start, end;

//     openblas_set_num_threads(1);
//     ddq_ring ring = generate_ddq_from_dag_optimized(num_nodes, node_info, matrices);
//     if (!ring) {
//         printf("生成 DDQ 失败。\n");
//         return;
//     }
//     ddq_update(ring);

//     // ddq_loop_init();
//     clear_cpu_cache();
//     clock_gettime(CLOCK_MONOTONIC, &start);
//     ddq_loop(ring, 0);
//     clock_gettime(CLOCK_MONOTONIC, &end);

//     ddq_delete(ring);
//     double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
//     printf("ddq_loop 测试运行时间: %.6f 秒\n", elapsed_time);
//     double gflops = (2.0 * row_size * col_size * reduce_size * num_nodes) / (elapsed_time * 1e9);
//     printf("性能: %.2f GFLOPS\n", gflops);
//     fflush(stdout);
// }

// 测试使用 DDQ 的 ddq_loop
void test_ddq_loop_pthread_pool(int num_nodes, const NodeInfo* node_info, double** matrices) {
    struct timespec start, end;

    openblas_set_num_threads(1);
    ddq_ring ring = generate_ddq_from_dag_pthread_pool(num_nodes, node_info, matrices);
    if (!ring) {
        printf("生成 DDQ 失败。\n");
        return;
    }
    ddq_update(ring);
    //输出ddq占用空间
    printf("ddq占用空间: %ld / %ld\n", ring->mem->bufsize, ring->mem->size);

    // ddq_loop_init();
    clear_cpu_cache();
    clock_gettime(CLOCK_MONOTONIC, &start);
    ddq_loop(ring, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);

    ddq_delete(ring);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("ddq_loop pthread_pool 测试运行时间: %.6f 秒\n", elapsed_time);
    double gflops = (2.0 * row_size * col_size * reduce_size * num_nodes) / (elapsed_time * 1e9);
    printf("性能: %.2f GFLOPS\n", gflops);
    fflush(stdout);
}

// 测试使用 DDQ 的 ddq_loop_old
// void test_ddq_loop_old(int num_nodes, const NodeInfo* node_info, double** matrices) {
//     struct timespec start, end;

//     openblas_set_num_threads(1);
//     ddq_ring ring = generate_ddq_from_dag_pthread_pool(num_nodes, node_info, matrices);
//     if (!ring) {
//         printf("生成 DDQ 失败。\n");
//         return;
//     }
//     ddq_update(ring);

//     // ddq_loop_init();
//     clear_cpu_cache();
//     clock_gettime(CLOCK_MONOTONIC, &start);
//     ddq_loop_old(ring, 0);
//     clock_gettime(CLOCK_MONOTONIC, &end);

//     ddq_delete(ring);
//     double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
//     printf("ddq_loop_old 测试运行时间: %.6f 秒\n", elapsed_time);
//     double gflops = (2.0 * row_size * col_size * reduce_size * num_nodes) / (elapsed_time * 1e9);
//     printf("性能: %.2f GFLOPS\n", gflops);
//     fflush(stdout);
// }

void test_ddq_loop_gpu(int num_nodes, const NodeInfo* node_info, double** matrices){
    struct timespec start, end; 
    // 生成基于 CUDA 的算子图
     ddq_ring ring = generate_ddq_from_dag_cuda(num_nodes, node_info, matrices);

     // 执行任务流
     ddq_loop_init();
     ddq_update(ring);
     clear_cpu_cache();
    clock_gettime(CLOCK_MONOTONIC, &start);
     ddq_loop(ring, 0);
     clock_gettime(CLOCK_MONOTONIC, &end);
     ddq_delete(ring);
     double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
     printf("ddq_loop_gpu 测试运行时间: %.6f 秒\n", elapsed_time);
     double gflops = (2.0 * row_size * col_size * reduce_size * num_nodes) / (elapsed_time * 1e9);
     printf("性能: %.2f GFLOPS\n", gflops);
     fflush(stdout);
}

// 主函数
int main() {
    int num_nodes = 500;//cuda有bug
    int num_edges = 0; 
    ddq_loop_init();

    printf(">>>>>>>>>>>>>>>>>>>>>>\n");
    printf("测试开始...\n");
    // 输出矩阵规模、节点数和边数
    printf("矩阵规模: %d x %d\n", row_size, col_size);
    printf("节点数: %d\n", num_nodes);
    printf("边数: %d\n", num_edges);

    // 生成矩阵数组
    double** matrices = generate_matrix_array(num_nodes * 3, row_size, col_size);
    const char* filename = "matrices.bin";
    write_matrix_array_to_file(filename, matrices, num_nodes * 3, row_size, col_size);

    // 生成 DAG
    // const NodeInfo* node_info = generate_random_dag(num_nodes, num_edges);
    const NodeInfo* node_info = generate_linear_dag(num_nodes);
    if (!node_info || !matrices) {
        printf("生成 DAG 或矩阵数组失败。\n");
        return -1;
    }

    // // 从文件中读取矩阵数组
    int read_num_matrices, read_rows, read_cols;

    // 测试 Base
    test_base(num_nodes, matrices, node_info);
    write_matrix_array_to_file("base.bin", matrices, num_nodes * 3, row_size, col_size);
    free_matrix_array(matrices, num_nodes * 3);

    matrices = read_matrix_array_from_file(filename, &read_num_matrices, &read_rows, &read_cols);
    test_base_parallel(num_nodes, matrices, node_info);
    write_matrix_array_to_file("base_parallel.bin", matrices, num_nodes * 3, row_size, col_size);
    free_matrix_array(matrices, num_nodes * 3);

    matrices = read_matrix_array_from_file(filename, &read_num_matrices, &read_rows, &read_cols);
    test_base_openmp(num_nodes, matrices, node_info);
    write_matrix_array_to_file("openmp.bin", matrices, num_nodes * 3, row_size, col_size);
    free_matrix_array(matrices, num_nodes * 3);

    matrices = read_matrix_array_from_file(filename, &read_num_matrices, &read_rows, &read_cols);
    test_base_openmp_2(num_nodes, matrices, node_info);
    write_matrix_array_to_file("openmp_2.bin", matrices, num_nodes * 3, row_size, col_size);
    free_matrix_array(matrices, num_nodes * 3);

    //测试 ddq_loop_pthread_pool
    matrices = read_matrix_array_from_file(filename, &read_num_matrices, &read_rows, &read_cols);
    test_ddq_loop_pthread_pool(num_nodes, node_info, matrices);
    write_matrix_array_to_file("ddq_loop_pthread_pool.bin", matrices, num_nodes * 3, row_size, col_size);
    free_matrix_array(matrices, num_nodes * 3);

    // 测试 ddq_loop
    // matrices = read_matrix_array_from_file(filename, &read_num_matrices, &read_rows, &read_cols);
    // test_ddq_loop_gpu(num_nodes, node_info, matrices);
    // write_matrix_array_to_file("ddq_loop_gpu.bin", matrices, num_nodes * 3, row_size, col_size);
    // free_matrix_array(matrices, num_nodes * 3);

    //测试 ddq_loop_old
    // matrices = read_matrix_array_from_file(filename, &read_num_matrices, &read_rows, &read_cols);
    // test_ddq_loop_old(num_nodes, node_info, matrices);
    // write_matrix_array_to_file("ddq_loop_old.bin", matrices, num_nodes * 3, row_size, col_size);
    // free_matrix_array(matrices, num_nodes * 3);

    // 释放资源
    free_node_info(node_info, num_nodes);

    //检验正确性
    // double** origin_matrices = read_matrix_array_from_file("matrices.bin", &read_num_matrices, &read_rows, &read_cols);
    matrices = read_matrix_array_from_file("base.bin", &read_num_matrices, &read_rows, &read_cols);
    double** matrices_ddq_loop = read_matrix_array_from_file("ddq_loop_pthread_pool.bin", &read_num_matrices, &read_rows, &read_cols);
    // double** matrices_ddq_loop_gpu = read_matrix_array_from_file("ddq_loop_gpu.bin", &read_num_matrices, &read_rows, &read_cols);
    // double** matrices_ddq_loop_old = read_matrix_array_from_file("ddq_loop_old.bin", &read_num_matrices, &read_rows, &read_cols);
    // if (compare_matrix_arrays(matrices, origin_matrices, 3 * num_nodes, row_size, col_size)) {
    //     printf("Base 和 origin_matrices 结果一致。\n");
    // } else {
    //     printf("Base 和 origin_matrices 结果不一致。\n");
    // }
    if (compare_matrix_arrays(matrices, matrices_ddq_loop, 3 * num_nodes, row_size, col_size)) {
        printf("Base 和 ddq_loop_pthread_pool 结果一致。\n");
    } else {
        printf("Base 和 ddq_loop_pthread_pool 结果不一致。\n");
    }
    // if (compare_matrix_arrays(matrices, matrices_ddq_loop_gpu, 3 * num_nodes, row_size, col_size)) {
    //     printf("Base 和 ddq_loop_gpu 结果一致。\n");
    // } else {
    //     printf("Base 和 ddq_loop_gpu 结果不一致。\n");
    // }

    free_matrix_array(matrices, 3 * num_nodes);
    // free_matrix_array(origin_matrices, 3 * num_nodes);
    free_matrix_array(matrices_ddq_loop, 3 * num_nodes);
    // free_matrix_array(matrices_ddq_loop_old, 3 * num_nodes);

    printf("所有测试完成！\n");
    return 0;
}