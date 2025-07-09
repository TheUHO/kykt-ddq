#ifndef	F_STD_OPS_H
#define	F_STD_OPS_H

#include    "task_types.h"


// load_builtin_register("op_noop", op_noop);
task_ret	op_noop(void **inputs, void **outputs, void **attribute);

task_ret op_input_retention(void** inputs, void**outputs, void** attributes);

#endif
