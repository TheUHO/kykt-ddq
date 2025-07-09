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

static inline	void	ddq_init(ddq_ring ring, ddq_op op)
{
	int_t	i;
	int_t idx;

	if (o_f)//FIXME:是否允许算子f为toassign，从ring的输入中寻找？
	{
		op->ver_f = o_f->version;
		if(!(o_f->prop & obj_prop_read_contest))
			o_f->n_readers ++;
		if (o_f->prop & obj_prop_consumable && o_f->status != obj_status_toread)
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
			if(!(o_in[i]->prop & obj_prop_read_contest))
				o_in[i]->n_readers ++;
			if (o_in[i]->prop & obj_prop_consumable && o_in[i]->status != obj_status_toread)
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

	if (o_f && o_f->prop & obj_prop_consumable && o_f->status == obj_status_towrite && o_f->n_writers == 0)	// FIXME: 其他状态应当怎么做？
		return	1;

	for (i = 0; i < o_n_in; i++)
		if (o_in[i] && o_in[i]->prop & obj_prop_consumable && o_in[i]->status == obj_status_towrite && o_in[i]->n_writers == 0)
			return	1;

	return	0;
}

static inline	flag_t	ddq_prerun(ddq_op op)
{	
	
	int_t	i, j;
	obj	pdup;

	if (o_f && o_f->status != obj_status_toread)
		return	0;

	if (o_f && !(o_f->prop & obj_prop_read_contest) && (o_f->version != op->ver_f))
		return	0;

	for (i = 0; i < o_n_in; i++)
		if (o_in[i])
		{
			if (o_in[i]->status != obj_status_toread)
				return	0;
			if (!(o_in[i]->prop & obj_prop_read_contest) && (o_in[i]->version != op->ver_inputs[i]))
				return	0;

		}

	for (i = 0; i < o_n_out; i++)
		if (o_out[i] && o_out[i]->status != obj_status_towrite && o_out[i]->status != obj_status_toalloc) //?
		{
			for (j = 0; j < o_n_in && o_in[j] != o_out[i]; j++);
			if (j == o_n_in && o_out[i] != o_f){
				return	0;
			}
			if (o_out[i]->n_readers != o_out[i]->n_reads + 1)
				return	0;
		}
	
	for (i = 0; i < o_n_out; i++)
		if (o_out[i])
		{
			if (o_out[i]->status == obj_status_toalloc)
			{
				o_out[i]->p = o_out[i]->constructor();
				for (pdup = o_out[i]->dup; pdup != o_out[i]; pdup = pdup->dup)
				{	
					pdup->p = o_out[i]->p;
					pdup->status = obj_status_towrite;		// FIXME: 这里有并发冲突的问题！是否应当允许冲突并交由用户负责以提高灵活性？
				}
			}
			o_out[i]->status = obj_status_towrite;
		}

	// o_pf = o_f->p;
	for (i = 0; i < o_n_in; i++)
		if (o_in[i])
			o_pin[i] = o_in[i]->p;
	for (i = 0; i < o_n_out; i++)
		if (o_out[i])
			o_pout[i] = o_out[i]->p;
	return	1;
}

// static inline	void	ddq_postread(ddq_op op)
// {
// 	int_t	i;

// 	if (o_f && o_f->prop & obj_prop_consumable)
// 	{
// 		o_f->n_reads ++;
// 		if (o_f->n_readers == o_f->n_reads)
// 			o_f->status = obj_status_towrite;
// 		op->ver_f = 1 - op->ver_f;
// 	}

// 	for (i = 0; i < o_n_in; i++)
// 		if (o_in[i])
// 		{
// 			if (o_in[i]->prop & obj_prop_consumable)
// 			{
// 				o_in[i]->n_reads ++;
// 				if (o_in[i]->n_readers == o_in[i]->n_reads)
// 					o_in[i]->status = obj_status_towrite;
// 				op->ver_inputs[i] = 1 - op->ver_inputs[i];
// 			}
// 		}
// }
// static inline	void	ddq_postwrite(ddq_op op)
// {
// 	int_t	i;
	
// 	for (i = 0; i < o_n_out; i++)
// 		if (o_out[i])
// 		{
// 			o_out[i]->n_reads = 0;
// 			if (o_out[i]->prop & obj_prop_consumable)
// 				o_out[i]->version = 1 - o_out[i]->version;
// 			o_out[i]->status = obj_status_toread;
// 		}
// }

static inline	void	ddq_prefinal(ddq_ring ring, ddq_op op)
{
	int_t	i, j;
	obj	pdup;

	if (o_f)
	{
		if(!(o_f->prop & obj_prop_read_contest)){
			o_f->n_readers --;
			if (o_f->prop & obj_prop_consumable && o_f->version != op->ver_f)
				o_f->n_reads --;
			if (o_f->prop & obj_prop_consumable && o_f->n_readers == o_f->n_reads)
				o_f->status = obj_status_towrite;
		}
	}
	for (i = 0; i < o_n_in; i++)
		if (o_in[i])
		{
			if(!(o_in[i]->prop & obj_prop_read_contest)){
				o_in[i]->n_readers --;
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
	
	for(i=0; i<op->n_neighbors; i++){
		ddq_op neighbor = op->neighbors[i];
		if(!neighbor) continue;
		for(j=0; j<neighbor->n_neighbors; j++){
			if(neighbor->neighbors[j] == op){
				neighbor->neighbors[j] = NULL;
			}
		}
		if(ddq_prefail(neighbor)){
			ddq_prefinal(ring, neighbor);
			neighbor->pos = 0;
		}
	}
	// ring->metadatas[op->uid] = o_pin[-1];
}

static inline	void	ddq_postrun(ddq_ring ring, ddq_op op, flag_t ret)
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

	for (i = 0; i < o_n_in; i++)
		if (o_in[i])
		{
			if (o_in[i]->prop & obj_prop_consumable)
			{

				// if(o_in[i]->status < obj_status_reading)
				// 	o_in[i]->status = obj_status_reading;
				// o_in[i]->status ++; //读不需要独占,计数

				if(ret != task_ret_read_partial)
				{
					o_in[i]->n_reads ++;
					if (o_in[i]->n_readers == o_in[i]->n_reads)
						o_in[i]->status = obj_status_towrite;
					op->ver_inputs[i] = 1 - op->ver_inputs[i];
				}
			}
			o_in[i]->p = o_pin[i];
		}

	for (i = 0; i < o_n_out; i++)
		if (o_out[i])
		{
			if(ret != task_ret_write_partial){
				o_out[i]->n_reads = 0;
				if (o_out[i]->prop & obj_prop_consumable)
					o_out[i]->version = 1 - o_out[i]->version;
				o_out[i]->status = obj_status_toread;
			}
			o_out[i]->p = o_pout[i];
		}

	for(i=0; i<op->n_neighbors; i++){
		if(!op->neighbors[i]) continue;
		if(ddq_prerun(op->neighbors[i])){
			op->neighbors[i]->runnable = 1;
		}
	}

	if(!ddq_prerun(op)){
		op->runnable = 0;
	}
}

static inline	flag_t	ddq_check_final(ddq_op op)
{
	int_t	i;

	for (i = 0; i < o_n_out; i++)
		if (o_out[i] && !(o_out[i]->prop & obj_prop_consumable))
			return	1;

	if (o_f && o_f->prop & obj_prop_consumable && o_f->status == obj_status_towrite && o_f->n_writers == 0)	// FIXME: 其他状态应当怎么做？
		return	1;

	for (i = 0; i < o_n_in; i++)
		if (o_in[i] && o_in[i]->prop & obj_prop_consumable && o_in[i]->n_writers == 0 && (o_in[i]->prop != obj_prop_read_contest || o_in[i]->status == obj_status_towrite))
			return	1;
	
	return	0;
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

void	ddq_debug_log_op(ddq_op p)
{
	int_t	i;
	ddq_log0("OP:%lx runnable:%ld pos:%ld: \n", (int_t)p, p->runnable, p->pos);
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

void	ddq_debug_iter(ddq_op op)
{
	static	int_t	iter = 0;
	ddq_op	p;

	ddq_log("start iter #%ld.\n", iter);
	p = op;
	ddq_debug_log_op(p);
	for (p = p->next; p != op; p = p->next)
		ddq_debug_log_op(p);
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
	// ring->metadatas = malloc(sizeof(void*) * ring->n_ops);
	// memset(ring->metadatas, 0, sizeof(void*) * ring->n_ops);
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

ddq_start_point :

	if (nstep>0 && istep--==0)
		return;

	while (op && !op->next->pos)
		if (op->next == op)
		{
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
			ddq_free(ring, op->next);
			ring->n_ops--;
			op->next = t;
		}


	if (op)
	{
		// ddq_log0("thread_id:%d op:%p\n", get_thread_id(), )
		op = op->next;

		#ifdef enable_processor_dsp
		// ddq_log0("thread_id:%d op:%p\n", get_thread_id(), op);
		// if(get_thread_id() == 0)
		// 	ddq_debug_iter(op);		// TODO: 这行平时用宏关掉
		#endif

		if(!op->runnable){
			goto ddq_start_point;
		}

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
