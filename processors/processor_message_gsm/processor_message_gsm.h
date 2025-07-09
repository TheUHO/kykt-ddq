
#include	"task_types.h"

// typedef	enum
// {
//     message_status_free = 0, 
// 	message_status_toread,
// 	message_status_towrite,
// 	message_status_reading,
//     message_status_writing
// } message_status_t;

typedef	enum
{
    message_role_free = 0, 
	message_role_sender,
	message_role_receiver,
} message_role_t;

struct processor_message_gsm_share
{
};


struct processor_message_gsm_t
{
	ddq_op_t	head;		// C99 6.7.2.1.13 : head的地址和整个结构体的地址相同 ，因此可以强制转换指针类型

	task_ret	ret;

	int thread_id;

	message_role_t role;

};


