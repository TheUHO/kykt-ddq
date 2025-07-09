#include	<string.h>			// memset()

#include	"ddq_plugin.h"
#include	"ddq_types.h"
#include 	"basic_types.h"
#ifdef enable_processor_dsp
#include "hthread_device.h"
#endif
#include "std/std_types/std_types.h"
#include 	"error.h"
#include "ddq.h"

int_t get_ring_size(void* pack_ring){
    obj_mem m = (obj_mem)pack_ring;
    return m->size;
}

obj_mem ddq_mem_init(int_t size){
    obj_mem mem = malloc(size);
    if(!mem) return NULL;
    mem->p = (char*)mem + aligned8(sizeof(obj_mem_t));
    mem->size = size;
    mem->bufsize = aligned8(sizeof(obj_mem_t));
    return mem;
}

void ddq_mem_destroy(obj_mem mem){
    if(mem) free(mem);
}

ddq_ring ddq_ring_init(){
    int_t size = 10240*16000;//TODO 动态分配内存
    obj_mem mem = ddq_mem_init(size);
    // printf("ddq_ring_init :mem:%p, mem->p:%p\n", mem, mem->p);
    if(!mem) return NULL;
    ddq_ring res = mem->p;
    res->mem = mem;
    return res;
}


void* ddq_malloc(ddq_ring ring, int_t size){
    obj_mem mem = ring->mem;
    // ddq_log0("ddq_malloc %dB, bufsize:%dB, size:%dB, aligned8(size):%dB\n", size, mem->bufsize, mem->size, aligned8(size));
    if(!mem) return NULL;
    size = aligned8(size);
    if(mem->bufsize + size > mem->size){
        // void* p = pack_ring(ring);
        // obj_mem new_mem = ddq_mem_init(mem->size * 2);
        // memcpy(new_mem, p, mem->size);
        // ddq_free(ring, ring);
        // ring = unpack_ring(new_mem);
        // new_mem->size = mem->size * 2;
        // mem = new_mem;
        ddq_error("ddq_malloc:not enough memory %dB\n", size);
        return NULL;
    }
    void* res = (char*)mem + mem->bufsize;
    mem->bufsize += size;
    memset(res, 0, size);
    // ddq_log0("ddq_malloc %dB, bufsize:%dB, size:%dB res:%p\n", size, mem->bufsize, mem->size, res);
    return res;
}

// void* ddq_malloc(ddq_ring* ring_ptr, int_t size){
//     ddq_ring ring = *ring_ptr;
//     obj_mem mem = ring->mem;
//     if (!mem) return NULL;

//     size = aligned8(size);

//     if (mem->bufsize + size > mem->size) {
//         // 动态扩展内存
//         int_t new_size = mem->size * 2;
//         while (new_size < mem->bufsize + size) {
//             new_size *= 2;
//         }

//         obj_mem new_mem = ddq_mem_init(new_size);
//         if (!new_mem) {
//             ddq_error("ddq_malloc: 无法分配新的内存块，大小为 %dB\n", new_size);
//             return NULL;
//         }

//         // 复制旧内存内容到新内存
//         void* p = pack_ring(ring);
//         memcpy(new_mem, p, mem->size);
//         new_mem->bufsize = mem->bufsize;

//         // 更新 ring 指针
//         *ring_ptr = (ddq_ring)new_mem->p;
//         (*ring_ptr)->mem = new_mem;

//         ddq_mem_destroy(mem); // 释放旧内存
//         mem = new_mem;
//     }
//     // 分配内存
//     void* res = (char*)mem + mem->bufsize;
//     mem->bufsize += size;

//     // 初始化分配的内存为 0
//     memset(res, 0, size);

//     return res;
// }

void ddq_free(ddq_ring ring, void* ptr){
    if(ptr == ring->mem->p) free(ring->mem);
}