#include "op_cache.h"
#include "hthread_device.h"
#include	"task_types.h"
__global__ task_ret op_copy_cached(void** input, void** output){
    hthread_printf("op_copy_cached begin!");
    //TODO:可能需要虚实地址转换，存疑实验一下，之前没考虑到
    void* mem_ptr = input[0];
    uint_t mem_len = (uint_t)(input[1]);

    cache_info_t* cache_info = output[0];
    cache_info->mem = mem_ptr;
    cache_info->mem_size = mem_len;
    // cache_info->autoPrefetch_ptr = autoPrefetch;
    // cache_info->prefetch_ptr = prefetch;
    cache_info->loadDataFromBufOnlyRead_ptr = loadDataFromBufOnlyReadAsync;

    return task_ret_done;
}