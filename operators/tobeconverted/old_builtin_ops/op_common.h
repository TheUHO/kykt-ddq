#ifndef	F_COMMON_OPS_H
#define	F_COMMON_OPS_H

#include	"ddq.h"
#include	"task_types.h"


typedef	enum	enum_op_bridge_status_t
{
	op_bridge_status_tounpack,
	op_bridge_status_toinit,
	op_bridge_status_ready,
	op_bridge_status_subsequent
} enum_op_bridge_status;


task_ret	op_noop(void **inputs, void **outputs, void **attribute);


// 这个函数开启一个新的平行于目前的ddq ring的另外一个ring，然后把它们桥接在一起
// 输入和输出可以有多个，但只有第一个输入被直接使用，它应当指向一个被打包的ring。
// 这个算子把输入内容解包为一个ring，然后在其中注入额外一个算子，是自身的孪生算子，互相保留对方的位置。
// 两个算子内容都很简单，即把当前全局的ring替换为对方的ring，然后返回task_ret_again。
// 当孪生算子发现自己是当前ring中最后一个时，把自己解构并彻底返回。最初的算子返回task_ret_done。
task_ret	op_bridge(void **inputs, void **outputs, void **attribute);


/* 这个函数产生n_para个obj对象并返回，每个obj对象指向n_dims个int_t类型数据。
 * 这些对象被ddq的其他算子使用后，会更新自己，直到所有的指标都已被更新为止。
 * 这个函数的...参数有n_dims组，每组3个int_t类型参数。所以这个函数总共有3*n_dims+2个参数。
 * 每组参数对应一个维度，其中第一个是这个维度的指标个数，第二个是第一个指标的值，第三个是指标每次的增减幅度。
 * 用户在使用完返回值之后，应当把返回值free掉。
*/
obj *		objs_iterator_iota(ddq_op *ring, int_t n_para, int_t n_dims, ...);


#endif
