/*
 * 所有processor插件都是这样的格式，如下：
 * 	ddq_start();
 * 	...	在这里写一些初始化的代码
 * 	ddq_do();
 * 	...	在这里写每次运行的过程，注意它可能会运行多次
 * 	ddq_while(...);	这里的条件决定了是继续运行下一轮还是终止
 * 	...	在这里写终止前的收尾代码
 * 	ddq_finish();
 */

/*
 * 在代码中可以使用如下协程操作：
 * 	ddq_yield()		设置让行点，协程运行到这里会跳转到其他协程运行，随后返回到此处继续运行
 * 	ddq_waitfor(condition)	在条件满足时继续运行，否则让行
 * 在代码中可以使用如下变量语法糖：
 * 	ddq_s			指向processor_pthread_share类的实例的指针，同类协程共享空间
 * 	o_sv(varname)		即(ddq_s->varname)
 * 	ddq_p			指向processor_pthread_t类的实例的指针，本协程私有，一般用来代替协程的栈变量使用
 * 	o_v(varname)		即(ddq_p->varname)
 * 也可以使用如下语法糖访问ddq_p.head内的成员：
 * 	o_f			算子对应的obj对象 
 * 	o_n_in, o_n_out		输入和输出对象的个数
 * 	o_in, o_out		输入和输出对象的指针数组
 * 	o_pf			算子对应的函数指针，task_f类型
 * 	o_pin, o_pout		输入和输出参数的指针数组
 */

ddq_start();

if (!o_sv(max_procs))
{
	o_sv(max_procs) = sysconf(_SC_NPROCESSORS_ONLN);	// FIXME: 这里采用的熟知可能有通用性和可移植性的问题，需要仔细斟酌
	o_sv(n_procs) = 0;
	ddq_log("processor_fork : Set max_procs = %ld.\n", o_sv(max_procs));
}

ddq_do();

ddq_waitfor(o_sv(n_procs) < o_sv(max_procs));
o_sv(n_procs)++;

if (pipe(o_v(pipefd)))
	ddq_error("processor_fork : pipe creation failed.\n");

o_v(pid) = fork();
if (o_v(pid) < 0)
	ddq_error("processor_fork : fork failed.\n");

if (o_v(pid) == 0)
{
	if (close(o_v(pipefd)[0]))
		ddq_warning("processor_fork : something happens when closing pipe.\n");

	o_v(ret) = o_pf(o_pin, o_pout, o_pattr);

	if (write(o_v(pipefd)[1], &o_v(ret), sizeof(task_ret)) != sizeof(task_ret))
		ddq_warning("processor_fork : write() failed in the child process.\n");
	if (close(o_v(pipefd)[1]))
		ddq_warning("processor_fork : something happens when closing pipe.\n");

	exit(0);
}

if (close(o_v(pipefd)[1]))
	ddq_warning("processor_fork : something happens when closing pipe.\n");

if (fcntl(o_v(pipefd)[0], F_SETFL, fcntl(o_v(pipefd)[0], F_GETFL) | O_NONBLOCK) == -1)
	ddq_warning("processor_fork : fcntl() failed to set the pipe nonblock.\n");

ddq_waitfor(read(o_v(pipefd)[0], &o_v(ret), sizeof(task_ret)) > 0);	// FIXME: 这里如果错误信息不是EAGAIN，可能导致卡死在这里，应该报错退出。

if (close(o_v(pipefd)[0]))
	ddq_warning("processor_fork : something happens when closing pipe.\n");

o_sv(n_procs)--;

ddq_while( o_v(ret) );


ddq_finish();
