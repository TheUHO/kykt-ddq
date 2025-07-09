#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
//mpicc -o stencil stencil.c -fopenmp -lm -O2
#define N 10000  // 全局网格大小 (N x N)
#define ITERATIONS 10  // 迭代次数

static int local_rows;
static int local_cols;

// 初始化网格
void initialize_grid(double* grid, int rank) {
    for (int i = 0; i < local_rows; i++) {
        for (int j = 0; j < local_cols; j++) {
            grid[i * local_cols + j] = rank;  // 用进程号初始化，便于调试
        }
    }
}

// 打印网格（仅限调试小网格）
void print_grid(double* grid, int rows, int cols, int rank) {
    printf("Rank %d grid:\n", rank);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%.2f ", grid[i * cols + j]);
        }
        printf("\n");
    }
}

// 交换边界数据
void exchange_boundaries(double* grid, MPI_Comm comm, int rank, int sqrt_size) {
    MPI_Request requests[8];
    MPI_Status statuses[8];

    int up = (rank - sqrt_size + sqrt_size * sqrt_size) % (sqrt_size * sqrt_size);  // 上方邻居
    int down = (rank + sqrt_size) % (sqrt_size * sqrt_size);                        // 下方邻居
    int left = (rank % sqrt_size == 0) ? MPI_PROC_NULL : rank - 1;                  // 左方邻居
    int right = (rank % sqrt_size == sqrt_size - 1) ? MPI_PROC_NULL : rank + 1;     // 右方邻居

    // 定义列数据类型
    MPI_Datatype column_type;
    MPI_Type_vector(local_rows - 2, 1, local_cols, MPI_DOUBLE, &column_type);
    MPI_Type_commit(&column_type);

    // 临时缓冲区，用于存储左右边界数据
    // double* left_buffer = (double*)malloc((local_rows - 2) * sizeof(double));
    // double* right_buffer = (double*)malloc((local_rows - 2) * sizeof(double));

    // // 打包左边界和右边界数据
    // for (int i = 1; i < local_rows - 1; i++) {
    //     left_buffer[i - 1] = grid[i * local_cols + 1];               // 左边界
    //     right_buffer[i - 1] = grid[i * local_cols + (local_cols - 2)]; // 右边界
    // }

    // 发送上边界，接收下边界
    MPI_Isend(&grid[local_cols], local_cols, MPI_DOUBLE, up, 0, comm, &requests[0]);
    MPI_Irecv(&grid[(local_rows - 1) * local_cols], local_cols, MPI_DOUBLE, down, 0, comm, &requests[1]);

    // 发送下边界，接收上边界
    MPI_Isend(&grid[(local_rows - 2) * local_cols], local_cols, MPI_DOUBLE, down, 1, comm, &requests[2]);
    MPI_Irecv(&grid[0], local_cols, MPI_DOUBLE, up, 1, comm, &requests[3]);

    // 发送左边界，接收右边界
    MPI_Isend(&grid[1 * local_cols + 1], 1, column_type, left, 2, comm, &requests[4]);
    MPI_Irecv(&grid[1 * local_cols + (local_cols - 1)], 1, column_type, right, 2, comm, &requests[5]);

    // 发送右边界，接收左边界
    MPI_Isend(&grid[1 * local_cols + (local_cols - 2)], 1, column_type, right, 3, comm, &requests[6]);
    MPI_Irecv(&grid[1 * local_cols], 1, column_type, left, 3, comm, &requests[7]);


    MPI_Waitall(8, requests, statuses);

    // 释放列数据类型
    MPI_Type_free(&column_type);
    // // 释放临时缓冲区
    // free(left_buffer);
    // free(right_buffer);
}

void compute_boundaries(double* grid, double* new_grid) {
    // 计算上边界
    for (int j = 1; j < local_cols - 1; j++) {
        new_grid[1 * local_cols + j] = 0.25 * (
            grid[0 * local_cols + j] +      // 上
            grid[2 * local_cols + j] +      // 下
            grid[1 * local_cols + (j - 1)] + // 左
            grid[1 * local_cols + (j + 1)]   // 右
        );
    }

    // 计算下边界
    for (int j = 1; j < local_cols - 1; j++) {
        new_grid[(local_rows - 2) * local_cols + j] = 0.25 * (
            grid[(local_rows - 3) * local_cols + j] +  // 上
            grid[(local_rows - 1) * local_cols + j] +  // 下
            grid[(local_rows - 2) * local_cols + (j - 1)] + // 左
            grid[(local_rows - 2) * local_cols + (j + 1)]    // 右
        );
    }

    // 计算左边界
    for (int i = 1; i < local_rows - 1; i++) {
        new_grid[i * local_cols + 1] = 0.25 * (
            grid[(i - 1) * local_cols + 1] +  // 上
            grid[(i + 1) * local_cols + 1] +  // 下
            grid[i * local_cols + 0] +        // 左
            grid[i * local_cols + 2]          // 右
        );
    }

    // 计算右边界
    for (int i = 1; i < local_rows - 1; i++) {
        new_grid[i * local_cols + (local_cols - 2)] = 0.25 * (
            grid[(i - 1) * local_cols + (local_cols - 2)] +  // 上
            grid[(i + 1) * local_cols + (local_cols - 2)] +  // 下
            grid[i * local_cols + (local_cols - 3)] +        // 左
            grid[i * local_cols + (local_cols - 1)]          // 右
        );
    }
}

void stencil_omp(double* grid, double* new_grid) {
    // #pragma omp parallel for collapse(2)
    for (int i = 2; i < local_rows - 2; i++) {  // 跳过边界行
        for (int j = 2; j < local_cols - 2; j++) {  // 跳过边界列
            new_grid[i * local_cols + j] = 0.25 * (
                grid[(i - 1) * local_cols + j] +  // 上
                grid[(i + 1) * local_cols + j] +  // 下
                grid[i * local_cols + (j - 1)] +  // 左
                grid[i * local_cols + (j + 1)]    // 右
            );
        }
    }
}

// // 执行 stencil 运算（多线程版本）
// void stencil_omp(double* grid, double* new_grid, int local_rows, int local_cols) {
//     #pragma omp parallel for collapse(2)
//     for (int i = 1; i < local_rows - 1; i++) {
//         for (int j = 1; j < local_cols - 1; j++) {
//             new_grid[i * local_cols + j] = 0.25 * (
//                 grid[(i - 1) * local_cols + j] +  // 上
//                 grid[(i + 1) * local_cols + j] +  // 下
//                 grid[i * local_cols + (j - 1)] +  // 左
//                 grid[i * local_cols + (j + 1)]    // 右
//             );
//         }
//     }
// }

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    // 假设网格可以均匀划分
    int sqrt_size = (int)sqrt(size);
    if (sqrt_size * sqrt_size != size) {
        if (rank == 0) {
            fprintf(stderr, "Error: Number of processes must be a perfect square.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    local_rows = N / sqrt_size + 2;  // 每个进程的行数（包括幽灵行）
    local_cols = N / sqrt_size + 2;  // 每个进程的列数（包括幽灵列）

    double* grid = (double*)malloc(local_rows * local_cols * sizeof(double));
    double* new_grid = (double*)malloc(local_rows * local_cols * sizeof(double));

    initialize_grid(grid, rank);

    // 设置 OpenMP 使用的线程数
    int num_threads = omp_get_max_threads();  // 默认使用所有可用线程
    if (rank == 0) {
        printf("Rank %d 使用的线程数: %d\n", rank, num_threads);
    }

    double start_time = MPI_Wtime();

    for (int iter = 0; iter < ITERATIONS; iter++) {
        // 交换边界数据
        exchange_boundaries(grid, comm, rank, sqrt_size);
    
        // 计算边界
        compute_boundaries(grid, new_grid);
    
        // 计算内部网格
        stencil_omp(grid, new_grid);
        
        // 交换指针
        double* temp = grid;
        grid = new_grid;
        new_grid = temp;


    }

    // 记录 ddq_loop 的结束时间
    double end_time = MPI_Wtime();

    // 计算 ddq_loop 的执行时间
    double elapsed_time = end_time - start_time;

    // 计算性能（GFLOPS）
    // 每次 stencil 运算的浮点操作次数为 4 * (local_rows - 2) * (local_cols - 2)
    // 总浮点操作次数为 ITERATIONS * 4 * (local_rows - 2) * (local_cols - 2) * size
    double total_flops = ITERATIONS * 4.0 * (local_rows - 2) * (local_cols - 2) * size;
    double gflops = total_flops / (elapsed_time * 1e9);

    // 打印结果
    if (rank == 0) {
        printf("ddq_loop 执行时间: %.6f 秒\n", elapsed_time);
        printf("性能: %.2f GFLOPS\n", gflops);
    }

    // 打印结果（仅限调试小网格）
    if (N <= 16) {
        if (rank == 0){
            print_grid(grid, local_rows, local_cols, rank);
            print_grid(new_grid, local_rows, local_cols, rank);
        }
    }

    free(grid);
    free(new_grid);

    MPI_Finalize();
    return 0;
}