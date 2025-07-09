#ifndef	F_OPLIB_H
#define	F_OPLIB_H

#include	"basic_types.h"
#include	"task_types.h"
#include	"error.h"
#include	"ddq.h"

typedef	enum
{
	oplib_direct = 0,	// 静态连接进可执行文件的函数，需要手工put进oplib。包括申威从核函数——因为它不支持动态链接。
	oplib_dl,		// 正常的dlopen/dlsym/dlclose处理
	oplib_tianhe,		// 天河上的DSP核函数
	oplib_tianhe_run,	// 天河上的run核函数
				// FIXME: 在不同的DSP上或不同的簇上，运行的同一个核函数是否地址相同？

	//...

	oplib_last
} oplib_type;


void	oplib_init();

void	oplib_put(task_f_wildcard *p, oplib_type type, char *libname, char *funcname);	// 内部需要维护一个散列表

obj	oplib_get(oplib_type type, char *libname, char *funcname);	// 用户只访问这个函数，得到算子的函数指针

// FIXME: 关于drop/gc部分还没有做，需要再斟酌一下。
//void	oplib_drop(task_f_wildcard *p);	// 用户在每个get后应当最终要把它drop掉，否则无法完整回收。内部维护一个引用计数
//void	oplib_gc();	// 引用计数为0并不一定马上触发gc，所以需要一个单独的全局gc函数，用于指定gc策略。


void	oplib_load_so(char *libname, char *funcname);
void	oplib_load_tianheDat(char *datpath);
void	oplib_load_tianheFunc(char *libname, char *funcname);
void	oplib_load_tianheRun(char *libname, char *funcname);


#endif
