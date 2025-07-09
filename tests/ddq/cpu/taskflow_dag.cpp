#include<taskflow/taskflow.hpp>
extern "C"{
#include "dag.h"
#include "matrix.h"
}
#include    <cblas.h> // 添加 BLAS 库头文件
#include <chrono> // 添加时间测量的头文件
//gcc -c taskflow_dag.cpp -std=c++17 -O2 -o taskflow_dag.o -I/data/hanbin/workspace/taskflow -I/usr/include/openblas
//g++ taskflow_dag.o matrix.o dag.o -O2 -pthread -lopenblas -o taskflow_dag
#define SIZE 256
int row_size = SIZE;
int col_size = SIZE;
int reduce_size = SIZE;

tf::Taskflow generate_taskflow_from_dag(int num_nodes, const NodeInfo* node_info, double** matrices) {
    tf::Taskflow taskflow;
    std::vector<tf::Task> tasks(num_nodes);
    std::vector<std::vector<int>> dependencies(num_nodes);

    // 创建任务
    for (int i = 0; i < num_nodes; i++) {
        tasks[i] = taskflow.emplace([i, matrices]() {
            double alpha = 1.0, beta = 1.0;
            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                        row_size, col_size, reduce_size,
                        alpha, matrices[i * 3], reduce_size, matrices[i * 3 + 1], col_size,
                        beta, matrices[i * 3 + 2], col_size);
        });
    }

    // 设置任务依赖关系
    for (int i = 0; i < num_nodes; i++) {
        for (int j = 0; j < node_info[i].input_count; j++) {
            int input_node = node_info[i].inputs[j];
            dependencies[input_node].push_back(i);
        }
    }

    // 添加依赖关系到任务
    for (int i = 0; i < num_nodes; i++) {
        for (int j : dependencies[i]) {
            tasks[j].precede(tasks[i]);
        }
    }

    return taskflow;
}

int main(){
    int num_nodes = 500;
    int num_edges = 0;

    NodeInfo* node_info = generate_random_dag(num_nodes, num_edges);
    if (node_info) {
        // printf("生成的DAG节点信息：\n");
        // print_node_info(node_info, num_nodes);

        // 生成随机矩阵
        double** matrices = generate_matrix_array(num_nodes * 3, row_size, col_size);

        // 创建任务流
        tf::Taskflow taskflow = generate_taskflow_from_dag(num_nodes, node_info, matrices);

        // 执行任务流
        tf::Executor executor;
        auto start_time = std::chrono::high_resolution_clock::now(); // 开始时间
        executor.run(taskflow).wait();
        auto end_time = std::chrono::high_resolution_clock::now(); // 结束时间

        // 计算执行时间
        std::chrono::duration<double> elapsed_time = end_time - start_time;
        double execution_time = elapsed_time.count(); // 执行时间（秒）

        // 计算性能（GFLOPS）
        // 每次矩阵乘法的 FLOPs = 2 * row_size * col_size * reduce_size
        // 总 FLOPs = num_nodes * FLOPs_per_task
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