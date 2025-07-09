
#include	"task_types.h"


struct processor_direct_share
{
	int	nothing;
};


struct processor_direct_t
{
	ddq_op_t	head;		// C99 6.7.2.1.13 : head的地址和整个结构体的地址相同 ，因此可以强制转换指针类型

	task_ret	ret;
};


