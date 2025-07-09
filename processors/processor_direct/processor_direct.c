ddq_start();

ddq_do();

ddq_waitfor((o_v(ret) = o_pf(o_pin, o_pout, o_pattr)) != task_ret_again);

ddq_while( o_v(ret));

ddq_finish();


