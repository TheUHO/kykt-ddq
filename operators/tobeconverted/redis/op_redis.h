//
// Created by fangx on 2023/7/12.
//

#ifndef _OP_REDIS_H_
#define _OP_REDIS_H_

#include "task_types.h"
#include "common_objs.h"
#include <hiredis/async.h>

typedef enum{
    redis_started,
    redis_done
} redis_flag;

typedef struct outputs {
    void* obj;
    redis_flag flag;
    struct outputs *next;
} Node,*Linklist;

task_ret op_redis_connect(void **inputs, void **outputs, void **attribute);
task_ret op_redis_command_set(void **inputs, void **outputs, void **attribute);
task_ret op_redis_command_getmem(void **inputs, void **outputs, void **attribute);


#endif //OP_REDIS_H
