#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include <string.h>

#include	"ddq.h"
#include	"oplib.h"
#include    "error.h"

#include    "std/std_ops/std_ops.h"
#include    "dag.h"
#include    "ddq_dag.h"
#include    <cblas.h> // 添加 BLAS 库头文件
#include <stdbool.h>

extern int row_size;
extern int col_size;
extern int reduce_size;

task_ret op_nothing(void** inputs, void** outputs, void** attributes){}
task_ret op_matmul_raw(void** inputs, void** outputs, void** attributes){
    // int_t row_size, col_size, reduce_size;
    double* imat0, * imat1, * omat;
    // row_size = (int_t)inputs[0];
    // col_size = (int_t)inputs[1];
    // reduce_size = (int_t)inputs[2];

    imat0 = (void*)inputs[0];
    imat1 = (void*)inputs[1];
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
    // printf("node %d op_matmul_raw done!\n", (int)inputs[2]);
    return task_ret_ok;
}

task_ret op_matmul_optimized(void** inputs, void** outputs, void** attributes){
    double* imat0 = (double*)inputs[0];
    double* imat1 = (double*)inputs[1];
    double* omat = (double*)outputs[0];

    // 使用 BLAS 的 dgemm 进行矩阵乘法
    // C = alpha * A * B + beta * C
    // printf("OpenBLAS 使用的线程数: %d\n", openblas_get_num_threads());
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
        
// 根据 DAG 自动生成算子图
// ddq_ring generate_ddq_from_dag_raw(int num_nodes, NodeInfo* nodes) {
//     ddq_ring ring = ddq_new(NULL, 0, 0);

//     obj matmul = obj_import(ring, op_matmul_raw, NULL, obj_prop_ready);
//     obj nothing = obj_import(ring, op_nothing, NULL, obj_prop_ready);
//     // 创建op
//     ddq_op *ops = (ddq_op*)malloc(num_nodes * sizeof(ddq_op));
//     int* inputs = (int*)malloc(num_nodes * sizeof(int));
//     int nothing_inputs = 0;
//     for (int i = 0; i < num_nodes; i++) {
//         ops[i] = ddq_spawn(ring, processor_pthread, 3 + nodes[i].input_count, 1);
//         obj imat0 = obj_import(ring, generate_random_matrix(row_size, reduce_size), free_matrix, obj_prop_ready | obj_prop_consumable);
//         obj imat1 = obj_import(ring, generate_random_matrix(reduce_size, col_size), free_matrix, obj_prop_ready | obj_prop_consumable);
//         obj omat = obj_import(ring, generate_random_matrix(row_size, col_size), free_matrix, obj_prop_consumable);
//         obj id = obj_import(ring, nodes[i].node_id, NULL, obj_prop_ready | obj_prop_consumable);
//         ddq_add_f(ops[i], matmul);
//         ddq_add_inputs(ops[i], 0, imat0);
//         ddq_add_inputs(ops[i], 1, imat1);
//         ddq_add_inputs(ops[i], 2, id);
//         ddq_add_outputs(ops[i], 0, omat);
//         inputs[i] = 3;

//         if(nodes[i].output_count == 0){
//             nothing_inputs++;
//         }
//     }

//     // 创建nothing算子
//     ddq_op nothing_op = ddq_spawn(ring, processor_pthread, nothing_inputs, 0);
//     ddq_add_f(nothing_op, nothing);

//     // 创建obj并连接算子
//     for (int i = 0; i < num_nodes; i++) {
//         if(nodes[i].output_count == 0){
//             ddq_add_inputs(nothing_op, --nothing_inputs, ops[i]->outputs[0]);
//         }
//         for (int j = 0; j < nodes[i].output_count; j++) {
//             int to_node = nodes[i].outputs[j];
//             ddq_add_inputs(ops[to_node], inputs[to_node]++, ops[i]->outputs[0]);
//             // inputs[to_node]++;
//         }
//     }

//     // free(nodes);
//     return ring;
// }

ddq_ring generate_ddq_from_dag_optimized(int num_nodes,const NodeInfo* nodes, double **matrixs) {
    printf("OpenBLAS 使用的线程数: %d\n", openblas_get_num_threads());
    ddq_ring ring = ddq_new(NULL, 0, 0);

    obj matmul = obj_import(ring, op_matmul_optimized, NULL, obj_prop_ready);
    obj nothing = obj_import(ring, op_nothing, NULL, obj_prop_ready);
    // 创建op
    ddq_op *ops = (ddq_op*)malloc(num_nodes * sizeof(ddq_op));
    int* inputs = (int*)malloc(num_nodes * sizeof(int));
    int nothing_inputs = 0;
    for (int i = 0; i < num_nodes; i++) {
        ops[i] = ddq_spawn(ring, processor_pthread, 2 + nodes[i].input_count, 1);
        obj imat0 = obj_import(ring, matrixs[i*3], NULL, obj_prop_ready | obj_prop_consumable);
        obj imat1 = obj_import(ring, matrixs[i*3 + 1], NULL, obj_prop_ready | obj_prop_consumable);
        obj omat = obj_import(ring, matrixs[i*3 + 2], NULL, obj_prop_consumable);
        // obj id = obj_import(ring, nodes[i].node_id, NULL, obj_prop_ready | obj_prop_consumable);
        ddq_add_f(ops[i], matmul);
        ddq_add_inputs(ops[i], 0, imat0);
        ddq_add_inputs(ops[i], 1, imat1);
        // ddq_add_inputs(ops[i], 2, id);
        ddq_add_outputs(ops[i], 0, omat);
        inputs[i] = 2;

        if(nodes[i].output_count == 0){
            nothing_inputs++;
        }
    }

    // 创建nothing算子
    ddq_op nothing_op = ddq_spawn(ring, processor_pthread, nothing_inputs, 0);
    ddq_add_f(nothing_op, nothing);

    // 创建obj并连接算子
    for (int i = 0; i < num_nodes; i++) {
        if(nodes[i].output_count == 0){
            ddq_add_inputs(nothing_op, --nothing_inputs, ops[i]->outputs[0]);
        }
        for (int j = 0; j < nodes[i].output_count; j++) {
            int to_node = nodes[i].outputs[j];
            ddq_add_inputs(ops[to_node], inputs[to_node]++, ops[i]->outputs[0]);
            // inputs[to_node]++;
        }
    }

    // free(nodes);
    free(ops);
    free(inputs);
    return ring;
}

ddq_ring generate_ddq_from_dag_pthread_pool(int num_nodes,const NodeInfo* nodes, double **matrixs) {
    printf("OpenBLAS 使用的线程数: %d\n", openblas_get_num_threads());
    ddq_ring ring = ddq_new(NULL, 0, 0);

    obj matmul = obj_import(ring, op_matmul_optimized, NULL, obj_prop_ready);
    obj nothing = obj_import(ring, op_nothing, NULL, obj_prop_ready);
    // 创建op
    ddq_op *ops = (ddq_op*)malloc(num_nodes * sizeof(ddq_op));
    int* inputs = (int*)malloc(num_nodes * sizeof(int));
    int nothing_inputs = 0;
    for (int i = 0; i < num_nodes; i++) {
        ops[i] = ddq_spawn(ring, processor_pthread_pool, 2 + nodes[i].input_count, 1);
        obj imat0 = obj_import(ring, matrixs[i*3], NULL, obj_prop_ready | obj_prop_consumable);
        obj imat1 = obj_import(ring, matrixs[i*3 + 1], NULL, obj_prop_ready | obj_prop_consumable);
        obj omat = obj_import(ring, matrixs[i*3 + 2], NULL, obj_prop_consumable);
        // obj id = obj_import(ring, nodes[i].node_id, NULL, obj_prop_ready | obj_prop_consumable);
        ddq_add_f(ops[i], matmul);
        ddq_add_inputs(ops[i], 0, imat0);
        ddq_add_inputs(ops[i], 1, imat1);
        // ddq_add_inputs(ops[i], 2, id);
        ddq_add_outputs(ops[i], 0, omat);
        inputs[i] = 2;

        if(nodes[i].output_count == 0){
            nothing_inputs++;
        }
    }

    // 创建nothing算子
    // ddq_op nothing_op = ddq_spawn(ring, processor_pthread_pool, nothing_inputs, 0);
    // ddq_add_f(nothing_op, nothing);

    // 创建obj并连接算子
    for (int i = 0; i < num_nodes; i++) {
        // if(nodes[i].output_count == 0){
        //     ddq_add_inputs(nothing_op, --nothing_inputs, ops[i]->outputs[0]);
        // }
        for (int j = 0; j < nodes[i].output_count; j++) {
            int to_node = nodes[i].outputs[j];
            ddq_add_inputs(ops[to_node], inputs[to_node]++, ops[i]->outputs[0]);
        }
    }

    // free(nodes);
    free(ops);
    free(inputs);
    return ring;
}