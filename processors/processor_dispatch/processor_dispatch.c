ddq_start();

ddq_do();

ddq_waitfor((o_v(dsp_id) = get_dsp(ddq_s)) >= 0);
o_sv(dsp_status)[o_v(dsp_id)] = dsp_status_busy;

ddq_waitfor((o_v(ret) = o_pf(o_pin, o_pout, o_pattr, o_v(dsp_id))) != task_ret_again);

o_sv(dsp_status)[o_v(dsp_id)] = dsp_status_idle;

ddq_while( ret_value);

ddq_finish();