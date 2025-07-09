#include    "std/std_types/std_types.h"
#include    "ddq_plugin.h"
#include    "processor_tianhe.h"
#include    "hthread_host.h"
#include    "ddq.h"
#include	"mt_hybrid.h"
#include    <string.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    "device/tianhe/host/types.h"

task_ret run_tianhe(struct processor_tianhe_t *p){
    // printf("run tianhe\n");
    int thread_id, cluster_id, core_num;
    obj_mem pack_ring;
    tianhe_op ops = (tianhe_op)(p->head.p_f);
// printf("1\n");
    // printf("3 packring:%p pack_ring->p:%p\n", pack_ring, pack_ring->p);
    if(ops == NULL){
        // printf("return\n");
        p->kernel_ret = NULL;
        return task_ret_done;
    }
    p->thread_info = ACCESS_DATA(p->head.p_inputs[0], tianhe_thread);
    // printf("2\n");
    thread_id = p->thread_info->thread_id;
    cluster_id = p->thread_info->cluster_id;
    core_num = p->thread_info->core_num;
    // printf("core_num:%d\n", core_num);

    // printf("hthread_malloc\n");
    // p->pack_ring = hthread_malloc(cluster_id, pack_ring->size, HT_MEM_RW);
    // memcpy(p->pack_ring, pack_ring, pack_ring->size);
    
    p->kernel_ret = hthread_malloc(cluster_id, sizeof(task_ret), HT_MEM_RW);
    *(p->kernel_ret) = task_ret_kernel;
    
    p->barrier_id = hthread_barrier_create(cluster_id);

    p->mems_meta = hthread_malloc(cluster_id, sizeof(void*)*core_num, HT_MEM_RW);
    
    //args prepare
    p->args = hthread_malloc(cluster_id, 4*sizeof(uint_t), HT_MEM_RW);
    p->args[0] = (uint_t)(p->barrier_id);
    p->args[1] = (uint_t)(ops);
    p->args[2] = (uint_t)(p->kernel_ret);
    p->args[3] = (uint_t)(p->mems_meta);
    printf("hthread_group_exec\n");
    if(hthread_group_exec(thread_id, "op_run_kernel", 1, 3, p->args) == -1){
        ddq_error("cluster %d : hthread_group_exec() fails.", cluster_id);
    }
    // printf("hthread_group_exec finish\n");
    return task_ret_kernel;

}

// task_ret run_tianhe(struct processor_tianhe_t *p, struct processor_tianhe_share *s){
//     // printf("run tianhe\n");
//     int thread_id, cluster_id, core_num;
//     obj_mem pack_ring;
//     tianhe_op ops = (tianhe_op)p->head.f->p;
// // printf("1\n");
//     // printf("3 packring:%p pack_ring->p:%p\n", pack_ring, pack_ring->p);
//     if(ops == NULL){
//         // printf("return\n");
//         p->kernel_ret = NULL;
//         return task_ret_done;
//     }
//     p->thread_info = ACCESS_DATA(p->head.p_inputs[0], tianhe_thread);
//     // printf("2\n");
//     thread_id = p->thread_info->thread_id;
//     cluster_id = p->thread_info->cluster_id;
//     core_num = p->thread_info->core_num;
//     // printf("core_num:%d\n", core_num);

//     // printf("hthread_malloc\n");
//     // p->pack_ring = hthread_malloc(cluster_id, pack_ring->size, HT_MEM_RW);
//     // memcpy(p->pack_ring, pack_ring, pack_ring->size);
    
//     p->kernel_ret = hthread_malloc(cluster_id, sizeof(task_ret), HT_MEM_RW);
//     *(p->kernel_ret) = task_ret_kernel;
    
//     p->barrier_id = hthread_barrier_create(cluster_id);

//     p->mems_meta = hthread_malloc(cluster_id, sizeof(void*)*core_num, HT_MEM_RW);
    
//     //args prepare
//     p->args = hthread_malloc(cluster_id, 4*sizeof(uint_t), HT_MEM_RW);
//     p->args[0] = (uint_t)(p->barrier_id);
//     p->args[1] = (uint_t)(ops);
//     p->args[2] = (uint_t)(p->kernel_ret);
//     p->args[3] = (uint_t)(p->mems_meta);
//     printf("hthread_group_exec\n");
//     if(hthread_group_exec(thread_id, "op_run_kernel", 1, 3, p->args) == -1){
//         ddq_error("cluster %d : hthread_group_exec() fails.", cluster_id);
//     }
//     // printf("hthread_group_exec finish\n");
//     return task_ret_kernel;

// }

void merge_meta_tianhe(meta* dst , struct processor_tianhe_t *p){
    // printf("merge_meta\n");
    if(!p->kernel_ret) return;
    int core_num = p->thread_info->core_num;
    // printf("merge_meta 0, p->mems_meta:%p, core_num:%d\n",p->mems_meta, core_num);
    mt_inv(p->mems_meta, sizeof(void*) * core_num);
    // printf("merge_meta 1\n");
    for(int i = 0; i < p->thread_info->core_num; i++){
        mt_inv(p->mems_meta[i], sizeof(obj_mem));
        // printf("merge_meta2\n");
        obj_mem mem_meta = (obj_mem)(p->mems_meta[i]);
        // printf("merge_meta3\n");
        mt_inv(mem_meta, mem_meta->size);
        // printf("merge_meta4\n");
        meta_merge(dst, unpack_meta(mem_meta));
        // printf("merge_meta5\n");
    }
    // printf("merge_meta6\n");
}

task_ret get_ret(struct processor_tianhe_t *p){
    if(!p->kernel_ret){
        return p->ret;
    }
    // printf("get_ret\n");
    mt_inv(p->kernel_ret, sizeof(task_ret));
    return *(p->kernel_ret);
}

void* flush_from_memory(void *ptr, uint_t size){
    mt_inv(ptr, size);
    return ptr;
}
