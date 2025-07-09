#include	<string.h>			// memset()

#include	"ddq_plugin.h"
#include	"ddq_types.h"
#include	"ddq_include_gen.h"
#include 	"ddq.h"

static	void *	ddq_type_share[ddq_type_last];	// NOTE: 默认初始化为NULL

#define	ddq_enable(x)	sizeof(struct x##_t),
static	int_t	type_size[ddq_type_last+1] =
{
	0,
#include	"ddq_types_list.h"
	0
};
#undef	ddq_enable

//p_inputs[-1]是算子只读的元数据，可写的元数据由processor合并？
static inline void ddq_meta_init(ddq_op op){
	int ddq_log_init_size = DDQ_LOG_INIT_SIZE;
	meta log_size = meta_new(NAME_DDQ_LOG_SIZE, meta_type_int, &ddq_log_init_size);
	log_size->next = op->metadata;
	op->metadata = log_size;
	meta log = meta_new(NAME_DDQ_LOG, meta_type_string, NULL);
	log->value.sval = malloc(DDQ_LOG_INIT_SIZE);
	memset(log->value.sval, 0, DDQ_LOG_INIT_SIZE);
	log->next = op->metadata;
	op->metadata = log;

	op->p_inputs[-1] = op->metadata;
}

static inline void* buffer_obj_reading(obj obj_){
	buffer_obj buf = (buffer_obj)obj_;
	int_t i;
	for(i=0; i<buf->size && buf->element_status[i] != obj_status_toread; i++);
	if(i >= buf->size) return NULL;
	buf->element_status[i] = obj_status_reading;
	// buf->count--;
	return (char*)obj_->p + i * buf->element_size;
}

static inline void* buffer_obj_writing(obj obj_){
	buffer_obj buf = (buffer_obj)obj_;
	int_t i;
	for(i=0; i<buf->size && buf->element_status[i] != obj_status_towrite; i++);
	if(i >= buf->size) return NULL;
	buf->element_status[i] = obj_status_writing;
	// buf->count++;
	return (char*)obj_->p + i * buf->element_size;
}

static inline void buffer_update_status(obj obj_){
	buffer_obj buf = (buffer_obj)obj_;
	obj_->status = obj_->status & ~obj_status_toread & ~obj_status_towrite;
	if(buf->count > 0) obj_->status = obj_->status | obj_status_toread;
	if(buf->count < buf->size) obj_->status = obj_->status | obj_status_towrite;
}

static inline void buffer_obj_toread(obj obj_, void* ptr){
	buffer_obj buf = (buffer_obj)obj_;
	int_t idx = ((char*)ptr - (char*)obj_->p) / buf->element_size;
	buf->element_status[idx] = obj_status_toread;
	buf->count++;
}

static inline void buffer_obj_towrite(obj obj_, void* ptr){
	buffer_obj buf = (buffer_obj)obj_;
	int_t idx = ((char*)ptr - (char*)obj_->p) / buf->element_size;
	buf->element_status[idx] = obj_status_towrite;
	buf->count--;
}

static inline	void	ddq_init(ddq_ring ring, ddq_op op)
{
	int_t	i;
	int_t idx;

	if (o_f)//FIXME:是否允许算子f为toassign，从ring的输入中寻找？
	{
		op->ver_f = o_f->version;
		// if(!(o_f->prop & obj_prop_read_contest))
			o_f->n_readers ++;
		if (o_f->prop & obj_prop_consumable && !(o_f->status & obj_status_toread))
			op->ver_f = 1 - op->ver_f;
	}

	for (i = 0; i < o_n_in; i++)
		if (o_in[i])
		{
			if(o_in[i]->prop & obj_prop_toassign){
				idx = -1 * (int_t) o_in[i]->p - 1;//FIXME:是否仅允许算子的输入是ring的输入，算子的输出为ring的输出
				if(!(--o_in[i]->n_ref))
					ddq_free(ring, o_in[i]);
				o_in[i] = ring->inputs[idx];
				o_in[i]->n_ref++;
			}
			op->ver_inputs[i] = o_in[i]->version;
			// if(!(o_in[i]->prop & obj_prop_read_contest))
				o_in[i]->n_readers ++;
			if (o_in[i]->prop & obj_prop_consumable && !(o_in[i]->status & obj_status_toread))
				op->ver_inputs[i] = 1 - op->ver_inputs[i];
		}

	for (i = 0; i < o_n_out; i++)
		if (o_out[i])
		{
			if(o_out[i]->prop & obj_prop_toassign){
				idx = (int_t) o_out[i]->p - 1;
				if(!(--o_out[i]->n_ref))
					ddq_free(ring, o_out[i]);
				o_out[i] = ring->outputs[idx];
				o_out[i]->n_ref++;
			}
			// if(!(o_in[i]->prop & obj_prop_write_contest))
				o_out[i]->n_writers ++;
		}
}

//TODO bug
static inline	flag_t	ddq_prefail(ddq_op op)
{
	int_t	i;

	if (o_f && (o_f->prop & obj_prop_consumable) && !(o_f->status & obj_status_toread) && o_f->n_writers == 0)	// FIXME: 其他状态应当怎么做？
		return	1;

	for (i = 0; i < o_n_in; i++)
		if(o_in[i] && o_in[i]->n_writers == 0){
			if((o_in[i]->prop & obj_prop_consumable) && !(o_in[i]->status & obj_status_toread))
				return 1;
		}

	return	0;
}
static inline	flag_t	ddq_prerun(ddq_op op)
{	
	
	int_t	i, j;
	obj	pdup;

	if(o_f){
		if(o_f->prop & obj_prop_buffer){
			if(!(o_f->p = buffer_obj_reading(o_f)))
				return 0;
		}
		else if(!(o_f->status & obj_status_toread) || o_f->version != op->ver_f)
			return 0;
	}

	for (i = 0; i < o_n_in; i++)
		if (o_in[i] && (!(o_in[i]->status & obj_status_toread) || (!(o_in[i]->prop & obj_prop_buffer) && (o_in[i]->version != op->ver_inputs[i]))) &&
			!(o_in[i]->status & obj_status_toalloc && o_in[i]->n_writers == 0))//如果writer为0，是否可以默认构建函数可以写
			return	0;

	for (i = 0; i < o_n_out; i++)
		if (o_out[i] && !(o_out[i]->status & obj_status_towrite) && !(o_out[i]->status & obj_status_toalloc)) //?
		{
			for (j = 0; j < o_n_in && o_in[j] != o_out[i]; j++);
			if (j == o_n_in && o_out[i] != o_f){
				return	0;
			}
			if (o_out[i]->n_readers != o_out[i]->n_reads + 1)
				return	0;
		}
	
	for(i = 0; i < o_n_in; i++)
		if(o_in[i])
		{
			if(o_in[i]->status & obj_status_toalloc && o_in[i]->n_writers == 0)
			{
				o_in[i]->p = o_in[i]->constructor();
				for (pdup = o_in[i]->dup; pdup != o_in[i]; pdup = pdup->dup)
				{	
					pdup->p = o_in[i]->p;
					pdup->status = obj_status_toread;		// FIXME: 这里有并发冲突的问题！是否应当允许冲突并交由用户负责以提高灵活性？
				}
				o_in[i]->status = obj_status_toread;
			}
		}
	
	for (i = 0; i < o_n_out; i++)
		if (o_out[i])
		{
			if (o_out[i]->status & obj_status_toalloc)
			{
				o_out[i]->p = o_out[i]->constructor();
				for (pdup = o_out[i]->dup; pdup != o_out[i]; pdup = pdup->dup)
				{	
					pdup->p = o_out[i]->p;
					pdup->status = obj_status_towrite;		// FIXME: 这里有并发冲突的问题！是否应当允许冲突并交由用户负责以提高灵活性？
				}
				o_out[i]->status = obj_status_towrite;
			}
		}

	if(o_f){
		if(o_f->prop & obj_prop_buffer){
			if(!(op->p_f = buffer_obj_reading(o_f)))
				return 0;
		}
		else
			op->p_f = o_f->p;
	}

	for (i = 0; i < o_n_in; i++)
		if (o_in[i]){
			if(o_in[i]->prop & obj_prop_buffer){
				if(!(o_pin[i] = buffer_obj_reading(o_in[i])))
					return 0;
			}
			else
				o_pin[i] = o_in[i]->p;
		}
	for (i = 0; i < o_n_out; i++)
		if (o_out[i]){
			if(o_out[i]->prop & obj_prop_buffer){
				if(!(o_pout[i] = buffer_obj_writing(o_out[i])))
					return 0;
			}
			else
				o_pout[i] = o_out[i]->p;
		}
	return	1;
}

static inline	void	ddq_postrun(ddq_ring ring, ddq_op op, ddq_op prev_op, flag_t ret)
{
	if(ret == task_ret_again)
		return;
	
	int_t	i;

	if (o_f && o_f->prop & obj_prop_consumable)
	{
		o_f->n_reads ++;
		if (o_f->n_readers == o_f->n_reads)
			o_f->status = obj_status_towrite;
		op->ver_f = 1 - op->ver_f;
		// o_f->p = o_pf;
	}

	if(ret != task_ret_read_partial)
		for (i = 0; i < o_n_in; i++)
		{
			if (o_in[i])
			{
				if (o_in[i]->prop & obj_prop_buffer)
				{
					buffer_obj_towrite(o_in[i], o_pin[i]);
					buffer_update_status(o_in[i]);
				}else{
					if (o_in[i]->prop & obj_prop_consumable)
					{
						o_in[i]->n_reads ++;
						if (o_in[i]->n_readers == o_in[i]->n_reads)
							o_in[i]->status = obj_status_towrite;
						op->ver_inputs[i] = 1 - op->ver_inputs[i];
					}
					o_in[i]->p = o_pin[i];
				}
			}
		}

	if(ret != task_ret_write_partial)
		for (i = 0; i < o_n_out; i++)
		{
			if (o_out[i]){
				if (o_out[i]->prop & obj_prop_buffer)
				{
					buffer_obj_toread(o_out[i], o_pout[i]);
					buffer_update_status(o_out[i]);
				}
				else
				{
					o_out[i]->n_reads = 0;
					if (o_out[i]->prop & obj_prop_consumable)
						o_out[i]->version = 1 - o_out[i]->version;
					o_out[i]->status = obj_status_toread;
					o_out[i]->p = o_pout[i];
				}
			}
		}
	//更新邻居表
	i=0;
	while(i < op->n_neighbors){
		if(ring->status_ops[op->neighbors[i]->uid] == op_status_removed){
			op->neighbors[i] = op->neighbors[--op->n_neighbors];
			continue;
		}
		else if(ring->status_ops[op->neighbors[i]->uid] == op_status_blocked){
			ring->status_ops[op->neighbors[i]->uid] = op_status_ready;
			op->neighbors[i]->next = op;
			prev_op->next = op->neighbors[i];
			prev_op = op->neighbors[i];
		}
		i++;
	}
}

static inline	flag_t	ddq_check_final(ddq_op op, flag_t ret)
{
	int_t	i;
	if(ret != task_ret_read_partial){
		for(i = 0; i < o_n_in; i++){
			if(o_in[i] && (o_in[i]->prop & obj_prop_consumable) && o_in[i]->n_writers == 0){
				if(!(o_in[i]->prop & obj_prop_buffer))
					return 1;
				else if(!(o_in[i]->status & obj_status_toread))
					return 1;
			} 
		}
			
	}
	for (i = 0; i < o_n_out; i++)
		if(o_out[i]){
			if(!(o_out[i]->prop & obj_prop_consumable))
				return 1;
			else if((o_out[i]->prop & obj_prop_consumable) && o_out[i]->n_readers == 0)
				return 1;
		}

	return	0;
}


static inline	void	ddq_prefinal(ddq_ring ring, ddq_op op)
{
	int_t	i, j;
	obj	pdup;

	if (o_f)
	{
		o_f->n_readers --;
		if(!(o_f->prop & obj_prop_buffer)){
			if (o_f->prop & obj_prop_consumable && o_f->version != op->ver_f)
				o_f->n_reads --;
			if (o_f->prop & obj_prop_consumable && o_f->n_readers == o_f->n_reads)
				o_f->status = obj_status_towrite;
		}
	}
	for (i = 0; i < o_n_in; i++)
		if (o_in[i])
		{
			o_in[i]->n_readers --;
			if(!(o_in[i]->prop & obj_prop_buffer)){
				if (o_in[i]->prop & obj_prop_consumable && o_in[i]->version != op->ver_inputs[i])
					o_in[i]->n_reads --;
				if (o_in[i]->prop & obj_prop_consumable && o_in[i]->n_readers == o_in[i]->n_reads)
					o_in[i]->status = obj_status_towrite;
			}
		}
	for (i = 0; i < o_n_out; i++)
		if (o_out[i])
		{
			o_out[i]->n_writers --;
		}

	if (o_f && !(--o_f->n_ref))
	{
		if (o_f->dup == o_f && o_f->destructor)
			o_f->destructor(o_f->p);
		for (pdup = o_f->dup; pdup->dup != o_f; pdup = pdup->dup);
		pdup->dup = o_f->dup;
		o_f->p = NULL;
		ddq_free(ring, o_f);
		for (j = 0; j < o_n_in; j++)
			if (o_in[j] == o_f)
				o_in[j] = NULL;
	}
	o_f = NULL;

	for (i = 0; i < o_n_in; i++)
		if (o_in[i])
		{
			if (!(--o_in[i]->n_ref))
			{
				if (o_in[i]->dup == o_in[i] && o_in[i]->destructor)
					o_in[i]->destructor(o_in[i]->p);
				for (pdup = o_in[i]->dup; pdup->dup != o_in[i]; pdup = pdup->dup);
				pdup->dup = o_in[i]->dup;
				o_in[i]->p = NULL;
				ddq_free(ring, o_in[i]);
				for (j = i+1; j < o_n_in; j++)
					if (o_in[j] == o_in[i])
						o_in[j] = NULL;
				for (j = 0; j < o_n_out; j++)
					if (o_out[j] == o_in[i])
						o_out[j] = NULL;
			}
			o_in[i] = NULL;
		}

	for (i = 0; i < o_n_out; i++)
		if (o_out[i])
		{
			if (!(--o_out[i]->n_ref))
			{
				if (o_out[i]->dup == o_out[i] && o_out[i]->destructor)
					o_out[i]->destructor(o_out[i]->p);
				for (pdup = o_out[i]->dup; pdup->dup != o_out[i]; pdup = pdup->dup);
				pdup->dup = o_out[i]->dup;
				o_out[i]->p = NULL;
				ddq_free(ring, o_out[i]);
				for (j = i+1; j < o_n_out; j++)
					if (o_out[j] == o_out[i])
						o_out[j] = NULL;
			}
			o_out[i] = NULL;
		}
	
	// ring->metadatas[op->uid] = o_pin[-1];
}


void	ddq_debug_log_obj(obj p)
{
	if (p)
	{
		ddq_log0("OBJ:%lx\t", (int_t)p);	// FIXME: 这个目前只适合当前with_name的设计，下同
		ddq_log0("ptr = %lx\t", (int_t)(p->p));
		switch (p->status)
		{
			case obj_status_toalloc :
				ddq_log0("status = toalloc\t");
				break;
			case obj_status_towrite :
				ddq_log0("status = towrite\t");
				break;
			case obj_status_writing :
				ddq_log0("status = writing\t");
				break;
			case obj_status_toread :
				ddq_log0("status = toread\t");
				break;
		}
		if (p->prop & obj_prop_consumable)
		{
			ddq_log0("readers = (%ld/%ld)\t", p->n_reads, p->n_readers);
			ddq_log0("writers = (%ld)", p->n_writers);
		}
		ddq_log0("\n");
	}
	else
		ddq_log0("OBJ:NULL\n");
}

void	ddq_debug_log_op(ddq_ring ring, ddq_op p)
{
	int_t	i;
	ddq_log0("OP:%lx uid: %d status:%d\n", (int_t)p, (int)p->uid, (int)ring->status_ops[p->uid]);
	for (i = 0; i < p->n_neighbors; i++)
	{
		ddq_log0("\tneighbors[%ld] \t= \t%lx\t", i, (int_t)p->neighbors[i]);
	}
	ddq_log0("\n");
	ddq_log0("\tf \t= \t");
	ddq_debug_log_obj(p->f);
	for (i = 0; i < p->n_inputs; i++)
	{
		ddq_log0("\tin[%ld] \t= \t", i);
		ddq_debug_log_obj(p->inputs[i]);
	}
	for (i = 0; i < p->n_outputs; i++)
	{
		ddq_log0("\tout[%ld] \t= \t", i);
		ddq_debug_log_obj(p->outputs[i]);
	}
}

void	ddq_debug_iter(ddq_ring ring, ddq_op op)
{
	static	int_t	iter = 0;
	ddq_op	p;

	ddq_log("start iter #%ld.\n", iter);
	p = op;
	ddq_debug_log_op(ring, p);
	for (p = p->next; p != op; p = p->next)
		ddq_debug_log_op(ring, p);
	ddq_log("stop iter #%ld.\n\n", iter);

	iter ++;
}

void	ddq_loop_init()
{
#define	ddq_enable(x)	ddq_type_init(x);
#include	"ddq_types_list.h"
#undef	ddq_enable

}

void	ddq_loop(ddq_ring ring, int nstep)
{
	if(!ring || !ring->n_ops)
		return;
	int_t i;
	for(i=0; i<ring->n_inputs; i++){
		if(ring->inputs[i]->prop & obj_prop_toassign){
			ddq_error("ddq_loop() : Input object %ld is toassign, but not assigned.\n", i);
			return;
		}
	}
	for(i=0; i<ring->n_outputs; i++){
		if(ring->outputs[i]->prop & obj_prop_toassign){
			ddq_error("ddq_loop() : Output object %ld is toassign, but not assigned.\n", i);
			return;
		}
	}
	
	int	istep = nstep;
	ddq_op op = ring->ops;
	ddq_op prev_op = ring->ops;
	int nops = ring->n_ops;

ddq_start_point :

	if (nstep>0 && istep--==0)
		return;

	while (op && !op->next->pos)
		if (op->next == op)
		{
			ring->status_ops[op->uid] = op_status_removed;
			ddq_free(ring, op);
			op = NULL;
			ring->ops = NULL;
			ring->n_ops--;
		}
		else
		{
			ddq_op t = op->next->next;
			if(ring->ops == op->next)
				ring->ops = t;
			ring->status_ops[op->next->uid] = op_status_removed;
			ddq_free(ring, op->next);
			ring->n_ops--;
			op->next = t;
		}


	if (op)
	{
		prev_op = op;
		op = prev_op->next;
		// printf("ddq_loop() : op->uid = %d\n", op->uid);
		// printf("ddq_loop() : prev_op->uid = %d\n", prev_op->uid);
		// if(ring->status_ops[op->uid] != op_status_ready){
		// 	goto ddq_start_point;
		// }
		//计算中op的个数
		// printf("ddq_loop() : ring->n_ops = %d\n", ring->n_ops);
		//输出status_ops
		// for(int i=0; i<nops; i++){
		// 	printf("%d ", ring->status_ops[i]);
		// }
		// printf("\n");


		// #ifdef enable_processor_dsp
		// ddq_log0("thread_id:%d op:%p\n", get_thread_id(), op);
		// if(get_thread_id() == 0)
			// ddq_debug_iter(ring, op);		// TODO: 这行平时用宏关掉
		// #endif

		switch (op->pos)
		{
			case 0:
				ddq_warning("ddq_loop() : Why is it here? pos==0 means being removed.\n");
				goto ddq_start_point;
#include	"ddq_section_gen.h"

			default :
				ddq_warning("ddq_loop() : Why is it here? Wrong pos!\n");
		}
	}
}