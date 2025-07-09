#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
// 随机生成矩阵
double* generate_random_matrix(int rows, int cols) {
    double* matrix = (double*)malloc(rows * cols * sizeof(double));
    if (!matrix) {
        fprintf(stderr, "内存分配失败！\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL)); // 初始化随机数种子
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i * cols + j] = (double)(rand() % 100) / 10.0; // 生成 0.0 到 10.0 的随机数
        }
    }

    return matrix;
}

// 根据指定数目生成矩阵数组
double** generate_matrix_array(int num_matrices, int rows, int cols) {
    double** matrices = (double**)malloc(num_matrices * sizeof(double*));
    if (!matrices) {
        fprintf(stderr, "内存分配失败！\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_matrices; i++) {
        matrices[i] = generate_random_matrix(rows, cols);
    }

    return matrices;
}


// 比较两个矩阵数组是否一致
bool compare_matrix_arrays(double** array1, double** array2, int num_matrices, int rows, int cols) {
    for (int i = 0; i < num_matrices; i++) {
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                double val1 = array1[i][r * cols + c];
                double val2 = array2[i][r * cols + c];
                //差值比较小的话表示一致
                if (fabs(val1 - val2) > 1e-6) {
                    printf("矩阵 %d 的元素 [%d][%d] 不一致: %f != %f\n", i, r, c, val1, val2);
                    return false;
                }
            }
        }
    }
    return true;
}

// 将矩阵数组写入文件
void write_matrix_array_to_file(const char* filename, double** matrices, int num_matrices, int rows, int cols) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "无法打开文件 %s 进行写入！\n", filename);
        exit(EXIT_FAILURE);
    }

    // 写入矩阵数量、行数和列数
    fwrite(&num_matrices, sizeof(int), 1, file);
    fwrite(&rows, sizeof(int), 1, file);
    fwrite(&cols, sizeof(int), 1, file);

    // 写入每个矩阵的数据
    for (int i = 0; i < num_matrices; i++) {
        fwrite(matrices[i], sizeof(double), rows * cols, file);
    }

    fclose(file);
}

// 从文件中读取矩阵数组
double** read_matrix_array_from_file(const char* filename, int* num_matrices, int* rows, int* cols) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "无法打开文件 %s 进行读取！\n", filename);
        exit(EXIT_FAILURE);
    }

    // 读取矩阵数量、行数和列数
    fread(num_matrices, sizeof(int), 1, file);
    fread(rows, sizeof(int), 1, file);
    fread(cols, sizeof(int), 1, file);

    // 分配矩阵数组
    double** matrices = (double**)malloc(*num_matrices * sizeof(double*));
    if (!matrices) {
        fprintf(stderr, "内存分配失败！\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < *num_matrices; i++) {
        matrices[i] = (double*)malloc((*rows) * (*cols) * sizeof(double));
        if (!matrices[i]) {
            fprintf(stderr, "内存分配失败！\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        fread(matrices[i], sizeof(double), (*rows) * (*cols), file);
    }

    fclose(file);
    return matrices;
}

// 释放矩阵数组
void free_matrix_array(double** matrices, int num_matrices) {
    for (int i = 0; i < num_matrices; i++) {
        free(matrices[i]);
    }
    free(matrices);
}

void free_matrix(void* p){
    free(p);
    return;
}

void clear_cpu_cache() {
    size_t cache_size = 256 * 1024 * 1024; // 假设 CPU 缓存大小为 256MB
    char* buffer = (char*)malloc(cache_size);
    if (!buffer) {
        fprintf(stderr, "内存分配失败！\n");
        return;
    }

    for (size_t i = 0; i < cache_size; i++) {
        buffer[i] = i % 256; // 写入数据以确保缓存被占用
    }

    free(buffer);
}