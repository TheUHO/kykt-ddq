#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include    <string.h>
#include	"hthread_device.h"
#include	"ddq.h"
#include	"task_types.h"
#include "ddq_plugin.h"
#include "error.h"

#define BLOCK_ROW_SIZE 16
#define BLOCK_COL_SIZE 16
#define BLOCK_REDUCE_SIZE 16

void* ptr_new(){
    // ddq_log0("ptr_new begin\n");
    return malloc(sizeof(void*));
}

void * mat_new(){
    void* mat =  malloc(sizeof(double) * BLOCK_ROW_SIZE * BLOCK_COL_SIZE);
    memset(mat, 0, sizeof(double) * BLOCK_ROW_SIZE * BLOCK_COL_SIZE);
    return mat;
}

void* int_new(){
    int* res =  malloc(sizeof(int));
    *res = 0;
    return res;
}

void* long_new(){
    // ddq_log0("long_new begin\n");
    long* res = malloc(sizeof(long));
    *res = 0;
    // ddq_log0("long_new end\n");
    return res;
}

void* raw_new(){return NULL;}

void raw_delete(void* p){
    free(p);
}

const int core_num = 1;
const int Mg = 1512;//1512
const int Ng = 1152;//1152
const int Kg = 512;//512
const int Ma = 252;//252
const int Na = 48;
const int Ka = 512;
const int Ms = 6;
const int row_size = Mg;
const int col_size = Ng * core_num;
const int reduce_size = Kg;

//DSP func
void* obj_Agsm_retention_count_new(){
    int_t* ptr = long_new();
    *ptr = Mg*Kg/Ms/Ka*Ng/Na;
    return ptr;
}

void* obj_Bam_retention_count_new(){
    int_t* ptr = long_new();
    *ptr = Mg/Ms;
    return ptr;
}

void* obj_Cam_retention_count_new(){
    int_t* ptr = long_new();
    *ptr = Ma/Ms;
    return ptr;
}

void* obj_iter_start_C_retention_count_new(){
    int_t* ptr = long_new();
    *ptr = reduce_size/Kg;
    return ptr;
}

void* obj_iter_B_retention_count_new(){
    int_t* ptr = long_new();
    *ptr = row_size/Mg;
    return ptr;
}

void* obj_iter_Agsm_1_retention_count_new(){
    int_t* ptr = long_new();
    *ptr = Ng/Na;
    return ptr;
}