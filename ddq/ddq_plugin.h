#ifndef	F_OSP_PLUGIN_H_
#define	F_OSP_PLUGIN_H_

#include	<limits.h>		// for USHRT_MAX

#include	"basic_types.h"
#include	"task_types.h"
#include	"error.h"
#include	"ddq_mem.h"


typedef	enum	enum_obj_status_t
{
	obj_status_toalloc = 1,
	obj_status_toread  = 2,
	obj_status_towrite = 4,
	obj_status_reading = 8,
	obj_status_writing = 16,
	obj_status_packed =  32
} enum_obj_status;

enum	enum_obj_prop_t
{
	obj_prop_n_mask = 16383,
	obj_prop_consumable = 16384,
	obj_prop_ready = 32768,
	obj_prop_toassign = 65536,
	obj_prop_linear = 131072,
	obj_prop_read_contest = 262144,
	obj_prop_buffer = 524288,
};
typedef	flag_t	enum_obj_prop;

typedef	void *	construct_f();
typedef	void	destruct_f(void *p);

typedef enum enum_meta_type_t
{
	meta_type_int,
	meta_type_double,
	meta_type_string,
} enum_meta_type;

typedef	struct meta_t	meta_t;
typedef	meta_t *		meta;
struct meta_t
{
	char*		name;
	enum_meta_type type;
	union{
		int		ival;
		double	dval;
		char*		sval;
	} value;
	meta 		next;
};

typedef	struct obj_t	obj_t;
typedef	obj_t *		obj;
struct	obj_t
{
	void *		p;// NOTE: 如果status为toassign，p值表示的是ring的第几个输入/输出，负数为输入，正数为输出。

	enum_obj_status	status;
	enum_obj_prop	prop;

	construct_f *	constructor; 
	// obj 	constructor1;
	// op to_alloc -> to_write
	destruct_f *	destructor;

	int_t		n_writers;
	int_t		n_readers, n_reads;
	flag_t		version;		// NOTE: 此处version在0/1之间切换，而不累加，以防溢出。

	obj		dup;			// NOTE: 正常情况下指向自己，共享情况下构成单向环
	int_t	n_ref;

	meta		metadata;
};

typedef	struct buffer_obj_t	buffer_obj_t;
typedef	buffer_obj_t *		buffer_obj;
struct	buffer_obj_t
{
	obj_t ob;
	int_t size, element_size;
	int_t count;
	enum_obj_status *element_status;
};

typedef	struct	ddq_op_t	ddq_op_t;
typedef	ddq_op_t *		ddq_op;
struct	ddq_op_t
{
	// 这一段需要用户负责在调用ddq_spawn()之后填入
	obj		f;
	obj *		inputs;
	obj *		outputs;

	// 之后的内容不需要用户负责
	int_t 		uid;

	int_t		n_threads;
	int_t		max_threads;
	ddq_op		next;
	ddq_label	pos;
	ddq_label 	type;

	int_t		n_inputs;
	int_t		n_outputs;
	// int_t 		n_attributes;

	void*		p_f;
	void **		p_inputs;
	void **		p_outputs;
	void ** 	p_attributes;

	flag_t		ver_f;
	flag_t *	ver_inputs;

	ddq_op *	neighbors;	//邻居算子
	int_t 		n_neighbors;

	meta		metadata;

	// void *		buf[];			// FIXME: 只是占位而已，不用。也许可以确保对齐。
};

typedef	enum	enum_op_status_t
{
	op_status_blocked = 0,
	op_status_ready  = 1,
	op_status_removed = 2
} enum_op_status;

typedef	struct	ddq_ring_t	ddq_ring_t;
typedef	ddq_ring_t *	ddq_ring;
struct ddq_ring_t
{
	ddq_op ops;
	obj*	inputs;
	obj*	outputs;
	int_t	n_inputs;
	int_t	n_outputs;
	int_t	n_ops;
	enum_op_status* 	status_ops;
	ddq_op block_queue;
	meta* 	metadatas;
	obj_mem mem;
};

#define	ddq_type_init(type)	\
	do {	\
		if (!ddq_type_share[type]) {	\
			ddq_type_share[type] = malloc(sizeof(struct type##_share));	\
			memset(ddq_type_share[type], 0, sizeof(struct type##_share)); }	\
	} while (0)

#define	o_f		(op->f)
#define	o_n_in		(op->n_inputs)
#define	o_n_out		(op->n_outputs)
#define	o_in		(op->inputs)
#define	o_out		(op->outputs)
#define o_id 		(op->uid)

#define	o_pf		((task_f *)(op->p_f))
#define	o_pin		(op->p_inputs)
#define	o_pout		(op->p_outputs)
#define o_pattr		(op->p_attributes)

#define	o_v(varname)	(ddq_p->varname)
#define	o_sv(varname)	(ddq_s->varname)


#define	ddq_yield()	\
	do {	\
		op->pos = __LINE__+this_type*USHRT_MAX; goto ddq_start_point; case __LINE__+this_type*USHRT_MAX : ;	\
	} while (0)

#define ddq_waitfor(condition)	\
	do {	\
		op->pos = __LINE__+this_type*USHRT_MAX; case __LINE__+this_type*USHRT_MAX :	\
		if (!(condition)) goto ddq_start_point;	\
	} while(0)

#define	ddq_start()	\
	do {	\
		case this_type : ;	\
		ddq_init(ring, op);	\
		ddq_yield();		\
	} while (0)
//prerun之后加入block队列ring->status_ops[op->uid] = op_status_blocked;
#define	ddq_do()	\
	do {	\
		case this_type+ddq_type_last : ;	\
		if (!ddq_prerun(op)) {	\
			if(ddq_prefail(op)){							\
				ddq_prefinal(ring, op);	\
				op->pos = 0;	\
			}else{	\
				op->pos = this_type+ddq_type_last;	\
				ring->status_ops[op->uid] = op_status_blocked;	\
				prev_op->next = op->next;	\
				op = prev_op;	\
			}	\
			goto ddq_start_point;	\
	}} while (0)
//post_run之后neighbor加入ring->ops
#define	ddq_while(ret_type)	\
	do {	\
		flag_t ret = (ret_type);	\
		ddq_postrun(ring, op, prev_op, ret);	\
		if ((ret != task_ret_done) && !ddq_check_final(op, ret)) {	\
			op->pos = this_type+ddq_type_last;	\
			goto ddq_start_point;	\
		}	\
		case this_type+ddq_type_last*2 : ;	\
	} while (0)

#define	ddq_finish()	\
	do {	\
		ddq_prefinal(ring, op);	\
		op->pos = 0;	\
		goto	ddq_start_point;	\
	} while (0)


#endif
