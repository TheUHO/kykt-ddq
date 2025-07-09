#ifndef OP_CACHE_H
#define OP_CACHE_H

#include    "basic_types.h"
#include    "error.h"

#define INTERRUPT_NUM 20

//数据需要按照ELE_SIZE对齐，此为数据的最小单位，BLOCK是CACHE的交换单位
#define ELE_SIZE 8          // Define the size of each element in bytes
#define ELEMENTS_PER_BLOCK 8   // Define the number of elements per block 
#define BLOCKS_PER_CACHELINE 2  // Define the number of blocks per cache line 
#define CACHELINE_COUNT 2       // Define the number of cache lines 
#define W_BUF_SIZE 1           // Define the size of the write buffer in blocks

// 定义一个字节中的位数
#define BITS_PER_BYTE 8
// 根据给定的符号位定义每个字中的符号数量
#define SYMBOLS_PER_WORD(SYMBOL_SIZE) (sizeof(void*)*BITS_PER_BYTE/SYMBOL_SIZE)
// 定义缓存符号的位宽为2位
#define CACHE_SYMBOL_SIZE 2
// 定义cache_w符号的位宽为1位
#define CACHE_W_SYMBOL_SIZE 1

#define DMA_COUNT 16
typedef enum{
    cache_symbol_invalid = 0,
    cache_symbol_valid,
    cache_symbol_updating   //waiting for update confirmation. eg. dma wait.
}cache_symbol_t;

typedef enum{
    cache_symbol_clean = 0,
    cache_symbol_dirty
}cache_symbol_w_t;

typedef enum{
    dma_tag_idle = 0,
    dma_tag_w_buf,
    dma_tag_load_block,
    dma_tag_store_block 
}dma_tag_t;

typedef enum{
    w_signal_idle = -2,
    w_signal_mem_to_read = -1,
    w_signal_dma_chl_shift = 0
}w_signal_t;

typedef	struct cache_info_t cache_info_t;

struct cache_info_t{
    uint_t elements_per_block;
    uint_t blocks_per_cacheline;
    uint_t cacheline_count;
    uint_t ele_size;
    uint_t symbols_size;
    uint_t mem_size;
    void* mem;
    void* buf;
    uint_t* tags;
    uint_t* symbols;
    //MT_LRU
    uint_t* LRUsymbols;
    //func ptr
    //TODO: 函数指针
    void (*autoPrefetch_ptr)(cache_info_t* cache_info);
    void (*prefetch_ptr)(cache_info_t* cache_info, unsigned long* src_blkIdx_array, unsigned long idx_mum);
    void* (*loadDataFromBufOnlyRead_ptr)(cache_info_t* cache_info, unsigned long idx);
};

typedef	struct cache_info_r_t cache_info_r_t; 

struct cache_info_r_t{
    cache_info_t cache_info_basic;
};

typedef	struct cache_info_w_t cache_info_w_t; 

typedef	struct w_buf_t w_buf_t;
struct w_buf_t{
    uint_t data_shift;
    uint_t block_idx;
    w_buf_t* next; 
}; 

struct cache_info_w_t{
    cache_info_t cache_info_basic;
    uint_t* w_symbols;
    uint_t w_buf_size;
    void* w_mem;
    // ｜w_but_t segment｜data segment｜
    void* w_buf;
    w_buf_t* w_buf_free_list;
    w_buf_t* w_buf_used_list;
    int_t w_signal;
};


// __global__ task_ret op_copy_cached(void** input, void** output);

//TODO: 预取函数
// void autoPrefetch(cache_info_t* cache_info);

// void prefetch(cache_info_t* cache_info, unsigned long* src_blkIdx_array, unsigned long idx_mum);

void *	cache_info_r_new();

int cache_info_r_del(void* cache_info);

void *	cache_info_w_new();

int cache_info_w_del(void* cache_info);

void* loadDataFromBufOnlyRead(cache_info_t* cache_info, unsigned long idx);

void* loadDataFromBufOnlyReadAsync(cache_info_t* cache_info, unsigned long idx);

int storeDataToBuf(cache_info_t* cache_info, unsigned long idx, void* data);

int storeDataToBufAsync(cache_info_t* cache_info, unsigned long idx, void* data);

#endif