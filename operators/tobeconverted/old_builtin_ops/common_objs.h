#ifndef	F_COMMON_OBJS_H
#define	F_COMMON_OBJS_H

#include	"basic_types.h"
#include	"error.h"


typedef	enum
{
	obj_mem_default = 0,
	obj_mem_cuda_host,
	obj_mem_cuda_device,
	obj_mem_tianhe3_dsp_scalar,
	//...

	obj_mem_last
} obj_mem_type;

typedef	struct	obj_mem_t	obj_mem_t;
typedef	obj_mem_t *		obj_mem;
struct	obj_mem_t
{
	obj_mem_type	type;
	int_t		size, bufsize;
	void *		p;
};

void *	obj_mem_new_size(int_t bufsize);	// 需要再包装一层才能输送给obj_new()
void *	obj_mem_new();
void	obj_mem_del(void *p);


typedef	union	obj_var_t	obj_var_t;
typedef	obj_var_t *	obj_var;
union	obj_var_t
{
	// TODO: 是否需要flag指定类型？
	uint_t	t_uint;
	int_t	t_int;
	float	t_float;
	double	t_double;
	void *	t_pointer;
	// ...
};

void *	obj_var_new();
void	obj_var_del(void *p);	// NOTE: 等价于free()


#endif
