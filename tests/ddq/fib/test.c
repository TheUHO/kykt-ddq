// #define     ddq_op_region
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ddq.h"
#include "oplib.h"
#include "error.h"

#include "std/std_ops/std_ops.h"

// 创建一个新的 long 类型的变量，并初始化为 0
void* new_long(){
    long *res = malloc(sizeof(long));
    *res = 0;
    return res;
    // printf("new_ul!\n");
    // return (void*)0;
}

// 创建一个新的 long 类型的变量，并初始化为 5
void* new_count(){
    long *res = malloc(sizeof(long));
    *res = 5;
    return res;
    // printf("new_ul!\n");
    // return (void*)0;
}

// 释放 long 类型数据的内存
void free_long(void* p){
    free(p);
    // printf("new_ul!\n");
    // return (void*)0;
}

// 目前 placeholder，用于创建无符号长整型数据（未实际分配内存）
void* new_ul(){
    // unsigned long *res = malloc(sizeof(unsigned long));
    // *res = 0;
    // return res;
    // printf("new_ul!\n");
    return (void*)0;
}

// 占位函数，未进行内存释放操作
void free_ul(void* p){
    // unsigned long *res = malloc(sizeof(unsigned long));
    // *res = 0;
    // return res;
    // printf("free_ul!\n");
    return;
}

// 初始化任务：将输入赋值给输出（通过强制类型转换实现）
task_ret f_init(void **inputs, void **outputs, void **attribute)
{
    // printf("init!\n");
    // 将输入[0]的指针直接传递到输出[0]
    outputs[0] = (void*)(unsigned long)inputs[0];
    // ddq_log("f_init %ld\n", *(unsigned long *)outputs[0]);
    return task_ret_done;
}

// 打印任务：输出无符号长整型的值
task_ret f_print(void **inputs, void **outputs, void **attribute)
{
    // printf("print!\n");
    // 输出打印输入[0]作为无符号长整型的值
    printf("%ld\t", (unsigned long)inputs[0]);
    fflush(stdout);
    // ddq_log("f_print %ld\n", *(unsigned long *)inputs[0]);
    return task_ret_ok;
}

// 加法任务：累加输入值到已有结果，然后判断是否达到终止条件
task_ret f_add(void **inputs, void **outputs, void **attribute)
{
    // printf("f_add!\n");
    // 将输入[0]与当前输出[0]相加，并将结果赋给输出[0]
    outputs[0] = (void*)((unsigned long)inputs[0] + (unsigned long)outputs[0]);

    // 模拟耗时
    sleep(1);
    // ddq_log("f_add %ld\n", *(unsigned long *)outputs[0]);

    // 如果累计结果超过 1000，则任务结束，否则继续执行
    if ((unsigned long)outputs[0] > 1000)
        return task_ret_done;
    else
        return task_ret_ok;
}

// 构造一个包含多个任务的环，用于计算斐波那契序列（或其它数列）
// 此环由多个处理器任务组成，每个任务完成特定的操作
ddq_ring fib_ring(){

    // 新建一个环对象，包含 1 个全局输入，0 个全局输出
    ddq_ring ring = ddq_new(NULL, 1, 0);

    obj f_a, f_p, f_i, a, b;
    ddq_op ia, ib, ab, ba, pa, pb, retention_a;

    // 导入并包装函数操作，用于初始化、加法和打印操作
    f_i = obj_import(ring, f_init, NULL, obj_prop_ready);
    f_a = obj_import(ring, f_add, NULL, obj_prop_ready);
    f_p = obj_import(ring, f_print, NULL, obj_prop_ready);
    obj input_retention = obj_import(ring, op_input_retention, NULL, obj_prop_ready);

    // 创建状态对象：a, a_copy 和 b，当前用 new_ul 创建
    a = obj_new(ring, new_ul, free_ul, obj_prop_consumable);
    obj a_copy = obj_new(ring, new_ul, free_ul, obj_prop_consumable);
    b = obj_new(ring, new_ul, free_ul, obj_prop_consumable);

    // 创建任务处理器，使用 pthread 调度

    // 任务 ia: 初始化任务，将全局输入[0]传递给 a
    ia = ddq_spawn(ring, processor_pthread, 1, 1);
    ddq_add_f(ia, f_i);
    ddq_add_inputs(ia, 0, ring->inputs[0]);
    ddq_add_outputs(ia, 0, a);

    // 任务 ab: 执行加法操作，将 a 结果加传递到 b
    ab = ddq_spawn(ring, processor_pthread, 1, 1);
    ddq_add_f(ab, f_a);
    ddq_add_inputs(ab, 0, a);
    ddq_add_outputs(ab, 0, b);

    // 任务 ba: 执行加法操作，将 b 结果加传递回 a
    ba = ddq_spawn(ring, processor_pthread, 1, 1);
    ddq_add_f(ba, f_a);
    ddq_add_inputs(ba, 0, b);
    ddq_add_outputs(ba, 0, a);

    // retention_a: 保持输入 a 的引用，同时传入计数和初始值数据，输出为 a_copy
    retention_a = ddq_spawn(ring, processor_pthread, 3, 1);
    ddq_add_f(retention_a, input_retention);
    ddq_add_inputs(retention_a, 0, a);
    ddq_add_inputs(retention_a, 1, obj_new(ring, new_count, free_long, 0));
    ddq_add_inputs(retention_a, 2, obj_new(ring, new_long, free_long, 0));
    ddq_add_outputs(retention_a, 0, a_copy);

    // 任务 pa: 打印 a 中的内容
    pa = ddq_spawn(ring, processor_pthread, 1, 0);
    ddq_add_f(pa, f_p);
    ddq_add_inputs(pa, 0, a);

    // 任务 pb: 打印 b 中的内容
    pb = ddq_spawn(ring, processor_pthread, 1, 0);
    ddq_add_f(pb, f_p);
    ddq_add_inputs(pb, 0, b);

    return ring;
}

int main()
{
    // 输出程序开始提示
    printf("begin\n");

    // 创建并构造计算环
    ddq_ring ring = fib_ring();
    
    // 执行环中任务的更新
    ddq_update(ring);

    // 以下代码段为可能的备选操作：打包/解包或者删除环（目前被注释掉） 
    // void* ptr = pack_ring(ring);
    // ddq_delete(ring);
    // ddq_ring ring3 = unpack_ring(ptr);
    // ddq_ring ring4 = unpack_ring(ptr);
    // ddq_delete(ring3);
    // ddq_delete(ring4);
    // ddq_ring ring2 = unpack_ring(ptr);
    // free(ptr);

    // 使用 ring2（这里与 ring 指向相同的对象）用于输入数据
    ddq_ring ring2 = ring;

    // 用一个无符号长整型变量设置初始输入数据
    unsigned long a0;
    a0 = 1;
    
    // 将 a0 转换后封装为对象，传入全局输入[0]
    ring2->inputs[0] = obj_import(ring, a0, NULL, obj_prop_ready);

    // 初始化事件循环
    ddq_loop_init();
    // 执行环的主循环; 参数 0 表示阻塞模式或其它循环配置
    ddq_loop(ring2, 0);

    // 以下代码可能用于调试或打印日志（目前被注释掉）
    // int i;
    // for(i=0; i<ring2->n_ops; i++){
    //     meta log = name2meta(ring2->metadatas[i], NAME_DDQ_LOG);
    //     printf("%s\n", log->value.sval);
    // }
    
    // 输出程序结束提示
    printf("Done!\n");

    // 删除环，释放资源
    ddq_delete(ring2);
    // TODO: pack 相关代码需要修改 delete——ring
    return 0;
}
