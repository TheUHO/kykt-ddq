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
// #include "processor_tianhe.h"
ddq_start();

ddq_do();

o_v(ret) = run_tianhe(ddq_p);
// printf("o_v(ret):%d\n",o_v(ret));

ddq_waitfor((o_v(ret) = get_ret(ddq_p)) <= task_ret_done);

// printf("o_v(pack_ring):%p\n",o_v(pack_ring));

// printf("before merge\n");

// merge_meta_tianhe(&(ring->metadatas[o_id]), ddq_p);
// printf("after merge\n");

if(o_v(kernel_ret)){
    hthread_group_wait(o_v(thread_info)->thread_id);

    hthread_barrier_destroy(o_v(barrier_id));
    hthread_free(o_v(args));
    // hthread_free(o_v(pack_ring));
    hthread_free(o_v(kernel_ret));
    hthread_free(o_v(mems_meta));
}
ddq_while( o_v(ret) );

ddq_finish();

