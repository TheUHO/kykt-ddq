#include	"basic_types.h"
#include	"task_types.h"
#include	"error.h"
#include	"ddq_mem.h"

typedef	void *	construct_f();
typedef	void	destruct_f(void *p);

typedef	enum	enum_obj_status_t
{
	obj_status_toalloc = 0,
	obj_status_toread  = 1,
	obj_status_towrite = 2,
	obj_status_mask = 65535,
	obj_status_reading = 65536,
	obj_status_writing = 131072,
	obj_status_packed =  524288
} enum_obj_status;

enum	enum_obj_prop_t
{
	obj_prop_n_mask = 16383,
	obj_prop_consumable = 16384,
	obj_prop_ready = 32768,
	obj_prop_toassign = 65536,
	obj_prop_linear = 131072,
	// obj_prop_read_contest = 262144,
	obj_prop_buffer = 524288,
};
typedef	flag_t	enum_obj_prop;

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
