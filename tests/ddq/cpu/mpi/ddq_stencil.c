#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include <string.h>

#include	"ddq.h"
#include	"oplib.h"
#include    "error.h"

#include    "std/std_ops/std_ops.h"
#include <mpi.h>
#include <omp.h>
#include <math.h>
#include <time.h>

#define N 10000  // 全局网格大小 (N x N)
#define ITERATIONS 10 // 迭代次数

int local_rows;
int local_cols;
int rank;
MPI_Comm comm;
MPI_Datatype column_type;
int sqrt_size;
int up;
int down; 
int left; 
int right;
MPI_Request requests[8];
MPI_Status statuses[8];

void initialize_grid(double* grid, int rank) {
    for (int i = 0; i < local_rows; i++) {
        for (int j = 0; j < local_cols; j++) {
            grid[i * local_cols + j] = rank;  // 用进程号初始化，便于调试
        }
    }
}

// 打印网格（仅限调试小网格）
void print_grid(double* grid, int rows, int cols, int rank) {
    // printf("Rank %d grid:\n", rank);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%.2f ", grid[i * cols + j]);
        }
        printf("\n");
    }
}

task_ret op_local_compute(void** inputs, void** outputs, void** attributes){
    // if(rank==0) printf("Rank %d op_local_compute begin\n", rank);
    double* grid = *(double**)inputs[0];
    double* new_grid = *(double**)outputs[0];
    int i, j;
    // 计算内部网格
    // #pragma omp parallel for collapse(2)
    for (i = 2; i < local_rows - 2; i++) {  // 跳过边界行
        for (j = 2; j < local_cols - 2; j++) {  // 跳过边界列
            new_grid[i * local_cols + j] = 0.25 * (
                grid[(i - 1) * local_cols + j] +  // 上
                grid[(i + 1) * local_cols + j] +  // 下
                grid[i * local_cols + (j - 1)] +  // 左
                grid[i * local_cols + (j + 1)]    // 右
            );
        }
    }

    return task_ret_ok;
}

task_ret op_boundaries_compute(void** inputs, void** outputs, void** attributes){
    // if(rank==0)printf("Rank %d op_boundaries_compute begin\n", rank);
    double* grid = *(double**)inputs[0];
    double* new_grid = *(double**)outputs[0];

    int i, j;
    // 计算上边界
    for (j = 1; j < local_cols - 1; j++) {
        new_grid[1 * local_cols + j] = 0.25 * (
            grid[0 * local_cols + j] +      // 上
            grid[2 * local_cols + j] +      // 下
            grid[1 * local_cols + (j - 1)] + // 左
            grid[1 * local_cols + (j + 1)]   // 右
        );
    }
    // 计算下边界
    for (j = 1; j < local_cols - 1; j++) {
        new_grid[(local_rows - 2) * local_cols + j] = 0.25 * (
            grid[(local_rows - 3) * local_cols + j] +  // 上
            grid[(local_rows - 1) * local_cols + j] +  // 下
            grid[(local_rows - 2) * local_cols + (j - 1)] + // 左
            grid[(local_rows - 2) * local_cols + (j + 1)]    // 右
        );
    }
    // 计算左边界
    for (i = 1; i < local_rows - 1; i++) {
        new_grid[i * local_cols + 1] = 0.25 * (
            grid[(i - 1) * local_cols + 1] +  // 上
            grid[(i + 1) * local_cols + 1] +  // 下
            grid[i * local_cols + 0] +        // 左
            grid[i * local_cols + 2]          // 右
        );
    }
    // 计算右边界
    for (i = 1; i < local_rows - 1; i++) {
        new_grid[i * local_cols + (local_cols - 2)] = 0.25 * (
            grid[(i - 1) * local_cols + (local_cols - 2)] +  // 上
            grid[(i + 1) * local_cols + (local_cols - 2)] +  // 下
            grid[i * local_cols + (local_cols - 3)] +        // 左
            grid[i * local_cols + (local_cols - 1)]          // 右
        );
    }
    
    return task_ret_ok;
}

task_ret op_halo_send(void** inputs, void** outputs, void** attributes){
    double* grid = *(double**)inputs[0];
    int* flag = (int*)inputs[1];
    
    if(*flag){
        MPI_Isend(&grid[local_cols], local_cols, MPI_DOUBLE, up, 0, comm, &requests[0]);
        MPI_Isend(&grid[(local_rows - 2) * local_cols], local_cols, MPI_DOUBLE, down, 1, comm, &requests[1]);
        MPI_Isend(&grid[1 * local_cols + 1], 1, column_type, left, 2, comm, &requests[2]);
        MPI_Isend(&grid[1 * local_cols + (local_cols - 2)], 1, column_type, right, 3, comm, &requests[3]);

    }

    MPI_Testall(4, requests, flag, statuses);
    if(!(*flag)){
        return task_ret_again;
    }

    return task_ret_ok;
}

task_ret op_halo_receive(void** inputs, void** outputs, void** attributes){
    // if(rank==0)printf("Rank %d halo receive begin\n", rank);
    double* grid = *(double**)outputs[0];
    int* flag = (int*)inputs[0];
    if(*flag){
        MPI_Irecv(&grid[(local_rows - 1) * local_cols], local_cols, MPI_DOUBLE, down, 0, comm, &requests[4]);
        MPI_Irecv(&grid[0], local_cols, MPI_DOUBLE, up, 1, comm, &requests[5]);
        MPI_Irecv(&grid[1 * local_cols + (local_cols - 1)], 1, column_type, right, 2, comm, &requests[6]);
        MPI_Irecv(&grid[1 * local_cols], 1, column_type, left, 3, comm, &requests[7]);
    }
    MPI_Testall(4, requests + 4, flag, statuses + 4);
    if(!(*flag)){
        return task_ret_again;
    }
    return task_ret_ok;

}

task_ret op_halo_sendreceive(void** inputs, void** outputs, void** attributes){
    // if(rank==0)printf("Rank %d halo receive begin\n", rank);
    double* grid = *(double**)outputs[0];
    int* flag = (int*)inputs[0];
    if(*flag){
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
    }
    MPI_Testall(8, requests, flag, statuses);
    if(!(*flag)){
        return task_ret_again;
    }
    return task_ret_ok;

}

void* new_int(){
    int* p = (int*)malloc(sizeof(int));
    *p = 1;
    return p;
}

void free_int(void* p){
    free(p);
}

task_ret op_update(void** inputs, void** outputs, void** attributes){
    // if(rank == 0){
    //     printf("Rank %d op_update begin count: %d\n", rank, (int)inputs[1]);
    // }   
    double** new_grid_ptr = (double**)inputs[0];
    inputs[1] = (int)inputs[1] - 1;
    double** grid_ptr = (double**)outputs[0];

    double* tmp = *grid_ptr;
    *grid_ptr = *new_grid_ptr;
    *new_grid_ptr = tmp;

    if((int)inputs[1] <= 1){
        return task_ret_done;
    }
    return task_ret_ok;
}

ddq_ring ddq_stencil_2d5(double** grid, double** new_grid){
    ddq_ring ring = ddq_new(NULL, 0, 0);
    
    obj obj_halo_receive = obj_import(ring, op_halo_receive, NULL, obj_prop_ready);
    obj obj_halo_send = obj_import(ring, op_halo_send, NULL, obj_prop_ready);
    obj obj_local_compute = obj_import(ring, op_local_compute, NULL, obj_prop_ready);
    obj obj_boundaries_compute = obj_import(ring, op_boundaries_compute, NULL, obj_prop_ready);
    obj obj_update = obj_import(ring, op_update, NULL, obj_prop_ready);

    obj obj_grid = obj_import(ring, grid, NULL, obj_prop_ready | obj_prop_consumable); 
    obj halo_neighbor = obj_import(ring, grid, NULL, obj_prop_consumable);
    obj new_halo = obj_import(ring, new_grid, NULL, obj_prop_consumable);
    obj new_local = obj_import(ring, new_grid, NULL, obj_prop_consumable);
    obj count = obj_import(ring, ITERATIONS - 1, NULL, obj_prop_ready);
    // obj count_1 = obj_import(ring, ITERATIONS, NULL, obj_prop_ready);

    int* send_flag = (int*)malloc(sizeof(int));
    *send_flag = 1;
    int* receive_flag = (int*)malloc(sizeof(int));
    *receive_flag = 1;
    obj obj_send_flag = obj_import(ring, send_flag, free_int, obj_prop_ready);
    obj obj_receive_flag = obj_import(ring, receive_flag, free_int, obj_prop_ready);

    ddq_op halo_receive = ddq_spawn(ring, processor_direct, 2, 1);
    ddq_add_f(halo_receive, obj_halo_receive);
    ddq_add_inputs(halo_receive, 0, obj_receive_flag);
    ddq_add_inputs(halo_receive, 1, obj_grid);
    ddq_add_outputs(halo_receive, 0, halo_neighbor);

    ddq_op halo_send = ddq_spawn(ring, processor_direct, 2, 0);
    ddq_add_f(halo_send, obj_halo_send);
    ddq_add_inputs(halo_send, 0, obj_grid);
    ddq_add_inputs(halo_send, 1, obj_send_flag);

    ddq_op local_compute = ddq_spawn(ring, processor_pthread_pool, 1, 1);
    ddq_add_f(local_compute, obj_local_compute);
    ddq_add_inputs(local_compute, 0, obj_grid);
    ddq_add_outputs(local_compute, 0, new_local);

    ddq_op boundaries_compute = ddq_spawn(ring, processor_pthread_pool, 1, 1);
    ddq_add_f(boundaries_compute, obj_boundaries_compute);
    // ddq_add_inputs(boundaries_compute, 0, obj_grid);
    ddq_add_inputs(boundaries_compute, 0, halo_neighbor);
    ddq_add_outputs(boundaries_compute, 0, new_halo);

    ddq_op update = ddq_spawn(ring, processor_pthread_pool, 3, 1);
    ddq_add_f(update, obj_update);
    ddq_add_inputs(update, 0, new_local);
    ddq_add_inputs(update, 1, count);
    ddq_add_inputs(update, 2, new_halo);
    ddq_add_outputs(update, 0, obj_grid);

    return ring;
}

//sendreceive
ddq_ring ddq_stencil_2d5_2(double** grid, double** new_grid){
    ddq_ring ring = ddq_new(NULL, 0, 0);
    
    obj obj_halo_sendreceive = obj_import(ring, op_halo_sendreceive, NULL, obj_prop_ready);
    obj obj_local_compute = obj_import(ring, op_local_compute, NULL, obj_prop_ready);
    obj obj_boundaries_compute = obj_import(ring, op_boundaries_compute, NULL, obj_prop_ready);
    obj obj_update = obj_import(ring, op_update, NULL, obj_prop_ready);

    obj obj_grid = obj_import(ring, grid, NULL, obj_prop_ready | obj_prop_consumable); 
    obj halo_neighbor = obj_import(ring, grid, NULL, obj_prop_consumable);
    obj new_halo = obj_import(ring, new_grid, NULL, obj_prop_consumable);
    obj new_local = obj_import(ring, new_grid, NULL, obj_prop_consumable);
    obj count = obj_import(ring, ITERATIONS - 1, NULL, obj_prop_ready);

    int* send_flag = (int*)malloc(sizeof(int));
    *send_flag = 1;
    int* receive_flag = (int*)malloc(sizeof(int));
    *receive_flag = 1;
    obj obj_send_flag = obj_import(ring, send_flag, free_int, obj_prop_ready);
    obj obj_receive_flag = obj_import(ring, receive_flag, free_int, obj_prop_ready);

    ddq_op halo_sendreceive = ddq_spawn(ring, processor_direct, 2, 1);
    ddq_add_f(halo_sendreceive, obj_halo_sendreceive);
    ddq_add_inputs(halo_sendreceive, 0, obj_receive_flag);
    ddq_add_inputs(halo_sendreceive, 1, obj_grid);
    ddq_add_outputs(halo_sendreceive, 0, halo_neighbor);

    ddq_op local_compute = ddq_spawn(ring, processor_pthread_pool, 1, 1);
    ddq_add_f(local_compute, obj_local_compute);
    ddq_add_inputs(local_compute, 0, obj_grid);
    ddq_add_outputs(local_compute, 0, new_local);

    ddq_op boundaries_compute = ddq_spawn(ring, processor_pthread_pool, 1, 1);
    ddq_add_f(boundaries_compute, obj_boundaries_compute);
    ddq_add_inputs(boundaries_compute, 0, halo_neighbor);
    ddq_add_outputs(boundaries_compute, 0, new_halo);

    ddq_op update = ddq_spawn(ring, processor_pthread_pool, 3, 1);
    ddq_add_f(update, obj_update);
    ddq_add_inputs(update, 0, new_local);
    ddq_add_inputs(update, 1, count);
    ddq_add_inputs(update, 2, new_halo);
    ddq_add_outputs(update, 0, obj_grid);

    return ring;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int provided;
    // MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    // if (provided < MPI_THREAD_MULTIPLE) {
    //     fprintf(stderr, "Error: MPI does not provide required threading level.\n");
    //     MPI_Finalize();
    //     return EXIT_FAILURE;
    // }

    int size;

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int sqrt_size = (int)sqrt(size);
    if (sqrt_size * sqrt_size != size) {
        if (rank == 0) {
            fprintf(stderr, "Error: Number %d of processes must be a perfect square.\n", size);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    local_rows = N / sqrt_size + 2;  // 每个进程的行数（包括幽灵行）
    local_cols = N / sqrt_size + 2;  // 每个进程的列数（包括幽灵列）
    // printf("Rank %d local_rows: %d local_cols: %d\n", rank, local_rows, local_cols);

    double* grid;
    double* new_grid;
    grid = (double*)malloc(local_rows * local_cols * sizeof(double));
    new_grid = (double*)malloc(local_rows * local_cols * sizeof(double));
    // printf("Rank %d grid %p new_grid %p\n", rank, grid, new_grid);
    initialize_grid(grid, rank);

    // 创建列数据类型
    MPI_Type_vector(local_rows - 2, 1, local_cols, MPI_DOUBLE, &column_type);
    MPI_Type_commit(&column_type);

    // 计算邻居进程的rank
    up = (rank - sqrt_size + sqrt_size * sqrt_size) % (sqrt_size * sqrt_size);  // 上方邻居
    down = (rank + sqrt_size) % (sqrt_size * sqrt_size);                        // 下方邻居
    left = (rank % sqrt_size == 0) ? MPI_PROC_NULL : rank - 1;                  // 左方邻居
    right = (rank % sqrt_size == sqrt_size - 1) ? MPI_PROC_NULL : rank + 1;     // 右方邻居

    ddq_loop_init();

    ddq_ring ring = ddq_stencil_2d5(&grid, &new_grid);
    ddq_update(ring);
    // printf("Rank %d grid before computation:\n", rank);
    // printf("myID is %d\n", getpid());
    // int j=1;         
    // while(j){
    //     sleep(2);  // 陷入休眠，避免执行到程序异常处，导致中途退出
    // }
    // 记录 ddq_loop 的开始时间
    double start_time = MPI_Wtime();

    // 执行 ddq_loop
    ddq_loop(ring, 0);

    // 记录 ddq_loop 的结束时间
    double end_time = MPI_Wtime();

    // 计算 ddq_loop 的执行时间
    double elapsed_time = end_time - start_time;

    // 计算性能（GFLOPS）
    // 每次 stencil 运算的浮点操作次数为 4 * (local_rows - 2) * (local_cols - 2)
    // 总浮点操作次数为 ITERATIONS * 4 * (local_rows - 2) * (local_cols - 2) * size
    double total_flops = (ITERATIONS) * 4.0 * (local_rows - 2) * (local_cols - 2) * size;
    double gflops = total_flops / (elapsed_time * 1e9);

    // 打印结果
    if (rank == 0) {
        printf("ddq_loop 执行时间: %.6f 秒\n", elapsed_time);
        printf("性能: %.2f GFLOPS\n", gflops);
    }


    // 打印结果（仅限调试小网格）
    if (N <= 16) {
        if(rank == 0){
            print_grid(grid, local_rows, local_cols, rank);
            print_grid(new_grid, local_rows, local_cols, rank);   
        }
    }

    // 清理资源
    free(grid);
    free(new_grid);
    ddq_delete(ring);
    MPI_Type_free(&column_type);
    
    MPI_Finalize();
    
    return EXIT_SUCCESS;
}