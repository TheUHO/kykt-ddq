#ifndef DEVICE_TIANHE_OPS_H
#define DEVICE_TIANHE_OPS_H
#include "task_types.h"
//device
#define TIANHECLUSTER_NUM 4
#define TIANHEDSP_NUM 24

task_ret	op_getMTthread(void **inputs, void **outputs);
task_ret    op_freeMTthread(void **inputs, void **outputs);
 #endif