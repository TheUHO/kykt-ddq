#ifndef	F_PROCESSOR_PTHREAD_PUBLIC_H
#define	F_PROCESSOR_PTHREAD_PUBLIC_H

#include	<pthread.h>

#include	"basic_types.h"
#include	"ddq_plugin.h"


typedef	enum
{
	ddq_resource_status_none = 0,
	ddq_resource_status_available,
	ddq_resource_status_inuse
} ddq_resource_status_e;

typedef	struct ddq_resource_pthread_t	ddq_resource_pthread_t;
typedef	ddq_resource_pthread_t *	ddq_resource_pthread;
struct	ddq_resource_pthread_t
{
	// 以下需要在调用ddq_resource_pthread_new()返回非零后自行填充，初始都是NULL。
	void **			pdata;

	// 以下由此处的代码负责，调用者不必理会。

	char *			name;
	uint_t			count;
	ddq_resource_status_e *	status;

	pthread_key_t		key;

	ddq_resource_pthread		next;
};


// 以下函数由processor_pthread_*调用，一般在processor_pthread_*_share结构体里放一个ddq_resource_pthread。
int	ddq_resource_pthread_new(char *name, ddq_resource_pthread *pp, uint_t count);	// name建议使用常量字符串
int	ddq_resource_pthread_pick(ddq_resource_pthread p, meta m);	// return 0 if not available
void	ddq_resource_pthread_return(ddq_resource_pthread p);


// 这个函数可以在算子内部使用
void *	ddq_resource_pthread_get(char *name);


#endif
