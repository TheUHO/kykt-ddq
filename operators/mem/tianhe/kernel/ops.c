#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"hthread_device.h"
#include	"ddq.h"
#include	"task_types.h"
#include "ddq_plugin.h"
#include "error.h"
#include "std/std_types/std_types.h"

__gsm__ int_t signals_gsm[24];
__gsm__ double Agsm[1512*512/2];

// task_ret op_enable_cores(void** inputs, void** outputs, void** attributes){

// }

task_ret    op_cluster2sLDM(void** inputs, void** outputs, void** attributes){
    void* cluster_mem = (void*)inputs[0];
    int_t size = (int_t)inputs[1];
    
    void* res = (void*)outputs[0];
    scalar_load(cluster_mem, res, size);

    return task_ret_ok;
}
//copy放在一个函数里，输入输出均在obj_mem里，脚本层面重载（metadata）
task_ret    op_cluster2sLDM_Async(void** inputs, void** outputs, void** attributes){
    obj_mem cluster_mem = (obj_mem)inputs[0];
    int_t size = (int_t)inputs[1];
    // int_t offset = (int_t)inputs[2];//该变量应该是与其他算子都不会有交互的变量，是否应该用单独的参数存储，比如attributes
    obj_mem res = (obj_mem)outputs[0];
    if(res->type < obj_mem_copying){
        if(!res->size){
            res->p = scalar_malloc(size);
            res->size = size;
            res->bufsize = size;
        }
        res->type = obj_mem_copying + scalar_load_async(cluster_mem->p, res->p, res->bufsize);
    }
    int_t chl =  res->type - obj_mem_copying;
    if(!dma_query(chl)){
        return task_ret_again;
    }
    dma_wait(chl);
    res->type = obj_mem_mt3000_dsp_sm;

    return task_ret_ok;
}

task_ret    op_dma_cluster2sLDM_Async_old(void** inputs, void** outputs, void** attributes){
    // if(get_thread_id() == 1)
    //     ddq_log0("op_dma_cluster2sLDM_Async begin\n");
    // int_t offset = (int_t)inputs[2];//该变量应该是与其他算子都不会有交互的变量，是否应该用单独的参数存储，比如attributes
    obj_mem res = (obj_mem)outputs[0];
    if(res->type < obj_mem_copying){
        obj_mem cluster_mem = (obj_mem)inputs[0];
        int_t* offset = (int_t*)inputs[1];
        int_t* params = (int_t*)inputs[2];
        int_t src_row_num = params[0];
        int_t src_row_size = params[1];
        int_t src_row_step = params[2];
        int_t dst_row_num = params[3];
        int_t dst_row_size = params[4];
        int_t dst_row_step = params[5];
        if(!res->size){
            res->size = dst_row_num * dst_row_size;
            res->p = malloc(res->size);
            res->bufsize = res->size;
        }
        // ddq_log0("op_dma_cluster2sLDM_Async with src:%p dst:%p, src_row_num:%ld, src_row_size:%ld, src_row_step:%ld, dst_row_num:%ld, dst_row_size:%ld, dst_row_step:%ld\n", 
        //         cluster_mem->p + *offset, res->p,  src_row_num, src_row_size, src_row_step, dst_row_num, dst_row_size, dst_row_step);
        // ddq_log0("op_dma_cluster2sLDM_Async with src:%p dst:%p, res->size:%ld\n", cluster_mem->p + *offset, res->p, res->size);
        res->type = obj_mem_copying + 
                    dma_p2p(cluster_mem->p + *offset, src_row_num, src_row_size, src_row_step, res->p, dst_row_num, dst_row_size, dst_row_step, 0, 0);
        // ddq_log0("dma_p2p finished\n");
    }
    int_t chl =  res->type - obj_mem_copying;
    if(!dma_query(chl)){
        return task_ret_again;
    }
    dma_wait(chl);
    res->type = obj_mem_mt3000_dsp_sm;
    // if(get_thread_id() == 1)
    //     ddq_log0("op_dma_cluster2sLDM_Async finish\n");

    return task_ret_ok;
}

task_ret    op_dma_cluster2sLDM_Async(void** inputs, void** outputs, void** attributes){
    // if(get_thread_id() == 1)
    //     ddq_log0("op_dma_cluster2sLDM_Async begin\n");
    // int_t offset = (int_t)inputs[2];//该变量应该是与其他算子都不会有交互的变量，是否应该用单独的参数存储，比如attributes
    obj_mem res = (obj_mem)outputs[0];
    obj_mem cluster_mem = (obj_mem)inputs[0];
    
    if(res->type < obj_mem_copying){
        int_t* offset = (int_t*)inputs[1];
        int_t* params = (int_t*)inputs[2];
        int_t src_row_num = params[0];
        int_t src_row_size = params[1];
        int_t src_row_step = params[2];
        int_t dst_row_num = params[3];
        int_t dst_row_size = params[4];
        int_t dst_row_step = params[5];
        if(!res->size){
            res->size = dst_row_num * dst_row_size;
            res->p = malloc(res->size);
            res->bufsize = res->size;
        }
        // ddq_log0("op_dma_cluster2sLDM_Async with src:%p dst:%p, src_row_num:%ld, src_row_size:%ld, src_row_step:%ld, dst_row_num:%ld, dst_row_size:%ld, dst_row_step:%ld\n", 
        //         cluster_mem->p + *offset, res->p,  src_row_num, src_row_size, src_row_step, dst_row_num, dst_row_size, dst_row_step);
        // ddq_log0("op_dma_cluster2sLDM_Async with src:%p dst:%p, res->size:%ld\n", cluster_mem->p + *offset, res->p, res->size);
        res->type = obj_mem_copying + 
                    dma_p2p(cluster_mem->p + *offset, src_row_num, src_row_size, src_row_step, res->p, dst_row_num, dst_row_size, dst_row_step, 0, 0);
        // ddq_log0("dma_p2p finished\n");
    }
    int_t chl =  res->type - obj_mem_copying;
    if(!dma_query(chl)){
        return task_ret_again;
    }
    dma_wait(chl);
    res->type = obj_mem_mt3000_dsp_sm;
    // if(get_thread_id() == 1)
    //     ddq_log0("op_dma_cluster2sLDM_Async finish\n");

    return task_ret_ok;
}

task_ret    op_dma_cluster2vLDM_Async(void** inputs, void** outputs, void** attributes){
    // if(get_thread_id() == 1)
    //     ddq_log0("op_dma_cluster2vLDM_Async begin\n");
    // ddq_log0("op_dma_cluster2vLDM_Async begin\n");
    // int_t offset = (int_t)inputs[2];//该变量应该是与其他算子都不会有交互的变量，是否应该用单独的参数存储，比如attributes
    obj_mem res = (obj_mem)outputs[0];
    if(res->type < obj_mem_copying){
        obj_mem cluster_mem = (obj_mem)inputs[0];
        int_t* offset = (int_t*)inputs[1];
        int_t* params = (int_t*)inputs[2];
        int_t src_row_num = params[0];
        int_t src_row_size = params[1];
        int_t src_row_step = params[2];
        int_t dst_row_num = params[3];
        int_t dst_row_size = params[4];
        int_t dst_row_step = params[5];
        if(!res->size){
            res->size = dst_row_num * dst_row_size;
            res->p = vector_malloc(res->size);
            res->bufsize = res->size;
        }
        // ddq_log0("op_dma_cluster2sLDM_Async with src:%p offset: %ld, dst:%p, src_row_num:%ld, src_row_size:%ld, src_row_step:%ld, dst_row_num:%ld, dst_row_size:%ld, dst_row_step:%ld\n", 
        //         cluster_mem->p + *offset, *offset, res->p,  src_row_num, src_row_size, src_row_step, dst_row_num, dst_row_size, dst_row_step);
        // ddq_log0("op_dma_cluster2sLDM_Async with src:%p dst:%p, res->size:%ld\n", cluster_mem->p + *offset, res->p, res->size);
        // ddq_log0("op_dma_cluster2vLDM_Async : thread %d : src: %p, offset : %ld\n", get_thread_id(), cluster_mem->p, *offset/sizeof(double));
        res->type = obj_mem_copying + 
                    dma_p2p(cluster_mem->p + *offset, src_row_num, src_row_size, src_row_step, res->p, dst_row_num, dst_row_size, dst_row_step, 0, 0);
        // ddq_log0("dma_p2p finished\n");
    }
    int_t chl =  res->type - obj_mem_copying;
    if(!dma_query(chl)){
        return task_ret_again;
    }
    dma_wait(chl);
    res->type = obj_mem_mt3000_dsp_am;
    // if(get_thread_id() == 1)
    //     ddq_log0("op_dma_cluster2vLDM_Async finish\n");

    return task_ret_ok;
}

task_ret    op_cluster2vLDM(void** inputs, void** outputs, void** attributes){
    void* cluster_mem = ACCESS_INPUTS(0, void*);
    int_t size = ACCESS_INPUTS(1, int_t);

    void* res = ACCESS_OUTPUTS(0, void*);
    if(!res){
        res = vector_malloc(size);
        WRITE_OUTPUTS(0, void*, res);
    }
    vector_load(cluster_mem, res, size);

    return task_ret_ok;
}

task_ret    op_cluster2vLDM_Async(void** inputs, void** outputs, void** attributes){
    obj_mem cluster_mem = (obj_mem)inputs[0];
    int_t step = (int_t)inputs[1];
    int_t offset = (int_t)inputs[2];//该变量应该是与其他算子都不会有交互的变量，是否应该用单独的参数存储，比如attributes
    obj_mem res = (obj_mem)outputs[0];
    if(res->type < obj_mem_copying){
        if(!res->size){
            res->p = vector_malloc(step);
            res->size = step;
            res->bufsize = step;
        }
        res->type = obj_mem_copying + vector_load_async(cluster_mem->p + offset, res->p, res->bufsize);
    }
    int_t chl =  res->type - obj_mem_copying;
    if(!dma_query(chl)){
    return task_ret_again;
    }
    dma_wait(chl);
    res->type = obj_mem_mt3000_dsp_am;
    inputs[2] += step;
    if(inputs[2] < cluster_mem->bufsize){
        return task_ret_read_partial;
    }
    

    return task_ret_ok;
}

task_ret    op_cluster2gsm_Async(void** inputs, void** outputs, void** attributes){
    obj_mem cluster_mem = (obj_mem)inputs[0];
    // int_t step = (int_t)inputs[1];
    // int_t offset = (int_t)inputs[2];//该变量应该是与其他算子都不会有交互的变量，是否应该用单独的参数存储，比如attributes
    obj_mem res = (obj_mem)outputs[0];
    if(res->type < obj_mem_copying){
        res->type = obj_mem_copying + dma_p2p(cluster_mem->p, 1, res->bufsize, 0, res->p, 1, res->bufsize, 0, 0, 0);
    }
    int_t chl =  res->type - obj_mem_copying;
    if(!dma_query(chl)){
    return task_ret_again;
    }
    dma_wait(chl);
    res->type = obj_mem_mt3000_dsp_gsm;
    // inputs[2] += step;
    // if(inputs[2] < cluster_mem->bufsize){
    //     return task_ret_read_partial;
    // }
    
    return task_ret_ok;
}

task_ret    op_dma_cluster2gsm_Async(void** inputs, void** outputs, void** attributes){
    // ddq_log0("op_dma_cluster2gsm_Async begin\n");
    // int_t step = (int_t)inputs[1];
    // int_t offset = (int_t)inputs[2];//该变量应该是与其他算子都不会有交互的变量，是否应该用单独的参数存储，比如attributes
    int thread_id = get_thread_id();
    obj_mem res = (obj_mem)outputs[0];
    if(res->type < obj_mem_copying){
        obj_mem cluster_mem = (obj_mem)inputs[0];
        int_t* offset = (int_t*)inputs[1];
        int_t* params = (int_t*)inputs[2];
        int_t src_row_num = params[0];
        int_t src_row_size = params[1];
        int_t src_row_step = params[2];
        int_t dst_row_num = params[3];
        int_t dst_row_size = params[4];
        int_t dst_row_step = params[5];
        // Agsm[0] = 3.14;
        // ddq_log0("op_dma_cluster2gsm_Async with src:%p dst:%p, res->size:%ld Agsm:%p %f\n", cluster_mem->p + *offset, res->p, res->size, Agsm, Agsm[0]);
        if(!res->size){
            res->size = dst_row_num * dst_row_size;
            res->p = Agsm;
            res->bufsize = res->size;
            // signals_gsm[thread_id] = !thread_id;
        }
        // ddq_log0("op_dma_cluster2gsm_Async with src:%p dst:%p, res->size:%ld, res->p[0]:%f\n", cluster_mem->p + *offset, res->p, res->size, ((double*)res->p)[0]);

        if(thread_id == 0){
            // int ready = 1;
            // for(int i=1; i<get_group_size(); i++){
            //     if(symbols_gsm[thread_id] == symbols_gsm[0]){
            //         return task_ret_again;
            //     }
            // }
            // ddq_log0("op_dma_cluster2gsm_Async : thread %d : src: %p, offset : %ld\n", get_thread_id(), cluster_mem->p, *offset/sizeof(double));
            res->type = obj_mem_copying + 
                        dma_p2p(cluster_mem->p + *offset, src_row_num, src_row_size, src_row_step, res->p, dst_row_num, dst_row_size, dst_row_step, 0, 0);
        }else{
            res->type = obj_mem_copying;
        }
    }
        //为什么一直是to_write
    if(thread_id == 0){
        int_t chl =  res->type - obj_mem_copying;
        // ddq_log0("op_dma_cluster2gsm_Async with src:%ld\n", chl);
        if(dma_query(chl)){
            dma_wait(chl);
            res->type = obj_mem_mt3000_dsp_gsm;
            signals_gsm[thread_id] = !signals_gsm[thread_id];
            // ddq_log0("thread_id 0 after dma_query: signals_gsm[%d] = %ld\n", thread_id, signals_gsm[thread_id]);
        // ddq_log0("op_dma_cluster2gsm_Async with symbol: %ld %ld\n", symbols_gsm[thread_id], symbols_gsm[1]);
        }
    }else{
        if(signals_gsm[0] == signals_gsm[thread_id]){
            res->type = obj_mem_mt3000_dsp_gsm;
            // ddq_log0("after dma_query: signals_gsm[%d] = %ld\n", thread_id, signals_gsm[thread_id]);
            signals_gsm[thread_id] = !signals_gsm[thread_id];
        }
    }

    if(res->type == obj_mem_mt3000_dsp_gsm){
        // if(get_thread_id() == 1)
            // ddq_log0("obj_mem_mt3000_dsp_gsm get %d\n", get_thread_id());
        return task_ret_ok;
    }
    return task_ret_again;
    // return task_ret_ok;
}

task_ret    op_dma_gsm2sLDM_Async(void** inputs, void** outputs, void** attributes){
    // if(get_thread_id() == 1)
    //     ddq_log0("op_dma_gsm2sLDM_Async begin\n");
    // int_t step = (int_t)inputs[1];
    // int_t offset = (int_t)inputs[2];//该变量应该是与其他算子都不会有交互的变量，是否应该用单独的参数存储，比如attributes
    obj_mem res = (obj_mem)outputs[0];
    if(res->type < obj_mem_copying){
        obj_mem cluster_mem = (obj_mem)inputs[0];
        int_t* offset = (int_t*)inputs[1];
        int_t* params = (int_t*)inputs[2];
        int_t src_row_num = params[0];
        int_t src_row_size = params[1];
        int_t src_row_step = params[2];
        int_t dst_row_num = params[3];
        int_t dst_row_size = params[4];
        int_t dst_row_step = params[5];
        if(!res->size){
            res->size = dst_row_num * dst_row_size;
            res->p = malloc(res->size);
            res->bufsize = res->size;
        }
        // ddq_log0("op_dma_gsm2sLDM_Async : thread %d : offset : %ld\n", get_thread_id(), *offset/sizeof(double));
        res->type = obj_mem_copying + 
                    dma_p2p(cluster_mem->p + *offset, src_row_num, src_row_size, src_row_step, res->p, dst_row_num, dst_row_size, dst_row_step, 0, 0);
    }
    int_t chl =  res->type - obj_mem_copying;
    if(!dma_query(chl)){ //BUG
        // ddq_log0("thread_id: %d dma_query chl %ld failed! \n",get_thread_id(), chl);
        return task_ret_again;
    }
    // ddq_log0("thread_id: %d dma_query chl %ld success! \n",get_thread_id(), chl);
    dma_wait(chl);
    res->type = obj_mem_mt3000_dsp_sm;

    // if(get_thread_id() == 1)
    //     ddq_log0("op_dma_gsm2sLDM_Async finish\n");

    return task_ret_ok;
}

task_ret    op_sLDM2cluster(void** inputs, void** outputs, void** attributes){
    obj_mem ldm_mem = (obj_mem)inputs[0];
    int_t size = (int_t)inputs[1];

    obj_mem res = (obj_mem)outputs[0];

    scalar_store(ldm_mem->p, res->p, res->bufsize);

    res->type = obj_mem_mt3000_dev_mem;

    return task_ret_ok;
}

task_ret    op_sLDM2cluster_Async(void** inputs, void** outputs, void** attributes){
    obj_mem ldm_mem = (obj_mem)inputs[0];
    int_t size = (int_t)inputs[1];

    obj_mem res = (obj_mem)outputs[0];

    if(res->type < obj_mem_copying){
        res->type = obj_mem_copying + scalar_store_async(ldm_mem->p, res->p, res->bufsize);
    }
    int_t chl =  res->type - obj_mem_copying;
    if(!dma_query(chl)){
        return task_ret_again;
    }
    dma_wait(chl);
    res->type = obj_mem_mt3000_dev_mem;

    return task_ret_ok;
}

task_ret    op_dma_sLDM2cluster_Async(void** inputs, void** outputs, void** attributes){
    // int_t offset = (int_t)inputs[2];//该变量应该是与其他算子都不会有交互的变量，是否应该用单独的参数存储，比如attributes
    obj_mem res = (obj_mem)outputs[0];
    if(res->type < obj_mem_copying){
        obj_mem buf = (obj_mem)inputs[0];
        int_t* offset = (int_t*)inputs[1];
        int_t* params = (int_t*)inputs[2];
        int_t src_row_num = params[0];
        int_t src_row_size = params[1];
        int_t src_row_step = params[2];
        int_t dst_row_num = params[3];
        int_t dst_row_size = params[4];
        int_t dst_row_step = params[5];
        // if(!res->size){
        //     res->size = dst_row_num * dst_row_size;
        //     res->p = scalar_malloc(res->size);
        //     res->bufsize = res->size;
        // }
        // ddq_log0("op_dma_sLDM2cluster_Async with src:%p dst:%p, %ld\n", buf->p, res->p + *offset, dst_row_num * dst_row_size);
        // ddq_log0("op_dma_sLDM2cluster_Async : thread %d : dst: %p, offset : %ld\n", get_thread_id(), res->p, *offset/sizeof(double));
        res->type = obj_mem_copying + 
                    dma_p2p(buf->p, src_row_num, src_row_size, src_row_step, res->p + *offset, dst_row_num, dst_row_size, dst_row_step, 0, 0);
    }
    int_t chl =  res->type - obj_mem_copying;
    if(!dma_query(chl)){
        return task_ret_again;
    }
    dma_wait(chl);
    res->type = obj_mem_mt3000_dev_mem;

    return task_ret_ok;
}

task_ret    op_vLDM2cluster(void** inputs, void** outputs, void** attributes){
    obj_mem ldm_mem = (obj_mem)inputs[0];
    int_t size = (int_t)inputs[1];

    obj_mem res = (obj_mem)outputs[0];

    vector_store(ldm_mem->p, res->p, res->bufsize);

    res->type = obj_mem_mt3000_dev_mem;

    return task_ret_ok;
}

task_ret    op_vLDM2cluster_Async(void** inputs, void** outputs, void** attributes){
    obj_mem ldm_mem = (obj_mem)inputs[0];
    int_t size = (int_t)inputs[1];

    obj_mem res = (obj_mem)outputs[0];

    if(res->type < obj_mem_copying){
        res->type = obj_mem_copying + vector_store_async(ldm_mem->p, res->p, res->bufsize);
    }
    int_t chl =  res->type - obj_mem_copying;
    if(!dma_query(chl)){
        return task_ret_again;
    }
    dma_wait(chl);
    res->type = obj_mem_mt3000_dev_mem;

    return task_ret_ok;
}

task_ret op_dma_query(void** inputs, void** outputs, void** attributes){
    int chl = *ACCESS_INPUTS(0, int*);
    // ddq_log0("dma_query chl:%d %d\n",chl, get_core_id());
    if(!dma_query(chl)){
        return task_ret_again;
    }
    dma_wait(chl);
    return task_ret_ok;
}