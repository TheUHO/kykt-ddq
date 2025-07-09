#include "op_cache.h"
#include "hthread_device.h"
#include	<stdio.h>		
#include	<stdlib.h>
#include "basic_types.h"
#include <string.h>

//===----------------------------------------------------------------------===//
// common variable and functions
//===----------------------------------------------------------------------===//

#define intr_w_signal_turn_idle_register(num) intr_w_signal_turn_idle_register_(num)

#define intr_w_signal_turn_idle_register_(num) intr_handler_register(INTERRUPT_NUM + num++, interrupt_func_w_signal_turn_idle_##num)

static struct{
    cache_info_t* cache_info;
    int_t block_idx;
    dma_tag_t dma_tag;
} DMA_flags[DMA_COUNT];

static uint_t interrupt_num = 0;


//TODO
static cache_info_w_t cache_info_w_global[4];

void interrupt_func_w_signal_turn_idle_0(){
    cache_info_w_global[0]->w_signal = w_signal_idle;
}
void interrupt_func_w_signal_turn_idle_1(){
    cache_info_w_global[1]->w_signal = w_signal_idle;
}

void interrupt_func_w_signal_turn_idle_2(){
    cache_info_w_global[2]->w_signal = w_signal_idle;
}

void interrupt_func_w_signal_turn_idle_3(){
    cache_info_w_global[3]->w_signal = w_signal_idle;
}
//...

//dma_query
int my_dma_query(unsigned int chl){
    unsigned long * dma_ptr = 0x4001dffd0;
    unsigned int check_bit;
    if(chl == 15){
        check_bit = 1 << (chl+5);
    }else{
        check_bit = 1 << chl;
    }
    
    if(dma_ptr[0] & check_bit){
        //idle
        return 0;
    }
    else{
        // busy
        return 1;
    }
}

//inline functions

// 用于修改缓存信息中的某个块的状态
// 参数：
//     symbols: 指向缓存状态数组的指针
//     block_idx: 要修改的块的索引
//     bit_value: 要设置的状态值
//     bit_size: 状态位的位数
static inline void modify_block_status(uint_t* symbols, uint_t block_idx, uint_t bit_value, uint_t bit_size) {
    // hthread_printf( "modify_block_status(%p, %ld, %ld, %ld)\n", symbols, block_idx, bit_value, bit_size );
    // 计算块在单词中的位置
    uint_t word_idx = block_idx / SYMBOLS_PER_WORD(bit_size);
    // 计算块的状态位在单词中的位移量
    uint_t bit_shift = block_idx % SYMBOLS_PER_WORD(bit_size) * bit_size;
    // 计算掩码，用于清除目标位区间的值
    uint_t mask = (~((-1L) << bit_size)) << bit_shift;
    // uint_t a = 3;
    // hthread_printf("mask:%d,  %llx, %llx\n", sizeof(uint_t),mask, a<<32);
    // 将目标位区间的值设置为新的值
    symbols[word_idx] = (symbols[word_idx] & ~mask) | (bit_value << bit_shift) & mask;
    // hthread_printf("word_idx : %ld, bit_shift: %ld, mask: %llx\n", word_idx, bit_shift, mask);
    // hthread_printf("symbols[%ld]: %llx\n", word_idx, symbols[word_idx]);
}

// 用于查看缓存信息中的某个块的状态
// 参数：
//     symbols: 指向缓存状态数组的指针
//     block_idx: 要修改的块的索引
//     bit_size: 状态位的位数
static inline uint_t get_block_status(uint_t* symbols, uint_t block_idx, uint_t bit_size) {
    // 计算块在单词中的位置
    uint_t word_idx = block_idx / SYMBOLS_PER_WORD(bit_size);
    // 计算块的状态位在单词中的位移量
    uint_t bit_shift = block_idx % SYMBOLS_PER_WORD(bit_size) * bit_size;
    // 计算掩码，用于获取目标位区间的值
    uint_t mask = (~((-1L) << bit_size)) << bit_shift;
    // 获取目标位区间的值
    return (symbols[word_idx] & mask) >> bit_shift;
}

static inline int MT_loadBlock(cache_info_t* cache_info, unsigned long mem_blockIdx, unsigned long buf_blockIdx){
    void* buf = cache_info->buf + buf_blockIdx * cache_info->elements_per_block * cache_info->ele_size;
    void* mem = cache_info->mem + mem_blockIdx * cache_info->elements_per_block * cache_info->ele_size;
    return scalar_load(mem, buf, cache_info->elements_per_block * cache_info->ele_size);
}

static inline int MT_storeBlock(cache_info_t* cache_info, unsigned long buf_blockIdx, unsigned long mem_blockIdx){
    void* buf = cache_info->buf + buf_blockIdx * cache_info->elements_per_block * cache_info->ele_size;
    void* mem = cache_info->mem + mem_blockIdx * cache_info->elements_per_block * cache_info->ele_size;
    return scalar_store(buf, mem, cache_info->elements_per_block * cache_info->ele_size);
}

static inline int MT_loadBlockAsync(cache_info_t* cache_info, unsigned long mem_blockIdx, unsigned long buf_blockIdx){
    void* buf = cache_info->buf + buf_blockIdx * cache_info->elements_per_block * cache_info->ele_size;
    void* mem = cache_info->mem + mem_blockIdx * cache_info->elements_per_block * cache_info->ele_size;
    int chl = scalar_load_async(mem, buf, cache_info->elements_per_block * cache_info->ele_size);
    if(chl == -1) return -1;
    DMA_flags[chl].dma_tag = dma_tag_load_block;
    DMA_flags[chl].block_idx = buf_blockIdx;
    DMA_flags[chl].cache_info = cache_info;
    return chl;
}

static inline int MT_storeBlockAsync(cache_info_t* cache_info, unsigned long buf_blockIdx, unsigned long mem_blockIdx){
    void* buf = cache_info->buf + buf_blockIdx * cache_info->elements_per_block * cache_info->ele_size;
    void* mem = cache_info->mem + mem_blockIdx * cache_info->elements_per_block * cache_info->ele_size;
    int chl = scalar_store_async(buf, mem, cache_info->elements_per_block * cache_info->ele_size);
    if(chl == -1) return -1;
    DMA_flags[chl].dma_tag = dma_tag_store_block;
    DMA_flags[chl].block_idx = buf_blockIdx;
    DMA_flags[chl].cache_info = cache_info;
    return chl;
}

static inline int MT_writeBuffer(cache_info_w_t* cache_info){
    hthread_printf("Writing buffer %p\n", cache_info->w_buf);
    return scalar_store(cache_info->w_buf, cache_info->w_mem, 
                        cache_info->w_buf_size * 
                        (cache_info->cache_info_basic.elements_per_block * cache_info->cache_info_basic.ele_size + sizeof(w_buf_t)));
}

static inline int MT_writeBufferAsync(cache_info_w_t* cache_info){
    int chl = scalar_store_async(cache_info->w_buf, cache_info->w_mem, 
                        cache_info->w_buf_size * 
                        (cache_info->cache_info_basic.elements_per_block * cache_info->cache_info_basic.ele_size + sizeof(w_buf_t)));
    DMA_flags[chl].cache_info = (cache_info_t*)cache_info;
    DMA_flags[chl].dma_tag = dma_tag_w_buf;
    return chl;
}

void initLRU(cache_info_t* cache_info) {
    uint_t i, j;
    for(i = 0; i < cache_info->cacheline_count; i++){
        for (j = 0; j < cache_info->blocks_per_cacheline; j++) {
            cache_info->LRUsymbols[i * cache_info->blocks_per_cacheline + j] = j;
        }
    }
}

// LRU 更新块的优先级
void updateLRU(cache_info_t* cache_info, uint_t dst_blockIdx) {
    uint_t i;
    uint_t idx = (dst_blockIdx / cache_info->blocks_per_cacheline) * cache_info->blocks_per_cacheline;
    for (i = 0; i < cache_info->blocks_per_cacheline; i++) {
        if (cache_info->LRUsymbols[i + idx] < cache_info->LRUsymbols[dst_blockIdx]) {
            cache_info->LRUsymbols[i + idx] += 1;
        }
    }
    cache_info->LRUsymbols[dst_blockIdx] = 0;
}

// 计算块索引和偏移量
static inline void calculateBlockOffsetAndIndex(cache_info_t* cache_info, unsigned long idx, uint_t* offset, uint_t* blockIdx, uint_t* tag, uint_t* lineIdx) {
    *offset = idx % cache_info->elements_per_block;
    *blockIdx = idx/cache_info->elements_per_block;
    *tag = *blockIdx / cache_info->blocks_per_cacheline;
    *lineIdx = *blockIdx % cache_info->blocks_per_cacheline;
}

void dma_update(){
    uint_t i;
    int_t dma_tag; 
    for (i = 0; i < DMA_COUNT; i++) {
        dma_tag = DMA_flags[i].dma_tag;
        if (dma_tag == dma_tag_idle) continue;
        if (my_dma_query(i)) continue;
        dma_wait(i);
        DMA_flags[i].dma_tag = dma_tag_idle;
        switch (dma_tag)
        {
        case dma_tag_w_buf:
            ((cache_info_w_t*)(DMA_flags[i].cache_info))->w_signal = w_signal_mem_to_read;
            break;
        case dma_tag_load_block:
            modify_block_status(DMA_flags[i].cache_info->symbols, DMA_flags[i].block_idx, cache_symbol_valid, CACHE_SYMBOL_SIZE);
            break;
        default:
            break;
        }
    }
            
}

uint_t getReplaceBlock(cache_info_t* cache_info, uint_t lineIdx){
    uint_t idx = lineIdx * cache_info->blocks_per_cacheline;
    uint_t cache_block_status;
    uint_t dst_blockIdx;
    uint_t i = 0;
    for(i = 0; i < cache_info->blocks_per_cacheline; i++){
        dst_blockIdx = idx + i;
        cache_block_status = get_block_status(cache_info->symbols, dst_blockIdx, CACHE_SYMBOL_SIZE);
        //invalid || 优先级最低需要被替换
        if(cache_block_status == cache_symbol_invalid || cache_info->LRUsymbols[dst_blockIdx] == cache_info->blocks_per_cacheline - 1){
            return dst_blockIdx;
        }
    }
    hthread_printf("cache LRU wrong!!\n");
    //TODO: 错误处理
    return ULONG_MAX;
}

int checkCacheHit(cache_info_t* cache_info, uint_t tag, uint_t lineIdx, uint_t* dst_blockIdx){
    uint_t idx = lineIdx * cache_info->blocks_per_cacheline;
    uint_t blockIdx;
    uint_t cache_block_status;
    uint_t i = 0;
    for(i = 0; i < cache_info->blocks_per_cacheline; i++){
        blockIdx = idx + i;
        cache_block_status = get_block_status(cache_info->symbols, blockIdx, CACHE_SYMBOL_SIZE);
        //hit (valid & tag hit)
        if( (cache_info->tags[blockIdx] == tag) && 
            !(cache_block_status == cache_symbol_invalid)){ 
            *dst_blockIdx = blockIdx;
            return 0;
        }
    }
    return -1;
}

//assert : BLOCKS_PER_CACHELINE * CACHELINE_COUNT % SYMBOLS_PER_BYTE = 0
static inline void* cache_info_new(uint_t size, uint_t cache_info_size){

    size += (ELE_SIZE * ELEMENTS_PER_BLOCK * BLOCKS_PER_CACHELINE * CACHELINE_COUNT);   //buf_ptr
    size += (BLOCKS_PER_CACHELINE * CACHELINE_COUNT * sizeof(unsigned long));   //tags
    size += ((BLOCKS_PER_CACHELINE * CACHELINE_COUNT / SYMBOLS_PER_WORD(CACHE_SYMBOL_SIZE) + 
            ((BLOCKS_PER_CACHELINE * CACHELINE_COUNT % SYMBOLS_PER_WORD(CACHE_SYMBOL_SIZE)) ? 1 : 0)) * sizeof(uint_t));        // symbols
    //LRU
    size += (BLOCKS_PER_CACHELINE * CACHELINE_COUNT * sizeof(unsigned long));   //LRUsymbols

    cache_info_t* res = malloc(size);

    res->elements_per_block = ELEMENTS_PER_BLOCK;
    res->blocks_per_cacheline = BLOCKS_PER_CACHELINE;
    res->cacheline_count = CACHELINE_COUNT;
    res->ele_size = ELE_SIZE;
    //TODO
    res->symbols_size = (BLOCKS_PER_CACHELINE * CACHELINE_COUNT / SYMBOLS_PER_WORD(CACHE_SYMBOL_SIZE)) + 
            ((BLOCKS_PER_CACHELINE * CACHELINE_COUNT % SYMBOLS_PER_WORD(CACHE_SYMBOL_SIZE)) ? 1 : 0);

    res->mem = NULL;
    res->mem_size = 0;
    res->buf = (void*)res + cache_info_size;
    res->tags = res->buf + ELE_SIZE * ELEMENTS_PER_BLOCK * BLOCKS_PER_CACHELINE * CACHELINE_COUNT;
    res->symbols = (void*)(res->tags) + BLOCKS_PER_CACHELINE * CACHELINE_COUNT * sizeof(unsigned long);
    res-> LRUsymbols = (void*)(res->symbols) + res->symbols_size * sizeof(uint_t);

    //init data
    uint_t i = 0;
    for(i = 0; i < res->symbols_size; i++){
        res->symbols[i] = 0;
    }

    initLRU(res);

    return (void*)res;
}

void cache_info_del(void *p){
    free(p);
}

//===----------------------------------------------------------------------===//
// cache_info_r
//===----------------------------------------------------------------------===//

void *	cache_info_r_new(){
    uint_t size = sizeof(cache_info_r_t);
    void* res = cache_info_new(size, sizeof(cache_info_r_t));
    return res;
}

void cache_info_r_del(void* cache_info){
    cache_info_del(cache_info);
}

void* loadDataFromBufOnlyRead(cache_info_t* cache_info, unsigned long idx){
    if(idx >= cache_info->mem_size){
        hthread_printf("idx out of range\n");
        return NULL;
    }
    uint_t offset;
    uint_t blockIdx;
    uint_t tag;
    uint_t lineIdx;
    uint_t dst_blockIdx; //命中或者要替换的块索引
    uint_t cache_block_status;
    
    calculateBlockOffsetAndIndex(cache_info, idx, &offset, &blockIdx, &tag, &lineIdx);
    if(checkCacheHit(cache_info, tag, lineIdx, &dst_blockIdx) != -1){
        updateLRU(cache_info, dst_blockIdx);
        return cache_info->buf + (dst_blockIdx * cache_info->elements_per_block + offset) * cache_info->ele_size;
    }

    //not hit
    //获取要被写入的块
    dst_blockIdx = getReplaceBlock(cache_info, lineIdx);
    modify_block_status(cache_info->symbols, dst_blockIdx, cache_symbol_invalid, CACHE_SYMBOL_SIZE);

    //dma load from mem to cache buffer
    while(MT_loadBlock(cache_info, blockIdx, dst_blockIdx) == -1){
        hthread_printf("DMA load block failed\n");
    }

    cache_info->tags[dst_blockIdx] = tag;
    modify_block_status(cache_info->symbols, dst_blockIdx, cache_symbol_valid, CACHE_SYMBOL_SIZE);

    //LRU 调整块的优先级
    updateLRU(cache_info,dst_blockIdx);

    return cache_info->buf + (dst_blockIdx * cache_info->elements_per_block + offset) * cache_info->ele_size;
} 

void* loadDataFromBufOnlyReadAsync(cache_info_t* cache_info, unsigned long idx){
    if(idx >= cache_info->mem_size){
        hthread_printf("idx out of range\n");
        return NULL;
    }
    uint_t offset;
    uint_t blockIdx;
    uint_t tag;
    uint_t lineIdx;
    uint_t chl;
    uint_t dst_blockIdx; //命中或者要替换的块索引
    uint_t cache_block_status;
    
    calculateBlockOffsetAndIndex(cache_info, idx, &offset, &blockIdx, &tag, &lineIdx);
    if(checkCacheHit(cache_info, tag, lineIdx, &dst_blockIdx) != -1){
        updateLRU(cache_info, dst_blockIdx);

        if(get_block_status(cache_info->symbols, dst_blockIdx, CACHE_SYMBOL_SIZE) == cache_symbol_valid){
            return cache_info->buf + (dst_blockIdx * cache_info->elements_per_block + offset) * cache_info->ele_size;
        }
        dma_update();
        if(get_block_status(cache_info->symbols, dst_blockIdx, CACHE_SYMBOL_SIZE) == cache_symbol_valid){
            return cache_info->buf + (dst_blockIdx * cache_info->elements_per_block + offset) * cache_info->ele_size;
        }
        return NULL;
    }

    //not hit
    //获取要被写入的块
    dst_blockIdx = getReplaceBlock(cache_info, lineIdx);

    //dma load from mem to cache buffer
    if(MT_loadBlockAsync(cache_info, blockIdx, dst_blockIdx) == -1){
        hthread_printf("DMA load block async failed, maybe dma channels are all busy!\n");
        return NULL;
    }
    
    cache_info->tags[dst_blockIdx] = tag;
    modify_block_status(cache_info->symbols, dst_blockIdx, cache_symbol_updating, CACHE_SYMBOL_SIZE);

    updateLRU(cache_info, dst_blockIdx);
    
    return NULL;
} 

//===----------------------------------------------------------------------===//
// cache_info_w
//===----------------------------------------------------------------------===//
void writeDirtyBlockToBuffer(cache_info_w_t* cache_info, uint_t dst_blockIdx){
    if(cache_info->w_buf_free_list == NULL){//w_buffer full
        //需要CPU同步
        while(MT_writeBuffer(cache_info) == -1){
            hthread_printf("write buffer is failed\n");
        }
        cache_info->w_signal = w_signal_mem_to_read;
        cpu_interrupt(INTERRUPT_NUM);
        while(cache_info->w_signal != w_signal_idle){
            hthread_printf("waiting for w_buf to be written to memory\n");
        }
        cache_info->w_buf_free_list = cache_info->w_buf_used_list;
        cache_info->w_buf_used_list = NULL;
    }
    //写入buffer
    w_buf_t* w_buf_node = cache_info->w_buf_free_list;
    memcpy(cache_info->w_buf + w_buf_node->data_shift, 
            cache_info->cache_info_basic.buf + dst_blockIdx * cache_info->cache_info_basic.elements_per_block * cache_info->cache_info_basic.ele_size, 
            cache_info->cache_info_basic.elements_per_block * cache_info->cache_info_basic.ele_size);
    cache_info->w_buf_free_list = w_buf_node->next;
    w_buf_node->next = cache_info->w_buf_used_list;
    w_buf_node->block_idx = cache_info->cache_info_basic.tags[dst_blockIdx] * cache_info->cache_info_basic.blocks_per_cacheline + dst_blockIdx / cache_info->cache_info_basic.blocks_per_cacheline;
    cache_info->w_buf_used_list = w_buf_node;
}

int writeDirtyBlockToBufferAsync(cache_info_w_t* cache_info, uint_t dst_blockIdx){
    uint_t chl;
    int_t w_signal;
    //DSP:w_buf->w_mem CPU:w_mem->other
    w_signal = cache_info->w_signal;
    if(cache_info->w_buf_free_list == NULL){//w_buffer full
        //需要CPU同步
        if(w_signal == w_signal_mem_to_read) return -1;

        if(w_signal >= w_signal_dma_chl_shift){
            if(my_dma_query(chl)) return -1;
            dma_wait(chl);
            cache_info->w_signal = w_signal_mem_to_read;
            cpu_interrupt(INTERRUPT_NUM);
            cache_info->w_buf_free_list = cache_info->w_buf_used_list;
            cache_info->w_buf_used_list = NULL;
        } else {
            if((chl = MT_writeBufferAsync(cache_info)) == -1){
                hthread_printf("write buffer is failed, maybe dma channels are all busy.\n");
                return -1;
            }
            cache_info->w_signal = w_signal_dma_chl_shift + chl;
            return -1;
        }
    }
    //写入buffer
    w_buf_t* w_buf_node = cache_info->w_buf_free_list;
    memcpy(cache_info->w_buf + w_buf_node->data_shift, 
            cache_info->cache_info_basic.buf + dst_blockIdx * cache_info->cache_info_basic.elements_per_block * cache_info->cache_info_basic.ele_size, 
            cache_info->cache_info_basic.elements_per_block * cache_info->cache_info_basic.ele_size);
    cache_info->w_buf_free_list = w_buf_node->next;
    w_buf_node->next = cache_info->w_buf_used_list;
    w_buf_node->block_idx = cache_info->cache_info_basic.tags[dst_blockIdx] * cache_info->cache_info_basic.blocks_per_cacheline + dst_blockIdx / cache_info->cache_info_basic.blocks_per_cacheline;
    cache_info->w_buf_used_list = w_buf_node;
    return 0;
}

void *	cache_info_w_new(){
    uint_t size = sizeof(cache_info_w_t);
    //write symbol
    size += ((BLOCKS_PER_CACHELINE * CACHELINE_COUNT / SYMBOLS_PER_WORD(CACHE_W_SYMBOL_SIZE) + 
            ((BLOCKS_PER_CACHELINE * CACHELINE_COUNT % SYMBOLS_PER_WORD(CACHE_W_SYMBOL_SIZE)) ? 1 : 0)) * sizeof(uint_t));        // symbols_w
    //w_buf + buf_data
    size += (W_BUF_SIZE * (sizeof(w_buf_t) + ELE_SIZE * ELEMENTS_PER_BLOCK));

    cache_info_w_t* res = (cache_info_w_t*)cache_info_new(size, sizeof(cache_info_w_t));
    
    res->w_buf = res->cache_info_basic.LRUsymbols + BLOCKS_PER_CACHELINE * CACHELINE_COUNT * sizeof(unsigned long);
    res->w_symbols = res->w_buf + (W_BUF_SIZE * (sizeof(w_buf_t) + ELE_SIZE * ELEMENTS_PER_BLOCK));

    res->w_buf_size = W_BUF_SIZE;
    res->w_mem = NULL;
    res->w_buf_used_list = NULL;
    res->w_buf_free_list = (w_buf_t*) res->w_buf;
    res->w_signal = w_signal_idle;

    
    uint_t i = 0;
    w_buf_t* tmp_buf;
    for(i = 0; i < W_BUF_SIZE-1; i++){
        tmp_buf = (w_buf_t*)(res->w_buf + i * sizeof(w_buf_t));
        tmp_buf->data_shift = W_BUF_SIZE * sizeof(w_buf_t) + i * ELE_SIZE * ELEMENTS_PER_BLOCK;
        tmp_buf->next = (w_buf_t*)(res->w_buf + (i+1) * sizeof(w_buf_t));
    }
    tmp_buf = (w_buf_t*)(res->w_buf + i * sizeof(w_buf_t));
    tmp_buf->data_shift = W_BUF_SIZE * sizeof(w_buf_t) + i * ELE_SIZE * ELEMENTS_PER_BLOCK;
    tmp_buf->next = NULL;

    for(i = 0; i < (BLOCKS_PER_CACHELINE * CACHELINE_COUNT / SYMBOLS_PER_WORD(CACHE_W_SYMBOL_SIZE) + 
            ((BLOCKS_PER_CACHELINE * CACHELINE_COUNT % SYMBOLS_PER_WORD(CACHE_W_SYMBOL_SIZE)) ? 1 : 0)); i++){
        res->w_symbols[i] = 0;
    }

    //TODO register interrupt handler 
    cache_info_w_global[interrupt_num] = res;
    intr_w_signal_turn_idle_register(interrupt_num);

    return (void*)res;
}

int cache_info_w_del(void* cache_info){
    cache_info_w_t* cache_info_w = (cache_info_w_t*)cache_info;
    uint_t i;
    uint_t cache_block_status;
    uint_t cache_w_block_status;
    for(i = 0; i < cache_info_w->cache_info_basic.blocks_per_cacheline * cache_info_w->cache_info_basic.cacheline_count; i++){
        cache_block_status = get_block_status(cache_info_w->cache_info_basic.symbols, i, CACHE_SYMBOL_SIZE);
        cache_w_block_status = get_block_status(cache_info_w->w_symbols, i, CACHE_W_SYMBOL_SIZE);
        if((cache_block_status == cache_symbol_valid) && (cache_w_block_status == cache_symbol_dirty)){
            writeDirtyBlockToBuffer(cache_info_w, i);
            modify_block_status(cache_info_w->w_symbols, i, cache_symbol_clean, CACHE_W_SYMBOL_SIZE);
            modify_block_status(cache_info_w->cache_info_basic.symbols, i, cache_symbol_invalid, CACHE_SYMBOL_SIZE);
        }
    }
    if(cache_info_w->w_buf_used_list != NULL){
        //标记free_buf block_idx设为-1.
        w_buf_t* temp_buf = cache_info_w->w_buf_free_list;
        while(temp_buf->next != NULL){
            temp_buf->block_idx = (uint_t)-1;
            temp_buf = temp_buf->next;
        }
        if(MT_writeBuffer(cache_info_w) == -1){
            hthread_printf("write buffer is failed\n");
            return -1;
        }
        cache_info_w->w_signal = w_signal_mem_to_read;
        cpu_interrupt(INTERRUPT_NUM);
        while(cache_info_w->w_signal != w_signal_idle){
            hthread_printf("waiting for w_buf to be written to memory\n");
        }
    }
    cache_info_del(cache_info);
    return 0;
}

// 检查请求的块索引是否在 w_buf_used_list 中
// 如果在，则将缓存行数据复制到 dst_data，并从 w_buf_used_list 中删除该缓存行
// 如果不在，则返回-1
int checkBlockInWBuf(cache_info_w_t* cache_info, uint_t blockIdx, void* dst_data){
    w_buf_t* temp = cache_info->w_buf_used_list;
    if(temp == NULL) return -1;
    if(temp->block_idx == blockIdx){
        memcpy(dst_data, cache_info->w_buf + temp->data_shift, cache_info->cache_info_basic.elements_per_block * cache_info->cache_info_basic.ele_size);
        cache_info->w_buf_used_list = temp->next;
        return 0;
    }
    w_buf_t* current = temp;
    while(current->next){
        if(blockIdx == current->next->block_idx){
            temp = current->next;
            memcpy(dst_data, cache_info->w_buf + temp->data_shift, cache_info->cache_info_basic.elements_per_block * cache_info->cache_info_basic.ele_size);
            current->next = temp->next;
            temp->next =cache_info->w_buf_free_list;
            cache_info->w_buf_free_list = temp;
            return 0;
        }
        current = current->next;
    };
    return -1;
}

//写数据，如果命中，则将symbol改为cache_symbol_dirty，如果未命中，需要先load？
int storeDataToBuf(cache_info_t* cache_info, unsigned long idx, void* data){
    if(idx >= cache_info->mem_size){
        hthread_printf("idx out of range\n");
        return -1;
    }

    uint_t offset;
    uint_t blockIdx;
    uint_t tag;
    uint_t lineIdx;

    calculateBlockOffsetAndIndex(cache_info, idx, &offset, &blockIdx, &tag, &lineIdx);
    uint_t dst_blockIdx; //命中或者要替换的块索引
    uint_t cache_w_block_status;
    uint_t cache_block_status;
    cache_info_w_t* cache_info_w = (cache_info_w_t*) cache_info;

    if(checkCacheHit(cache_info, tag, lineIdx, &dst_blockIdx) != -1){
        updateLRU(cache_info, dst_blockIdx);
        memcpy(cache_info->buf + (dst_blockIdx * cache_info->elements_per_block + offset) * cache_info->ele_size, 
                    data, cache_info->ele_size);
        modify_block_status(cache_info_w->w_symbols, dst_blockIdx, cache_symbol_dirty, CACHE_W_SYMBOL_SIZE);
        return 0;
    }

    //not hit
    //TODO：未命中，先查找w_buf,看是否在w_buf中，如果在，将该块取出来，准备塞回cache，w_buf清理出free空间
    //获取要被写入的块
    dst_blockIdx = getReplaceBlock(cache_info, lineIdx);

    cache_w_block_status = get_block_status(cache_info_w->w_symbols, dst_blockIdx, CACHE_W_SYMBOL_SIZE);
    cache_block_status = get_block_status(cache_info->symbols, dst_blockIdx, CACHE_SYMBOL_SIZE);

    if((cache_block_status == cache_symbol_valid) && (cache_w_block_status == cache_symbol_dirty)){
        writeDirtyBlockToBuffer(cache_info_w, dst_blockIdx);
        modify_block_status(cache_info_w->w_symbols, dst_blockIdx, cache_symbol_clean, CACHE_W_SYMBOL_SIZE);
        modify_block_status(cache_info->symbols, dst_blockIdx, cache_symbol_invalid, CACHE_SYMBOL_SIZE);
    }

    void* tmp_data = malloc(cache_info->elements_per_block * cache_info->ele_size);

    if(checkBlockInWBuf(cache_info_w, blockIdx, tmp_data) != -1){
        memcpy(cache_info->buf + dst_blockIdx * cache_info->elements_per_block * cache_info->ele_size, tmp_data, cache_info->elements_per_block * cache_info->ele_size);
    }
    else{  
        while(MT_loadBlock(cache_info, blockIdx, dst_blockIdx) == -1){
            hthread_printf("DMA load block failed\n");
        }
    }

    free(tmp_data);
    //写入数据,并改为dirty
    memcpy(cache_info->buf + (dst_blockIdx * cache_info->elements_per_block + offset) * cache_info->ele_size, 
            data, cache_info->ele_size);
    modify_block_status(cache_info_w->w_symbols, dst_blockIdx, cache_symbol_dirty, CACHE_W_SYMBOL_SIZE);
    modify_block_status(cache_info->symbols, dst_blockIdx, cache_symbol_valid, CACHE_SYMBOL_SIZE);
    cache_info->tags[dst_blockIdx] = tag;
    updateLRU(cache_info, dst_blockIdx);
}


//写数据，如果命中，则将symbol改为cache_symbol_dirty，如果未命中，需要先load？
int storeDataToBufAsync(cache_info_t* cache_info, unsigned long idx, void* data){
    if(idx >= cache_info->mem_size){
        hthread_printf("idx out of range\n");
        return -1;
    }

    uint_t offset;
    uint_t blockIdx;
    uint_t tag;
    uint_t lineIdx;

    calculateBlockOffsetAndIndex(cache_info, idx, &offset, &blockIdx, &tag, &lineIdx);
    uint_t dst_blockIdx; //命中或者要替换的块索引
    uint_t cache_w_block_status;
    uint_t cache_block_status;
    uint_t chl;
    cache_info_w_t* cache_info_w = (cache_info_w_t*) cache_info;

    if(checkCacheHit(cache_info, tag, lineIdx, &dst_blockIdx) != -1){
        updateLRU(cache_info, dst_blockIdx);

        if(get_block_status(cache_info->symbols, dst_blockIdx, CACHE_SYMBOL_SIZE) == cache_symbol_valid){
            memcpy(cache_info->buf + (dst_blockIdx * cache_info->elements_per_block + offset) * cache_info->ele_size, 
                    data, cache_info->ele_size);
            modify_block_status(cache_info_w->w_symbols, dst_blockIdx, cache_symbol_dirty, CACHE_W_SYMBOL_SIZE);
            return 0;
        }
        dma_update();

        if(get_block_status(cache_info->symbols, dst_blockIdx, CACHE_SYMBOL_SIZE) == cache_symbol_valid){
            memcpy(cache_info->buf + (dst_blockIdx * cache_info->elements_per_block + offset) * cache_info->ele_size, 
                    data, cache_info->ele_size);
            modify_block_status(cache_info_w->w_symbols, dst_blockIdx, cache_symbol_dirty, CACHE_W_SYMBOL_SIZE);
            return 0;
        }
        //wait for load block dma finished.
        return -1;
    }
    dst_blockIdx = getReplaceBlock(cache_info, lineIdx);
    cache_w_block_status = get_block_status(cache_info_w->w_symbols, dst_blockIdx, CACHE_W_SYMBOL_SIZE);
    cache_block_status = get_block_status(cache_info->symbols, dst_blockIdx, CACHE_SYMBOL_SIZE);

    if((cache_block_status == cache_symbol_valid) && (cache_w_block_status == cache_symbol_dirty)){
        if(writeDirtyBlockToBufferAsync(cache_info_w, dst_blockIdx) == -1){
            //wait for write buffer dma finished.
            hthread_printf("wait for write buffer dma finished.\n");
            return -1;
        }
        //TODO 是否只需要symbols而不需要w_symbols?
        modify_block_status(cache_info_w->w_symbols, dst_blockIdx, cache_symbol_clean, CACHE_W_SYMBOL_SIZE);
        modify_block_status(cache_info->symbols, dst_blockIdx, cache_symbol_invalid, CACHE_SYMBOL_SIZE);
    }

    void* tmp_data = malloc(cache_info->elements_per_block * cache_info->ele_size);
    if(checkBlockInWBuf(cache_info_w, blockIdx, tmp_data) != -1){
        memcpy(cache_info->buf + dst_blockIdx * cache_info->elements_per_block * cache_info->ele_size, tmp_data, cache_info->elements_per_block * cache_info->ele_size);
        modify_block_status(cache_info->symbols, dst_blockIdx, cache_symbol_valid, CACHE_SYMBOL_SIZE);
        modify_block_status(cache_info_w->w_symbols, dst_blockIdx, cache_symbol_dirty, CACHE_W_SYMBOL_SIZE);
        cache_info->tags[dst_blockIdx] = tag;
        updateLRU(cache_info, dst_blockIdx);
        free(tmp_data);
        return 0;
    }   
    if(MT_loadBlockAsync(cache_info, blockIdx, dst_blockIdx) == -1){
        hthread_printf("DMA load block failed, maybe dma channels are all busy.\n");
        free(tmp_data);
        return -1;
    }
    cache_info->tags[dst_blockIdx] = tag;
    updateLRU(cache_info, dst_blockIdx);
    modify_block_status(cache_info->symbols, dst_blockIdx, cache_symbol_updating, CACHE_SYMBOL_SIZE);
    modify_block_status(cache_info_w->w_symbols, dst_blockIdx, cache_symbol_clean, CACHE_W_SYMBOL_SIZE);
    free(tmp_data);
    //wait for load block dma finished.
    hthread_printf("load_block start, wait for load block dma finished.\n");
    return -1;
}