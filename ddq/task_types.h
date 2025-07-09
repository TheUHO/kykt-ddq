#ifndef	F_TASK_TYPE_H_
#define	F_TASK_TYPE_H_

typedef	enum
{
	task_ret_ok = 0,	// 任务完成，算子保留下次使用
	task_ret_read_partial,
	task_ret_write_partial,
	task_ret_again,		// 任务未完成，可以暂时移交控制权以等待并发工作，算子内部应当使用static变量记录状态。目前只在processor_direct里使用。

	task_ret_done,		// 任务完成，算子不必保留
	task_ret_error,		// TODO: 错误有哪些？怎么处理？

	task_ret_kernel,	//需要调用kernel函数 //是否需要，error放大数，分管权限

	// ...

	task_ret_last
} task_ret;

typedef task_ret    task_f(void **inputs, void **outputs, void **attribute);

#ifdef	enable_processor_cuda
typedef task_ret    task_cuda_f(void **inputs, void **outputs, void **attribute, void* stream);

#endif


typedef	void	task_f_wildcard(void);	//FIXME: 是否应当这样做？如果用void*来承载各种函数指针，会造成UB吧？或者都用task_f来承载？


#endif
