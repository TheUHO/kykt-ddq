ddq_start();

if (!o_sv(pool))
{
	o_sv(pool) = stream_pool_init();
	if(!o_sv(pool)){
		ddq_error("processor_cuda : malloc() fails.\n");
	}
}
o_sv(pool)->n_ref++;

ddq_do();

ddq_waitfor((stream_pool_submit(o_sv(pool), ddq_p)) > 0);

ddq_waitfor(stream_pool_query(o_sv(pool), ddq_p) > 0);
o_sv(pool)->status[o_v(istream)] = stream_status_available;

ddq_while( o_v(ret));

// TODO: 什么时候清理streams？
o_sv(pool)->n_ref--;
if(o_sv(pool)->n_ref == 0){
	stream_pool_destroy(o_sv(pool));
	o_sv(pool) = NULL;
}

ddq_finish();


