#include	<string.h>			// memset()

#include	"ddq_plugin.h"
#include	"ddq_types.h"
#include 	"basic_types.h"
#ifdef enable_processor_dsp
#include "hthread_device.h"
#endif
#include	"ddq_include_gen.h"
#include "std/std_types/std_types.h"
#include 	"error.h"
#include "ddq.h"

#define	ddq_enable(x)	sizeof(struct x##_t),
static	int_t	type_size[ddq_type_last+1] =
{
	0,
#include	"ddq_types_list.h"
	0
};
#undef	ddq_enable


//TODO:tianhe_processor对应的修改 metadata的内存管理 obj的meta如何管理，析构
//并发处理
meta meta_new(char*name, enum_meta_type t, void* v_ptr){
	if(!name)
		return NULL;
	meta res = malloc(sizeof(meta_t) + strlen(name) + 1);

	res->name = (char*)res + sizeof(meta_t);
	strcpy(res->name, name);

	res->type = t;
	res->next = NULL;

	switch(t){
	case meta_type_string:
		if(!v_ptr || !strlen(v_ptr)) 
			res->value.sval = NULL;
		else{
			res->value.sval = malloc(strlen(v_ptr) + 1);
			strcpy(res->value.sval, v_ptr);
		}
		break;
	case meta_type_int:
		res->value.ival = v_ptr ? *((int*)v_ptr) : 0;
		break;
	case meta_type_double:
		res->value.dval = v_ptr ? *((double*)v_ptr) : 0;
		break;
	default:
		ddq_error("meta_new() : Unsupported meta type.\n");
		break;
	}

	return res;
}

void meta_delete(meta metadata){
	if(!metadata) return;
	meta m = metadata;
	while(m){
		meta n = m->next;
		if(m->type == meta_type_string)
			free(m->value.sval);
		free(m);
		m = n;
	}
}

void meta_merge(meta* dst, meta src){
	if(!dst || !src) return;
	if(!(*dst)) {
		*dst = src;
		return;
	}

	meta m = *dst, n = src, tmp;
	while(m->next){
		m = m->next;
	}

	while(n){
		if(tmp = name2meta(*dst, n->name)){
			switch(tmp->type){
				case meta_type_string:
					{
						int_t len = strlen(tmp->value.sval) + strlen(n->value.sval) + 1;
						char* new_str = malloc(len);
						strcpy(new_str, tmp->value.sval);
						strcat(new_str, n->value.sval);
						free(tmp->value.sval);
						tmp->value.sval = new_str;
					}
					break;

				case meta_type_int:
					tmp->value.ival = n->value.ival;
					break;
					
				case meta_type_double:
					tmp->value.dval = n->value.dval;
					break;

				default:
					break;
			}
		}else{
			m->next = n;
			m = m->next;
		}
		n = n->next;
	}
}

meta name2meta(meta metadata, char* name){
	meta res = NULL, m = metadata;
	while(m){
		if(!strcmp(name, m->name)){
			res = m;
			break;
		}
		m = m->next;
	}
	return res;
}

obj	obj_new(ddq_ring ring, construct_f *pfc, destruct_f *pfd, enum_obj_prop prop)
{
	obj	res;

	if (prop & obj_prop_ready)
		ddq_error("obj_new() : Don't set obj_prop_ready on a new obj.\n");
	if (!pfc)
		ddq_error("obj_new() : A constructor is needed for a new obj.\n");

	res = ddq_malloc(ring, sizeof(obj_t));//ddq_malloc填入free指针
	// res->p = malloc(sizeof(void*));
	// *(void**)(res->p) = NULL;
	res->p = NULL;
	res->status = obj_status_toalloc;
	res->prop = prop;
	res->constructor = pfc;
	res->destructor = pfd;
	res->n_writers = 0;
	res->n_readers = 0;
	res->n_reads = 0;
	res->version = 0;
	res->dup = res;
	res->metadata = NULL;
	res->n_ref = 0;

	return res;
}

obj	buffer_obj_new(ddq_ring ring, construct_f *pfc, destruct_f *pfd, int_t element_size, int_t size)
{
	obj	res;

	// if (prop & obj_prop_ready)
	// 	ddq_error("buffer_obj_new() : Don't set obj_prop_ready on a new obj.\n");
	if (!pfc)
		ddq_error("buffer_obj_new() : A constructor is needed for a new obj.\n");

	res = ddq_malloc(ring, sizeof(buffer_obj_t) + sizeof(enum_obj_status) * size);//ddq_malloc填入free指针
	buffer_obj buf = res;
	
	res->p = NULL;
	res->status = obj_status_toalloc;
	res->prop = obj_prop_buffer | obj_prop_consumable;
	res->constructor = pfc;
	res->destructor = pfd;
	res->n_writers = 0;
	res->n_readers = 0;
	res->n_reads = 0;
	res->version = 0;
	res->dup = res;
	res->metadata = NULL;
	res->n_ref = 0;

	buf->count = 0;
	buf->element_size = element_size;
	buf->size = size;
	buf->element_status = (enum_obj_status*)((char*)res + sizeof(buffer_obj_t));
	for(int i=0; i<size; i++){
		buf->element_status[i] = obj_status_towrite;
	}

	return res;
}

meta meta_copy(meta metadata){
	meta m;
	int meta_count = 0, meta_size = 0;
	int i = 0;
	void* res, *str;

	for(m = metadata; m; m = m->next){
		meta_size += (sizeof(meta_t) + strlen(m->name) + 1);
		if(m->type == meta_type_string)
			meta_size += (strlen(m->value.sval) + 1);
		meta_count ++;
	}
	res = malloc(meta_size);
	str = res + meta_count * sizeof(meta_t);
	for(m = metadata; m; m = m->next){
		memcpy(res + i * sizeof(meta_t), m, sizeof(meta_t));
		memcpy(str, m->name, strlen(m->name) + 1);
		((meta)res + i)->name = str;
		str += (strlen(m->name) + 1);
		if(m->type == meta_type_string){
			memcpy(str, m->value.sval, strlen(m->value.sval) + 1);
			((meta)res + i)->value.sval = str;
			str += (strlen(m->value.sval) + 1);
		}	
	}

	return (meta)res;
}

obj	obj_new_with_meta(ddq_ring ring, meta metadata, construct_f *pfc, destruct_f *pfd, enum_obj_prop prop)
{
	obj	res;

	res = obj_new(ring, pfc, pfd, prop);
	
	res->metadata = meta_copy(metadata);

	return	res;
}


obj	obj_import(ddq_ring ring, void *p, destruct_f *pfd, enum_obj_prop prop)
{
	obj	res;

	res = ddq_malloc(ring, sizeof(obj_t));
	// res->p = malloc(sizeof(void*));
	// *(void**)(res->p) = p;
	res->p = p;
	res->status = (prop & obj_prop_ready) ? obj_status_toread : obj_status_towrite;
	res->prop = prop;
	res->constructor = NULL;
	res->destructor = pfd;
	res->n_writers = 0;
	res->n_readers = 0;
	res->n_reads = 0;
	res->version = 0;
	res->dup = res;
	res->metadata = NULL;
	res->n_ref = 0;

	return res;
}

obj	obj_import_with_meta(ddq_ring ring, meta metadata, void *p, destruct_f *pfd, enum_obj_prop prop)
{
	obj	res;

	// res = obj_import(p, pfd, prop);
	res = obj_import(ring, p, pfd, prop);

	res->metadata = meta_copy(metadata);

	return	res;
}

obj	obj_dup(ddq_ring ring, obj ob)
{
	obj	res;

	// res = malloc(sizeof(obj_t));
	res = ddq_malloc(ring, sizeof(obj_t));
	res->p = ob->p;
	res->status = ob->status;
	res->prop = ob->prop;
	res->constructor = ob->constructor;
	res->destructor = ob->destructor;
	res->n_writers = 0;
	res->n_readers = 0;
	res->n_reads = 0;
	res->version = 0;
	res->dup = ob->dup;
	ob->dup = res;
	res->metadata = NULL;
	res->n_ref = 0;

	return res;
}

obj	obj_dup_with_meta(ddq_ring ring, meta metadata, obj ob)

{
	obj	res;

	// res = obj_dup(ob);
	res = obj_dup(ring, ob);
	res->metadata = meta_copy(metadata);

	return	res;
}

// ddq_ring	ddq_new(int_t n_inputs, int_t n_outputs){
ddq_ring	ddq_new(ddq_ring top, int_t n_inputs, int_t n_outputs){
	obj_mem mem;
	if(!top){
		top = ddq_ring_init();
	}
	mem = top->mem;

	int_t size = sizeof(ddq_ring_t) + (n_inputs + n_outputs) * sizeof(obj);
	// ddq_ring	res = malloc(size);
	//这里有一点副作用，如果原本没有top，top在ddq_malloc时被修改了，需要提前存mem
	ddq_ring res = ddq_malloc(top, size);

	res->mem = mem;
	res->ops = NULL;
	res->inputs = (obj*)((char*)res + sizeof(ddq_ring_t));
	res->outputs = res->inputs + n_inputs;
	res->n_ops = 0;
	res->n_inputs = n_inputs;
	res->n_outputs = n_outputs;
	res->metadatas = NULL;

	int_t i;
	for(i=0; i<res->n_inputs; i++){
		// res->inputs[i] = obj_import((void*)(-i-1), NULL, obj_prop_toassign);
		res->inputs[i] = obj_import(res, (void*)(-i-1), NULL, obj_prop_toassign);
	}
	for(i=0; i<res->n_outputs; i++){
		// res->outputs[i] = obj_import((void*)(i+1), NULL, obj_prop_toassign);
		res->outputs[i] = obj_import(res, (void*)(i+1), NULL, obj_prop_toassign);
	}

	return res;
}

//TODO:free(metadata)
void		ddq_delete(ddq_ring p)
{
	if(p->metadatas){
		int i;
		for(i=0; i<p->n_ops; i++){
			meta_delete(p->metadatas[i]);
		}
		free(p->metadatas);
	}
	// free(p);
	ddq_free(p, p);
	p = NULL;
}

ddq_op	ddq_spawn(ddq_ring _ring, ddq_type_t type, int_t n_inputs, int_t n_outputs)
{
	ddq_op	res;
	int_t	size;

	size = type_size[type] + (sizeof(obj)+sizeof(void *))*(n_inputs+n_outputs+1) + sizeof(flag_t)*n_inputs + sizeof(void*);

	// res = malloc(size);
	res = ddq_malloc(_ring, size);
	memset(res, 0, size);
	res->uid = _ring->n_ops;
	res->n_threads = 1;
	res->max_threads = 1;
	res->pos = type;
	res->type = type;
	res->n_inputs = n_inputs;
	res->n_outputs = n_outputs;
	res->inputs = (obj *)((char *)res + type_size[type]);		// FIXME: 这里是否需要考虑对齐问题？成员足够大，一般不会有问题，但需要仔细确认一下。
	res->outputs = res->inputs + n_inputs;
	//p_inputs[-1]存储metadata指针
	res->p_f = NULL;
	res->p_inputs = (void **) (res->outputs + n_outputs) + 1;
	res->p_outputs = res->p_inputs + n_inputs;
	res->p_attributes = res->p_outputs + n_outputs;
	res->ver_inputs = (flag_t *) (res->p_attributes + 1);
	res->metadata = NULL;
	if (!_ring->ops)
	{
		res->next = res;
		_ring->ops = res;
	}
	else
	{
		res->next = _ring->ops->next;
		_ring->ops->next = res;
	}
	_ring->n_ops++;
	return	res;
}

ddq_op	ddq_spawn_with_meta(meta metadata, ddq_ring _ring, ddq_type_t type, int_t n_inputs, int_t n_outputs)
{
	ddq_op	res;

	res = ddq_spawn(_ring, type, n_inputs, n_outputs);
	
	res->metadata = meta_copy(metadata);

	return	res;
}

void ddq_add_f(ddq_op op, obj ob){
	if(!op || !ob){
		ddq_error("ddq_add_f: op or obj is NULL\n");
		return;
	}
	ob->n_ref++;
	op->f = ob;
}

void ddq_add_inputs(ddq_op op, int_t idx, obj ob){
	if(!op || !ob){
		ddq_error("ddq_add_inputs: op or obj is NULL\n");
		return;
	}
	if(idx >= op->n_inputs){
		ddq_error("ddq_add_inputs: index out of range");
		return;
	}
	ob->n_ref++;
	op->inputs[idx] = ob;
}

void ddq_add_outputs(ddq_op op, int_t idx, obj ob){
	if(!op || !ob){
		ddq_error("ddq_add_outputs: op or obj is NULL\n");
		return;
	}
	if(idx >= op->n_outputs){
		ddq_error("ddq_add_outputs: index out of range\n");
		return;
	}
	ob->n_ref++;
	op->outputs[idx] = ob;
}

void ddq_add_attributes(ddq_op op, int_t idx, obj ob){
	// if(!op || !ob){
	// 	ddq_error("ddq_add_attributes: op or obj is NULL\n");
	// 	return;
	// }
	// if(idx >= op->n_attributes){
	// 	ddq_error("ddq_add_attributes: index out of range\n");
	// 	return;
	// }
	// op->p_attributes[idx] = ob;
}

int_t get_type_size(int_t type){
	return type_size[type];
}

typedef struct OpList {
    ddq_op op;
    struct OpList* next;
} OpList;

void getNeighbors(ddq_op target, ddq_ring ring){
	OpList* neighbors = NULL, *tmp, *to_free;
	target->n_neighbors = 0;
	target->neighbors = NULL;
	int_t i,j,k;
	ddq_op op_ = target->next;
	do{
		for(j=0; j<target->n_inputs; j++){
			for(k=0; k<op_->n_outputs; k++){
				if(target->inputs[j] == op_->outputs[k]){
					target->n_neighbors++;
					if(!neighbors){
						neighbors = malloc(sizeof(OpList));
						tmp = neighbors;
					}else{
						tmp->next = malloc(sizeof(OpList));
						tmp = tmp->next;
					}
					tmp->op = op_;
					tmp->next = NULL;
					goto next_op_label;
				}
			}

		}
		for(j=0; j<target->n_outputs; j++){
			for(k=0; k<op_->n_inputs; k++){
				if(target->outputs[j] == op_->inputs[k]){
					target->n_neighbors++;
					if(!neighbors){
						neighbors = malloc(sizeof(OpList));
						tmp = neighbors;
					}else{
						tmp->next = malloc(sizeof(OpList));
						tmp = tmp->next;
					}
					tmp->op = op_;
					tmp->next = NULL;
					goto next_op_label;
				}
			}
		}
		next_op_label:
		op_ = op_->next;
	}while(op_ != target);

	if(neighbors){
		target->neighbors = ddq_malloc(ring, target->n_neighbors * sizeof(ddq_op));
		tmp = neighbors;
		for(i=0; i<target->n_neighbors; i++){
			target->neighbors[i] = tmp->op;
			to_free = tmp;
			tmp = tmp->next;
			free(to_free);
		}
		
	}
}

void ddq_update(ddq_ring ring){
	ddq_op op_ = ring->ops;
	do{
		getNeighbors(op_, ring);
		op_ = op_->next;
	}while(op_ != ring->ops);
	ring->status_ops = ddq_malloc(ring, sizeof(enum_op_status) * ring->n_ops);
	for(int i=0; i<ring->n_ops; i++){
		ring->status_ops[i] = op_status_ready;
	}
}