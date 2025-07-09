#include 	"ddq.h"
#include	"pool.h"
#include 	"../std_types/std_types.h"
#include	"std_ops.h"

/*
 * built-in operators
 */

task_ret	op_noop(void **inputs, void **outputs, void **attribute)
{
	return	task_ret_ok;
}

task_ret op_input_retention(void** inputs, void**outputs, void** attributes){
    // ddq_log0("op_input_retention begin\n");
    outputs[0] = inputs[0];
    int_t retention_count = *(int_t*)inputs[1];
    int_t* current_count = (int_t*)inputs[2];
    if(++(*current_count) < retention_count){
        // ddq_log0("op_input_retention task_ret_read_partial\n");
        return task_ret_read_partial;
    }
    // ddq_log0("op_input_retention task_ret_ok\n");
    *current_count = 0;
    return task_ret_ok;
}

task_ret op_output_retention(void** inputs, void**outputs, void** attributes){
    outputs[0] = inputs[0];
    int_t retention_count = *(int_t*)inputs[1];
    int_t* current_count = (int_t*)inputs[2];
    if(++(*current_count) < retention_count){
        return task_ret_write_partial;
    }
    return task_ret_ok;
}

