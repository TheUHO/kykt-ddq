// #define     ddq_op_region
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ddq.h"
#include "oplib.h"
#include "error.h"

#include "std/std_ops/std_ops.h"

void *new_long() {
    long *res = malloc(sizeof(long));
    *res = 0;
    return res;
    // printf("new_ul!\n");
    // return (void*)0;
}

void *new_buffer() {
    long *res = malloc(sizeof(long) * 5);
    memset(res, 0, sizeof(long) * 5);
    return res;
    // printf("new_ul!\n");
    // return (void*)0;
}

void *new_count() {
    long *res = malloc(sizeof(long));
    *res = 5;
    return res;
}

void free_long(void *p) { free(p); }

void *new_ul() { return (void *)0; }
void free_ul(void *p) { return; }
task_ret f_init(void **inputs, void **outputs, void **attribute) {
    // printf("init!\n");
    // *(unsigned long *)outputs[0] = *(unsigned long *)inputs[0];
    // WRITE_OUTPUTS(0, unsigned long, ACCESS_INPUTS(0, unsigned long));
    *(unsigned long *)outputs[0] = *(unsigned long *)inputs[0];
    // ddq_log("f_init %ld\n", *(unsigned long *)outputs[0]);

    return task_ret_done;
}

task_ret f_print(void **inputs, void **outputs, void **attribute) {
    // printf("print!\n");
    // printf("%ld\t", *(unsigned long *)inputs[0]);
    printf("%ld\t", *(unsigned long *)inputs[0]);
    fflush(stdout);
    sleep(1);
    // ddq_log("f_print %ld\n", *(unsigned long *)inputs[0]);

    return task_ret_ok;
}

task_ret f_acc(void **inputs, void **outputs, void **attribute) {
    // printf("f_add!\n");
    *(unsigned long *)outputs[0] = *(unsigned long *)inputs[0] + 3;
    *(unsigned long *)inputs[0] = *(unsigned long *)outputs[0];
    // unsigned int res = ACCESS_INPUTS(0, unsigned long) + ACCESS_OUTPUTS(0,
    // unsigned long);
    // WRITE_OUTPUTS(0, unsigned long, res);

    // sleep(1);
    // ddq_log("f_add %ld\n", *(unsigned long *)outputs[0]);

    // if (*(unsigned long *)outputs[0] > 1000)
    // printf("f_add finish!\n");
    if (*(unsigned long *)outputs[0] > 78)
        return task_ret_done;
    else
        return task_ret_ok;
}
// #undef ddq_op_region
// task_ret	f_print(void **inputs, void **outputs, void **attribute);
// task_ret	f_add(void **inputs, void **outputs, void **attribute);

ddq_ring acc_ring() {

    ddq_ring ring = ddq_new(NULL, 1, 0);

    obj f_a, f_p, f_i, ob, buf;
    ddq_op iob, acc, pob;
    f_i = obj_import(ring, f_init, NULL, obj_prop_ready);
    f_a = obj_import(ring, f_acc, NULL, obj_prop_ready);
    f_p = obj_import(ring, f_print, NULL, obj_prop_ready);

    ob = obj_new(ring, new_long, free_long, 0);
    buf = buffer_obj_new(ring, new_buffer, free_long, sizeof(long), 5);

    iob = ddq_spawn(ring, processor_pthread_pool, 1, 1);
    ddq_add_f(iob, f_i);
    ddq_add_inputs(iob, 0, ring->inputs[0]);
    ddq_add_outputs(iob, 0, ob);

    acc = ddq_spawn(ring, processor_pthread_pool, 1, 1);
    ddq_add_f(acc, f_a);
    ddq_add_inputs(acc, 0, ob);
    ddq_add_outputs(acc, 0, buf);

    pob = ddq_spawn(ring, processor_pthread_pool, 1, 0);
    ddq_add_f(pob, f_p);
    ddq_add_inputs(pob, 0, buf);

    return ring;
}

int main() {
    printf("begin\n");
    ddq_ring ring = acc_ring();
    ddq_update(ring);
    // void* ptr = pack_ring(ring);
    // ddq_delete(ring);
    // ddq_ring ring3 = unpack_ring(ptr);
    // ddq_ring ring4 = unpack_ring(ptr);
    // ddq_delete(ring3);
    // ddq_delete(ring4);
    // ddq_ring ring2 = unpack_ring(ptr);
    // free(ptr);
    ddq_ring ring2 = ring;

    unsigned long *a0 = malloc(sizeof(unsigned long));
    *a0 = 0;

    ring2->inputs[0] = obj_import(ring, a0, NULL, obj_prop_ready);
    ddq_loop_init();
    ddq_loop(ring2, 0);

    // int i;
    // for(i=0; i<ring2->n_ops; i++){
    //     meta log = name2meta(ring2->metadatas[i], NAME_DDQ_LOG);
    //     printf("%s\n", log->value.sval);
    // }
    printf("Done!\n");

    ddq_delete(ring2);
    // TODO pack要修改 delete——ring
    return 0;
}
