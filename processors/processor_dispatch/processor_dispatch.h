
#include	"task_types.h"


#define MAX_DSP 23

typedef	enum
{
	dsp_status_idle = 0,
	dsp_status_busy,
	dsp_status_allocated
} dsp_status_t;

struct processor_dispatch_share
{
	dsp_status_t dsp_status[MAX_DSP];
};


struct processor_dispatch_t
{
	ddq_op_t	head;		// C99 6.7.2.1.13 : head的地址和整个结构体的地址相同 ，因此可以强制转换指针类型

	task_ret	ret;

    int         dsp_id;

};


