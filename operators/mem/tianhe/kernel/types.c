#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include    <string.h>
#include	"hthread_device.h"
#include	"ddq.h"
#include	"task_types.h"
#include "ddq_plugin.h"
#include "error.h"
#include "std/std_types/std_types.h"

#define BLOCK_ROW_SIZE 16
#define BLOCK_COL_SIZE 16
#define BLOCK_REDUCE_SIZE 16

void* obj_mem_new(){
    obj_mem res = malloc(sizeof(obj_mem_t));
    res->type = obj_mem_default;
    res->p = NULL;
    res->size = 0;
    res->bufsize = 0;
    return res;
}

// void* obj_mem_gsm_new(){
//     // ddq_log0("sizeof(obj_mem_t):%d\n", sizeof(obj_mem_t));
//     obj_mem res = obj_mem_gsm;
//     if(get_thread_id()==0){
//         res->type = obj_mem_default;
//         res->p = NULL;
//         res->size = 0;
//         res->bufsize = 0;
//     }
//     return res;
// }

//不同的环使用同一个obj_mem的话，内部的指针应该由谁free？
void obj_mem_delete(void* p){
    //     ddq_log0("obj_mem_delete begin %p\n", p);

    // obj_mem mem = (obj_mem)p;
    // free(p);
}

void obj_mem_gsm_delete(void* p){
    //     ddq_log0("obj_mem_delete begin %p\n", p);

    // obj_mem mem = (obj_mem)p;
    // free(p);
}