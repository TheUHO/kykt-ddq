#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cblas.h> // 使用 BLAS 库
#include <openblas_config.h>

//gcc -o test_peak_performance test_peak_performance.c -lopenblas -lm -I/usr/include/openblas

#define MATRIX_SIZE 20000 // 矩阵大小（可调整以测试不同规模）

// 随机生成矩阵
double* generate_random_matrix(int rows, int cols) {
    double* matrix = (double*)malloc(rows * cols * sizeof(double));
    if (!matrix) {
        fprintf(stderr, "内存分配失败！\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows * cols; i++) {
        matrix[i] = (double)(rand() % 100) / 10.0; // 生成 0.0 到 10.0 的随机数
    }

    return matrix;
}

int main() {
    // openblas_set_num_threads(4);
    printf("OpenBLAS 使用的线程数: %d\n", openblas_get_num_threads());
    int size = MATRIX_SIZE;
    double *A, *B, *C;
    double alpha = 1.0, beta = 0.0;

    // 分配矩阵内存
    A = generate_random_matrix(size, size);
    B = generate_random_matrix(size, size);
    C = (double*)calloc(size * size, sizeof(double));
    if (!C) {
        fprintf(stderr, "内存分配失败！\n");
        free(A);
        free(B);
        exit(EXIT_FAILURE);
    }

    // 初始化时间测量
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start); // 记录开始时间

    // 执行矩阵乘法
    // for(int i=0; i<1000; i++)
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                size, size, size,
                alpha, A, size, B, size,
                beta, C, size);

    clock_gettime(CLOCK_MONOTONIC, &end); // 记录结束时间

    // 计算运行时间
    double elapsed_time = (end.tv_sec - start.tv_sec) +
                          (end.tv_nsec - start.tv_nsec) / 1e9;

    // 计算 GFLOPS
    double gflops = (2.0 * size * size * size) / (elapsed_time * 1e9);
    printf("矩阵大小: %dx%d\n", size, size);
    printf("运行时间: %.6f 秒\n", elapsed_time);
    printf("性能: %.2f GFLOPS\n", gflops);

    // 释放内存
    free(A);
    free(B);
    free(C);

    return 0;
}