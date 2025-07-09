#ifndef	F_STD_TYPES_H
#define	F_STD_TYPES_H

#include	"basic_types.h"
#include	"task_types.h"


/*
 * 基本的变量类型：int, double, char*
 */

// load_builtin_register("new_int", new_int);
// load_builtin_register("del_int", del_int);
// load_builtin_register("op_ser_int", op_ser_int);
// load_builtin_register("op_deser_int", op_deser_int);
void *		new_int();
void		del_int(void *p);
task_ret	op_ser_int(void **inputs, void **outputs, void **attribute);
task_ret	op_deser_int(void **inputs, void **outputs, void **attribute);

// load_builtin_register("new_real", new_real);
// load_builtin_register("del_real", del_real);
// load_builtin_register("op_ser_real", op_ser_real);
// load_builtin_register("op_deser_real", op_deser_real);
void *		new_real();
void		del_real(void *p);
task_ret	op_ser_real(void **inputs, void **outputs, void **attribute);
task_ret	op_deser_real(void **inputs, void **outputs, void **attribute);

// load_builtin_register("del_str", del_str);
void		del_str(void *p);


/*
 * 内存块类型
 */

typedef	enum
{
	obj_mem_default = 0,

	obj_mem_rawptr,

	obj_mem_cuda_host,
	obj_mem_cuda_device,
	obj_mem_mt3000_dev_mem,
	obj_mem_mt3000_dsp_sm,
	obj_mem_mt3000_dsp_am,
	obj_mem_mt3000_dsp_gsm,
	//...

	obj_file_local,
	//...

	obj_mem_copying,
	obj_mem_last = 65536,
} obj_mem_type;

typedef	struct	obj_mem_t	obj_mem_t;
typedef	obj_mem_t *		obj_mem;
struct	obj_mem_t
{
	obj_mem_type	type;
	int_t		size, bufsize;
	void *		p;
};

// load_builtin_register("new_mem", new_mem);
// load_builtin_register("new_mem_cude_device", new_mem_cuda_device);
// load_builtin_register("new_mem_file", new_mem_file);
// load_builtin_register("del_mem", del_mem);
// load_builtin_register("new_ptr", new_ptr);
void *		new_mem();
void *		new_mem_cuda_device();
void *		new_mem_file();
void		del_mem(void *p);

void *		new_ptr();


/*
 * 运行时算子环类型
 */

// load_builtin_register("new_ring", new_ring);
// load_builtin_register("del_ring", del_ring);
void *	new_ring();
void	clear_ring(void *p);
void	del_ring(void *p);


/*
 * ddq脚本的中间表示类型
 */

// load_builtin_register("new_ast", new_ast);
// load_builtin_register("del_ast", del_ast);
void *	new_ast();
void	del_ast(void *p);


/*
 * 仅用于连接算子先后关系的dummy数据类型
 */

// load_builtin_register("new_dummy", new_dummy);
void *	new_dummy();



#endif
