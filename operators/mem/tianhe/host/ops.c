#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "task_types.h"
#include "basic_types.h"

#include "ops.h"
#include "device/tianhe/host/types.h"

#include "hthread_host.h"

//TODO:1.dma_chl可以用专门的队列去维护 2.inputs[i]和outputs[i]均保存数据的指针，构建函数和析构函数的逻辑需要更改一下
task_ret    op_mem2cluster(void** inputs, void** outputs){
    void* mem = ACCESS_INPUTS(0, void*);
    int_t size = ACCESS_INPUTS(1, int_t);
    tianhe_thread thread = ACCESS_INPUTS(2, tianhe_thread);

    void* res = ACCESS_OUTPUTS(0, void*);
    if(!res){
        res = hthread_malloc(thread->cluster_id, size, HT_MEM_RW);
        WRITE_OUTPUTS(0, void*, res);
    }
    memcpy(res, mem, size);

    return task_ret_ok;
}
task_ret    op_cluster2mem(void** inputs, void** outputs){
    void* cluster_mem = ACCESS_INPUTS(0, void*);
    int_t size = ACCESS_INPUTS(1, int_t);
    
    void* res = ACCESS_OUTPUTS(0, void*);
    if(!res){
        res = malloc(size);
        WRITE_OUTPUTS(0, void*, res);
    }
    memcpy(res, cluster_mem, size);

    return task_ret_ok;
}