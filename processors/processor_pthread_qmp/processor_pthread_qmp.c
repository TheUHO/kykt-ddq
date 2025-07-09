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


if (ddq_resource_pthread_new("qmp", &o_sv(comms), N_MPI_COMMS))
{
	struct QMP_comm_struct	buf;
	memcpy(&buf, QMP_comm_get_default_origin(), sizeof(struct QMP_comm_struct));	// 这个QMP_comm_get_default_origin()函数需要从原来的QMP_comm_get_default()复制出来
	for (int i=0; i<N_MPI_COMMS; i++)
	{
		o_sv(comms)->pdata[i] = malloc(sizeof(struct QMP_comm_struct));
		memcpy(o_sv(comms)->pdata[i], &buf, sizeof(struct QMP_comm_struct));
		MPI_Comm_dup(buf.mpicomm, &(((struct QMP_comm_struct *)o_sv(comms)->pdata[i])->mpicomm));
		o_sv(comms)->status[i] = ddq_resource_status_available;
	}
	ddq_resource_pthread_pick(o_sv(comms), NULL);	// NOTE: 主线程占据第0号通信域	// FIXME: 这个函数不应该失败吧？
	processor_pthread_qmp_initialized = 1;
}


ddq_do();


o_v(share) = ddq_s;
o_v(is_done) = 0;

if (pthread_create(&o_v(pt), NULL, run_pthread_qmp, ddq_p))
	ddq_error("processor_pthread : pthread_create() fails.\n");

ddq_waitfor(o_v(is_done));

if (pthread_join(o_v(pt), NULL))
	ddq_warning("processor_pthread : pthread_join() fails.\n");


ddq_while( o_v(ret) );


ddq_finish();
