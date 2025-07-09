ddq_start();

o_v(nstep) = 0;	// NOTE: 可以动态修改这个参数，实现调整不同环的相对优先级。如果设置为0则运行到结束才返回。

ddq_do();

{
	ddq_op	p;
	int	flag = 0;
	int	i, j;
	int size = ((obj_mem)o_f->p)->size + (o_n_in + o_n_out)*sizeof(obj_t);
	o_v(ring) = malloc(size);
	memcpy(o_v(ring), o_f->p, ((obj_mem)o_f->p)->size);
	o_v(ring) = unpack_ring(o_v(ring));	
	o_v(ring)->mem->size = size;

	// assert(o_v(ring)->n_inputs <= o_n_in);
	// assert(o_v(ring)->n_outputs <= o_n_out);
	for(i=0; i<o_n_in; i++){ 
		o_v(ring)->inputs[i] = obj_dup(o_v(ring), o_in[i]);
		o_v(ring)->inputs[i]->prop = obj_prop_consumable;
	}
	for(i=0; i<o_n_out; i++){
		for(j=0; j<o_n_in; j++) { // 处理输入和输出相同的情况
			if (o_in[j] == o_out[i]) {
				flag = 1;
				o_v(ring)->outputs[i] = o_v(ring)->inputs[j];
				break;
			}
		}
		if (!flag) o_v(ring)->outputs[i] = obj_dup(o_v(ring), o_out[i]);
		o_v(ring)->outputs[i]->prop = obj_prop_consumable;
		
		o_v(ring)->outputs[i]->status = obj_status_towrite;  // call的prerun执行后status会变成writing，这里把输出都变成towrite，否则ddq_run无法正常执行
		if (flag) o_v(ring)->outputs[i]->status = obj_status_toread; // 但是如果输出是某个输入，状态应该是toread
		flag = 0;
	}

}

ddq_loop_init();

ddq_waitfor( (o_v(ret) = ddq_run(ddq_p)) != task_ret_again);

meta op_meta = op->metadata;
char* once = "once"; 
while(op_meta) {
	switch (op_meta->type)
		{
			case meta_type_int:  // 找到once属性并且值为1表明这个算子只会执行一次
				if (strcmp(op_meta->name, once) == 0 && op_meta->value.ival == 1) {
					o_v(ret) = task_ret_done; // 变相让call有了done的返回值
				}
				break;
			default:
				break;
		}
	op_meta = op_meta->next;
}

ddq_while(o_v(ret));		// FIXME: 确认不需要主动析构这个算子？ 什么时候应该析构？

ddq_finish();


