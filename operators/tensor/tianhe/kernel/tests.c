#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"hthread_device.h"
#include	"ddq.h"
#include	"task_types.h"
#include "ddq_plugin.h"
#include "error.h"
#include <compiler/m3000.h>

#include "op.h"

__global__ void test_op_gemm(int_t Ms, int_t Ma, int_t Na, int_t Ka, double* A, double* B, double* C){
    // hthread_printf("1\n");
    obj_mem mem_A = malloc(sizeof(obj_mem_t));
    mem_A->p = malloc(Ms * Ka * sizeof(double));
    mem_A->size = Ms * Ka * sizeof(double);
    mem_A->bufsize = Ms * Ka * sizeof(double);
    obj_mem mem_B = malloc(sizeof(obj_mem_t));
    mem_B->p = vector_malloc(Na * Ka * sizeof(double));
    mem_B->size = Na * Ka * sizeof(double);
    mem_B->bufsize = Na * Ka * sizeof(double);
    obj_mem mem_C = malloc(sizeof(obj_mem_t));
    mem_C->p = vector_malloc(Na * Ma * sizeof(double));
    mem_C->size = Na * Ma * sizeof(double);
    mem_C->bufsize = Na * Ma * sizeof(double);
    obj_mem mem_res = malloc(sizeof(obj_mem_t));
    mem_C->p = vector_malloc(Na * Ma * sizeof(double));
    mem_C->size = Na * Ma * sizeof(double);
    mem_C->bufsize = Na * Ma * sizeof(double);
    // hthread_printf("2\n");
    vector_load(B, mem_B->p, mem_B->size);
    vector_load(C, mem_C->p, mem_C->size);
    // hthread_printf("3\n");
    int_t* size_array = malloc(sizeof(int_t) * 5);
    size_array[0] = Ms;
    size_array[1] = Na;
    size_array[2] = Ka;
    size_array[3] = Ma/Ms;
    size_array[4] = 0;
    void** inputs = malloc(sizeof(void*) * 4);
    inputs[0] = mem_A;
    inputs[1] = mem_B;
    inputs[2] = mem_C;
    inputs[3] = size_array;
    void** outputs = malloc(sizeof(void*) * 1);
    outputs[0] = mem_C;
    // hthread_printf("4\n");
    for(int i=0; i<Ma/Ms; i++){
        scalar_load(A + i * Ms * Ka, mem_A->p, mem_A->size);
        op_gemm_old(inputs, outputs);
    }
    //  hthread_printf("8\n");
    vector_store(mem_C->p, C, Na * Ma * sizeof(double));
}