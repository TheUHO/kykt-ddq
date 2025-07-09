#include    "error.h"
#include    "basic_types.h"
#include    "hthread_host.h"

//TODO 如何存放全局变量，让interrupt找到对应的mem
static uint_t* mem;
static uint_t mem_len;
static void* w_mem;
void interrupt_func_read_w_mem_thread_group_0(int id, unsigned long val){
    int thread_group_id = 0;
    mt_inv(w_mem, (W_BUF_SIZE * (sizeof(w_buf_t) + ELE_SIZE * ELEMENTS_PER_BLOCK)));
    uint_t i;
    //需要处理尾巴问题
    for(i=0; i<W_BUF_SIZE; i++){
        w_buf_t buf = ((w_buf_t*)w_mem)[i];
        if(buf.block_idx != (uint_t)-1){
            uint_t element_count = (buf.block_idx == mem_len/ELEMENTS_PER_BLOCK)?(len%ELEMENTS_PER_BLOCK):ELEMENTS_PER_BLOCK;
            memcpy((void*)mem + buf.block_idx*ELE_SIZE*ELEMENTS_PER_BLOCK, (void*)w_mem + buf.data_shift, ELE_SIZE * element_count);
            mt_flush((void*)mem + buf.block_idx*ELE_SIZE*ELEMENTS_PER_BLOCK, ELE_SIZE * element_count);
        }
    }
    hthread_intr_send(thread_group_id, id, val);
}

void write_cache_op(void**inputs, void**outputs){
    int thread_id = inputs[0];
    uint_t w_buf_len = (W_BUF_SIZE * (sizeof(w_buf_t) + ELE_SIZE * ELEMENTS_PER_BLOCK));
    mem = inputs[1];
    w_mem = inputs[2];
    mem_len = inputs[3];

    hthread_handler_register(thread_id, interrupt_func_read_w_mem_thread_group_0);
}