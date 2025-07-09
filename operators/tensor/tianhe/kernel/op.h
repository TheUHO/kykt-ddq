#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"hthread_device.h"
#include	"ddq.h"
#include	"task_types.h"
#include "ddq_plugin.h"
#include "error.h"
#include <compiler/m3000.h>

task_ret op_gemm_old(void** inputs, void**outputs);
task_ret op_gemm(void** inputs, void**outputs);
//需要一个保持算子
task_ret op_remain(void** inputs, void**outputs);