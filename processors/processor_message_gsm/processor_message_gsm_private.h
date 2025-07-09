#include    "std/std_types/std_types.h"
#include    "ddq_plugin.h"
#include    "processor_message_gsm.h"
#include    "hthread_device.h"
#include    "ddq.h"
#include    <string.h>
#include    <stdio.h>
#include    <stdlib.h>

#define dsp_num 24

// __gsm__ int messages[1];
// __gsm__ message_status_t status[1];
// __gsm__ int versions[dsp_num];

int is_sender(struct processor_message_gsm_t* p){
    return p->role == message_role_sender;
}

int is_receiver(struct processor_message_gsm_t* p){
    return p->role == message_role_receiver;
}

void message_gsm_initial(struct processor_message_gsm_t* p){
    p->version = 0;
    for(int i=0; i<dsp_num; i++){
        versions[i] = 0;
    }

    int_t send_mask = *(int_t*)p->head.p_inputs[p->head.n_inputs - 3];
    int_t receive_mask = *(int_t*)p->head.p_inputs[p->head.n_inputs - 2];

    int thread_id = get_thread_id();

    if(send_mask & ((int_t)1 << thread_id)){
        p->role = message_role_sender;
    }
    else if(receive_mask & ((int_t)1 << thread_id)){
        p->role = message_role_receiver;
    }
}

int prerun_transfer_message(struct processor_message_gsm_t* p){
    int_t send_mask = *(int_t*)p->head.p_inputs[p->head.n_inputs - 3];
    int_t receive_mask = *(int_t*)p->head.p_inputs[p->head.n_inputs - 2];

    if(is_sender(p)){
        for(int i=0; i<dsp_num; i++){
            if(receive_mask & ((int_t)1 << i) && (versions[i] & ~p->version)){
                return 0;
            }
        }
    }

    if(is_receiver(p)){
        for(int i=0; i<dsp_num; i++){
            if(send_mask & ((int_t)1 << i) && (versions[i] & p->version)){
                return 0;
            }
        }
    }

    return 1;
}

void transfer_message(struct processor_message_gsm_t* p){
    int_t send_mask = *(int_t*)p->head.p_inputs[p->head.n_inputs - 3];
    int_t receive_mask = *(int_t*)p->head.p_inputs[p->head.n_inputs - 2];
    int message = *(int*)p->head.p_inputs[p->head.n_inputs - 1];

    int id = p->thread_id;
    //send
    if(is_sender(p)){
        messages[0] = 1;
        versions[id] = 1 - versions[id];
    }
    //receive
    else if(is_receiver(p)){
        versions[id] = 1 - versions[id];
    }
}

