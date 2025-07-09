#include <tbb/flow_graph.h>
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>
#include <chrono>
#include <vector>
#include <iostream>
#include <cblas.h>
extern "C" {
#include "dag.h"
#include "matrix.h"
}

#define SIZE 256
int row_size = SIZE;
int col_size = SIZE;
int reduce_size = SIZE;

// 矩阵乘法任务
void matrix_multiply(double* A, double* B, double* C, int row_size, int col_size, int reduce_size) {
    double alpha = 1.0, beta = 1.0;
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                row_size, col_size, reduce_size,
                alpha, A, reduce_size, B, col_size,
                beta, C, col_size);
}

// 基于 oneTBB 的任务流生成
void generate_tbb_taskflow_from_dag(int num_nodes, const NodeInfo* node_info, double** matrices) {
    tbb::flow::graph graph;
    std::vector<tbb::flow::continue_node<tbb::flow::continue_msg>> nodes(num_nodes);

    // 创建任务节点
    for (int i = 0; i < num_nodes; i++) {
        nodes[i] = tbb::flow::continue_node<tbb::flow::continue_msg>(graph, [i, matrices](const tbb::flow::continue_msg&) {
            matrix_multiply(matrices[i * 3], matrices[i * 3 + 1], matrices[i * 3 + 2], row_size, col_size, reduce_size);
            std::cout << "节点 " << i << " 的矩阵乘法完成\n";
        });
    }

    // 设置任务依赖关系
    for (int i = 0; i < num_nodes; i++) {
        for (int j = 0; j < node_info[i].input_count; j++) {
            int input_node = node_info[i].inputs[j];
            tbb::flow::make_edge(nodes[input_node], nodes[i]);
        }
    }

    // 启动任务流
    for (int i = 0; i < num_nodes; i++) {
        if (node_info[i].input_count == 0) {
            nodes[i].try_put(tbb::flow::continue_msg());
        }
    }

    graph.wait_for_all();
}

int main() {
    int num_nodes = 500;
    int num_edges = 250 * 499;

    NodeInfo* node_info = generate_random_dag(num_nodes, num_edges);
    if (node_info) {
        // 生成随机矩阵
        double** matrices = generate_matrix_array(num_nodes * 3, row_size, col_size);

        // 执行任务流并记录时间
        auto start_time = std::chrono::high_resolution_clock::now(); // 开始时间
        generate_tbb_taskflow_from_dag(num_nodes, node_info, matrices);
        auto end_time = std::chrono::high_resolution_clock::now(); // 结束时间

        // 计算执行时间
        std::chrono::duration<double> elapsed_time = end_time - start_time;
        double execution_time = elapsed_time.count(); // 执行时间（秒）

        // 计算性能（GFLOPS）
        double flops_per_task = 2.0 * row_size * col_size * reduce_size;
        double total_flops = num_nodes * flops_per_task;
        double gflops = total_flops / (execution_time * 1e9); // 转换为 GFLOPS

        // 打印结果
        printf("任务流执行时间: %.6f 秒\n", execution_time);
        printf("性能: %.2f GFLOPS\n", gflops);

        // 释放内存
        for (int i = 0; i < num_nodes * 3; i++) {
            free(matrices[i]);
        }
        free(matrices);
        free_node_info(node_info, num_nodes);
    }

    return 0;
}