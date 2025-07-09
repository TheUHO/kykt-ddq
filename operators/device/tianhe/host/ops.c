#include "ops.h"
#include "types.h"
#include "error.h"

#include "hthread_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//获取包含指定DSP核数的MT线程组
//inputs：
//inputs[0]：申请的核数
task_ret	op_getMTthread(void **inputs, void **outputs, void** attribute){
    int core_num = ACCESS_INPUTS(0, int);

    int cluster_id, thread_id;
    for(cluster_id=0; cluster_id<TIANHECLUSTER_NUM; cluster_id++){
        if(hthread_get_avail_threads(cluster_id) >= core_num)
            break;
    }
    if(cluster_id >= TIANHECLUSTER_NUM){
        printf("Insufficient cores\n");
        exit(-1);
    }
    if((thread_id = hthread_group_create(cluster_id, core_num)) == -1){
        printf("hthread_group_create failed!\n");
        exit(-1);
    }
    tianhe_thread t = ACCESS_OUTPUTS(0, tianhe_thread);
    t->thread_id = thread_id;
    t->cluster_id = cluster_id;
    t->core_num = core_num;

    return task_ret_ok;
}

task_ret op_freeMTthread(void **inputs, void **outputs, void **attribute){
    tianhe_thread thread = ACCESS_INPUTS(0, tianhe_thread);

    hthread_group_destroy(thread->thread_id);
    
    return  task_ret_ok;
}