#ifndef F_OSP_H
#define	F_OSP_H

#include	"ddq_plugin.h"
#include	"ddq_types.h"
#include "../operators/std/std_types/std_types.h"
#ifdef __cplusplus
extern "C"
{
#endif

meta meta_new(char*name, enum_meta_type t, void* v_ptr);
meta name2meta(meta metadata, char* name);
void meta_merge(meta* dst, meta src);
obj_mem pack_meta(meta metadata);
meta unpack_meta(obj_mem mem);
void meta_delete(meta metadata);

// 新建一个对象，但不预先分配，完全由ddq管理。析构函数的指针可以为NULL。
// obj	obj_new(construct_f *pfc, destruct_f *pfd, enum_obj_prop prop);
// obj	obj_new_with_name(char* name, construct_f *pfc, destruct_f *pfd, enum_obj_prop prop);
obj	obj_new(ddq_ring ring, construct_f *pfc, destruct_f *pfd, enum_obj_prop prop);
obj	buffer_obj_new(ddq_ring ring, construct_f *pfc, destruct_f *pfd, int_t element_size, int_t size);
obj	obj_new_with_name(ddq_ring ring, char* name, construct_f *pfc, destruct_f *pfd, enum_obj_prop prop);


// 新建一个对象，把已分配的内存指针传入。析构函数的指针可以为NULL。
// obj	obj_import(void *p, destruct_f *pfd, enum_obj_prop prop);
// obj	obj_import_with_name(char* name, void *p, destruct_f *pfd, enum_obj_prop prop);
obj	obj_import(ddq_ring ring,void *p, destruct_f *pfd, enum_obj_prop prop);
obj	obj_import_with_name(ddq_ring ring, char* name, void *p, destruct_f *pfd, enum_obj_prop prop);


// 从一个已有的对象建立一个新对象，它们共享指针和建构析构函数
// obj	obj_dup(obj ob);
// obj	obj_dup_with_name(char* name, obj ob);
obj	obj_dup(ddq_ring ring,obj ob);
obj	obj_dup_with_name(ddq_ring ring, char* name, obj ob);


// TODO：需要考虑清楚所有obj的状态和所有权等问题。也许需要保证，一个obj不可以放进不同的ring中。
// 当然，同一个void *p可以被包装成不同的obj并放入不同的ring，后果由用户负责。

// 新建和删除一个ddq的ring
// ddq_ring	ddq_new(int_t n_inputs, int_t n_outputs);
ddq_ring	ddq_new(ddq_ring top, int_t n_inputs, int_t n_outputs);
void		ddq_delete(ddq_ring p);

// 新建一个协程，随后用户需要自行填入其中的f和输入输出等obj。
ddq_op	ddq_spawn(ddq_ring _ring, ddq_type_t type, int_t n_inputs, int_t n_outputs);
ddq_op	ddq_spawn_with_name(char* name, ddq_ring _ring, ddq_type_t type, int_t n_inputs, int_t n_outputs);

void ddq_add_f(ddq_op op, obj ob);
void ddq_add_inputs(ddq_op op, int_t idx, obj ob);
void ddq_add_outputs(ddq_op op, int_t idx, obj ob);
//TODO
void ddq_add_attributes(ddq_op op, int_t idx, obj ob);

// 开始运行所有协程，都运行完毕后返回。所有涉及到的obj对象都会被释放。
/*void	ddq_loop();

ddq_ring ddq_get();

void ddq_put(ddq_ring _ring); 
*/

// int_t get_ddq_size(ddq_op op);
//返回obj_mem类型指针
void* pack_ring(ddq_ring ring);
// 新的processor_call需要的函数：
ddq_ring unpack_ring(void *p);

void ddq_loop_init();
void ddq_loop(ddq_ring ring, int nstep);

ddq_ring ddq_ring_init();
void ddq_ring_destroy(ddq_ring ring);
void* ddq_malloc(ddq_ring ring, int_t size);
void ddq_free(ddq_ring ring, void* ptr);

obj_mem ddq_mem_init(int_t size);
int_t get_type_size(int_t type);

void	ddq_debug_iter(ddq_ring ring, ddq_op op);

void ddq_update(ddq_ring ring);



#ifdef __cplusplus
}
#endif

#endif
