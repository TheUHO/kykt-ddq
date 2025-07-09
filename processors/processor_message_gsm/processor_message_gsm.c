ddq_start();

ddq_do();

message_gsm_initial(ddq_p);

if(is_sender(ddq_p)){
    ddq_waitfor((o_v(ret) = o_pf(o_pin, o_pout, o_pattr)) != task_ret_again);
}

prerun_transfer_message(ddq_p);

if(is_receiver(ddq_p)){
    ddq_waitfor((o_v(ret) = o_pf(o_pin, o_pout, o_pattr)) != task_ret_again);
}

transfer_message(ddq_p);


ddq_while(ret_value);

ddq_finish();