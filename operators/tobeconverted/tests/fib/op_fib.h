#include	"task_types.h"

task_ret	op_print(void **inputs, void **outputs, void **attribute);

/*
    The op adds two long operands to obtain the result.
    inputs:
        int_t operand0;
        int_t operand1;
    outputs:
        int_t result;
*/
task_ret	op_add(void **inputs, void **outputs, void **attribute);


