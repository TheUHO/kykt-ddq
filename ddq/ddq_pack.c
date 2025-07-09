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

//ddq_pack && ddq_unpack
typedef struct obj_node_t {
    void *ptr;
    struct obj_node_t *next;
} obj_node_t;
typedef obj_node_t* obj_node;

static inline int_t get_meta_size(meta metadata){
	int_t size = 0;
	meta m = metadata;
	while(m){
		size += aligned8(sizeof(meta_t)) + aligned8(strlen(m->name) + 1);	// FIXME: 这里m->name似乎是空指针
		if(m->type == meta_type_string)
			size += aligned8(strlen(m->value.sval) + 1);
		m = m->next;
	}
	return size;
}

obj_mem pack_meta(meta metadata){
	if(!metadata) return NULL;
	int_t size = sizeof(obj_mem_t) + get_meta_size(metadata);
	//分配空间
    void *ptr = malloc(size);
	obj_mem res = (obj_mem)ptr;
	res->type = obj_mem_default;
    res->p = (void*)sizeof(obj_mem_t);
	res->size = size;
	//拷贝metadata
	meta m = metadata;
	ptr = (void*)((char*)ptr + sizeof(obj_mem_t));
	meta tmp;
	while(m){
		memcpy(ptr, m, sizeof(meta_t));
		tmp = (meta)ptr; 
		ptr = (void*)((char*)ptr + aligned8(sizeof(meta_t)));
		memcpy(ptr, m->name, strlen(m->name) + 1);
		tmp->name = (char*)((char*)ptr - (char*)res);
		ptr = (void*)((char*)ptr + aligned8(strlen(m->name) + 1));
		if(m->type == meta_type_string){
			memcpy(ptr, m->value.sval, strlen(m->value.sval) + 1);
			tmp->value.sval = (char*)((char*)ptr - (char*)res);
			ptr = (void*)((char*)ptr + aligned8(strlen(m->value.sval) + 1));
		}
		tmp->next = (m->next) ? (meta)((char*)ptr - (char*)res) : 0;
		m = m->next;
	}
	return res;
}

meta unpack_meta(obj_mem mem){
	if(!mem) return NULL;
	mem->p = (void*)((char*)mem + (int_t)mem->p);
	meta res = NULL, new_meta = NULL, prev = NULL;
	meta m = (meta)(mem->p);
	unsigned int i;
	do{
		m->name = (char*)mem + (int_t)m->name;
		if(m->type == meta_type_string)
			m->value.sval = (char*)mem + (int_t)m->value.sval;
		m->next = (m->next) ? (meta)((char*)mem + (int_t)m->next) : NULL;

		new_meta = meta_new(m->name, m->type, (m->type == meta_type_string) ? (void*)m->value.sval : (void*)&m->value);
		new_meta->next = res;
		res = new_meta;
		
		m = m->next;
	}while(m);

	return res;
}


static inline int_t get_op_size(ddq_op op){
	return get_type_size(op->type)
			+ (sizeof(obj) + sizeof(void *)) * (op->n_inputs + op->n_outputs)
			+ sizeof(flag_t)*op->n_inputs + sizeof(void*);
}


static inline int_t find_obj_idx(obj_node obj_list, void *p){
    obj_node o = obj_list;
    int idx = 0;
    while(o != NULL){
        if(o->ptr == p)
            return idx;
        o = o->next;
        idx++;
    }
    return -1;
}

static inline int get_obj_count(obj_node obj_list){
    obj_node o = obj_list;
    int count = 0;
    while(o != NULL){
        o = o->next;
        count++;
    }
    return count;
}

static inline void add_obj(obj_node *obj_list, void *p){
    if(find_obj_idx(*obj_list, p) < 0){
        obj_node node = malloc(sizeof(obj_node_t));
        node->ptr = p;
        node->next = *obj_list;
        *obj_list = node;
    }
}

obj_node get_obj_list(ddq_ring ring){
	obj_node obj_list = NULL;
	ddq_op op = ring->ops;
	int i;
    do{ 
		add_obj(&obj_list, op->f);
		for(i=0; i<op->n_inputs; i++){
			add_obj(&obj_list, op->inputs[i]);
		}
		for(i=0; i<op->n_outputs; i++){
			add_obj(&obj_list, op->outputs[i]);
		}
        op = op->next;
    }while(op != ring->ops);
	return obj_list;
}

static inline int_t get_ops_size(ddq_op ops){
	int_t size = 0;
    ddq_op op = ops;
    do{ 
		size += aligned8(get_op_size(op));
		if(op->metadata)
			size += (sizeof(obj_mem_t) + get_meta_size(op->metadata));
        op = op->next;
    }while(op != ops);
	return size;
}

static inline int_t get_ring_size(ddq_ring ring){
	int_t size = 0;
	int i;
    size = sizeof(ddq_ring_t) + (ring->n_inputs + ring->n_outputs) * sizeof(obj);
	if(ring->metadatas){
		size += (ring->n_ops * sizeof(meta));
		for(i=0; i<ring->n_ops; i++){
			if(ring->metadatas[i])
				size += (sizeof(obj_mem_t) + get_meta_size(ring->metadatas[i]));
		}
	}
	return size;
}

static inline int_t get_obj_list_size(obj_node obj_list){
	int_t size = aligned8(sizeof(obj_t)) * get_obj_count(obj_list);
	obj_node o = obj_list;
	while(o){
		if(((obj)o->ptr)->metadata){
			size += (sizeof(obj_mem_t) + get_meta_size(((obj)o->ptr)->metadata));
		}
		o = o->next;
	}
	return size;
}

static inline int_t get_total_size(ddq_ring ring, obj_node obj_list){
	int_t size = 0;
    size = get_ring_size(ring);
	size += get_ops_size(ring->ops);
    size += get_obj_list_size(obj_list);
	return size;
}

int is_contained(ddq_ring ring, void* ptr){
	int_t offset = (char*)ptr - (char*)ring;
	if(offset >= 0 && offset < ring->mem->bufsize){
		return 1;
	}
	return 0;
}

void ser_obj(void* ptr, obj ob, ddq_ring ring){
	obj new_ob = (obj)ptr;
	if(!(new_ob->status & obj_status_packed)){
		new_ob->dup = is_contained(ring, ob->dup) ? (int_t)((char*)ob->dup - (char*)ring) : ob->dup;
		new_ob->status = new_ob->status | obj_status_packed; 

		if(new_ob->prop & obj_prop_buffer){
			buffer_obj new_buf = new_ob;
			buffer_obj buf = ob;
			new_buf->element_status = (enum_obj_status*)((char*)buf->element_status - (char*)ob);
		}
	}
}

void ser_ops(void* ptr, ddq_ring ring){
	ddq_op ops = ring->ops;
	ddq_op op = ops;
	ddq_op new_op = (ddq_op) ptr;
	do{
		//modify f
		new_op -> f = (obj)((char*)op->f - (char*)ops);
		ser_obj((int_t)new_op->f + ptr, op->f, ring);

		//modify inputs
		int i;
		new_op->inputs = (obj*)((char*)op->inputs - (char*)ops + (char*)ptr);
		for(i=0; i<op->n_inputs; i++){
			new_op -> inputs[i] = (obj)((char*)op -> inputs[i] - (char*)ops);
			ser_obj((int_t)new_op->inputs[i] + ptr, op -> inputs[i], ring);
		}
		new_op->outputs = (obj*)((char*)op->outputs - (char*)ops + (char*)ptr);
		for(i=0; i<op->n_outputs; i++){
			new_op -> outputs[i] = (obj)((char*)op -> outputs[i] - (char*)ops);
			ser_obj((int_t)new_op->outputs[i] + ptr, op -> outputs[i], ring);
		}
		new_op->neighbors = (ddq_op*)((char*)op->neighbors - (char*)ops + (char*)ptr);
		for(i=0; i<op->n_neighbors; i++){
			new_op -> neighbors[i] = (ddq_op)((char*)op -> neighbors[i] - (char*)ops);
		}

		new_op->inputs = (obj*)((char*)op->inputs - (char*)ops);
		new_op->outputs = (obj*)((char*)op->outputs - (char*)ops);
		new_op->neighbors = (ddq_op*)((char*)op->neighbors - (char*)ops);

		new_op->ver_inputs = (flag_t*)((char*)op->ver_inputs - (char*)ops);
		new_op->p_inputs = (void**)((char*)op->p_inputs - (char*)ops);
		new_op->p_outputs = (void**)((char*)op->p_outputs - (char*)ops);

		new_op->next = (ddq_op)((char*)op->next - (char*)ops);

		new_op = (int_t) new_op->next + ptr;
		op = op->next;
    }while(op != ops);
}

void ser_ring(void* ptr, ddq_ring ring){
	ddq_ring new_ring = (ddq_ring) ptr;
	int i;
	if(ring->inputs)
		new_ring->inputs = (obj*)((char*)ring->inputs - (char*)ring + (char*)ptr);
	if(ring->outputs)
		new_ring->outputs = (obj*)((char*)ring->outputs - (char*)ring + (char*)ptr);
	for(i=0; i<ring->n_inputs; i++){
		new_ring->inputs[i] = (obj)((char*)ring->inputs[i] - (char*)ring);
		ser_obj((int_t)new_ring->inputs[i] + ptr, ring->inputs[i], ring);
	}
	for(i=0; i<ring->n_outputs; i++){
		new_ring->outputs[i] = (obj)((char*)ring->outputs[i] - (char*)ring);
		ser_obj((int_t)new_ring->outputs[i] + ptr, ring->outputs[i], ring);
	}
	if(ring->inputs)
		new_ring->inputs = (obj*)((char*)ring->inputs - (char*)ring);
	if(ring->outputs)
		new_ring->outputs = (obj*)((char*)ring->outputs - (char*)ring);
	// if(ring->metadatas)
	// 	new_ring->metadatas = (obj*)((char*)ring->metadatas - (char*)ring);
	new_ring->mem = (obj_mem)(0);
	new_ring->ops = (ddq_op)((char*)ring->ops - (char*)ring);
	new_ring->status_ops = (enum_op_status*)((char*)ring->status_ops - (char*)ring);
	ser_ops((int_t)new_ring->ops + ptr, ring);
}

void* pack_ring(ddq_ring ring){
	if(!ring){
		return NULL;
	}

	if(ring->mem->p != ring){
		ddq_error("can not pack sub_ring %p, but ring->mem->p: %p\n", ring, ring->mem->p);
	}

	obj_mem res = malloc(ring->mem->bufsize + sizeof(obj_mem_t));
	memcpy(res, ring->mem, ring->mem->bufsize + sizeof(obj_mem_t));
	res->size = ring->mem->bufsize;
	res->bufsize = res->size;
	res->p = (void*)sizeof(obj_mem_t);

	ser_ring((int_t)res->p + (void*)res, ring);

	return res;
}

void deser_obj(void* ptr, ddq_ring ring){
	obj ob = (obj)ptr;
	if(ob->status & obj_status_packed){
		if((int_t)ob->dup > 0 && (int_t)ob->dup < ring->mem->bufsize)
			ob->dup = (obj)((int_t)ob->dup + (char*)ring);
		ob->status = ob->status & ~obj_status_packed;

		if(ob->prop & obj_prop_buffer){
			buffer_obj buf = ob;
			buf->element_status = (enum_obj_status*)((int_t)buf->element_status + (char*)ob);
		}
	}
}

void deser_ops(void* ptr, ddq_ring ring){
	ddq_op op = (ddq_op) ptr;
	do{
		//modify f
		op -> f = (obj)((int_t)op->f + (char*)ptr);
		deser_obj(op->f, ring);

		op->inputs = (obj*)((int_t)op->inputs + (char*)ptr);
		op->outputs = (obj*)((int_t)op->outputs + (char*)ptr);
		op->neighbors = (obj*)((int_t)op->neighbors + (char*)ptr);
		op->ver_inputs = (flag_t*)((int_t)op->ver_inputs + (char*)ptr);
		op->p_inputs = (void**)((int_t)op->p_inputs + (char*)ptr);
		op->p_outputs = (void**)((int_t)op->p_outputs + (char*)ptr);
		//modify inputs
		int i;
		for(i=0; i<op->n_inputs; i++){
			op->inputs[i] = (obj)((int_t)op->inputs[i] + (char*)ptr);
			deser_obj(op->inputs[i], ring);
		}
		for(i=0; i<op->n_outputs; i++){
			op->outputs[i] = (obj)((int_t)op -> outputs[i] + (char*)ptr);
			deser_obj(op->outputs[i], ring);
		}
		for(i=0; i<op->n_neighbors; i++){
			op->neighbors[i] = (obj)((int_t)op -> neighbors[i] + (char*)ptr);
		}
		op->next = (ddq_op)((int_t)op->next + (char*)ptr);

		op = op->next;
    }while(op != ptr);
}

void deser_ring(void* ptr){
	if(!ptr) return;
	ddq_ring ring = (ddq_ring) ptr;
	ring->inputs = (obj*)((int_t)ring->inputs + (char*)ptr);
	int i;
	for(i=0; i<ring->n_inputs; i++){
		ring->inputs[i] = (obj)((int_t)ring->inputs[i] + (char*)ptr);
		deser_obj(ring->inputs[i], ring);
	}
	ring->outputs = (obj*)((int_t)ring->outputs + (char*)ptr);
	for(i=0; i<ring->n_outputs; i++){
		ring->outputs[i] = (obj)((int_t)ring->outputs[i] + (char*)ptr);
		deser_obj(ring->outputs[i], ring);
	}
	// if(ring->metadatas)
	// 	new_ring->metadatas = (obj*)(ring->metadatas - ring);
	// new_ring->mem = (obj_mem)(0);
	ring->ops = (ddq_op)((int_t)ring->ops + (char*)ptr);
	ring->status_ops = (enum_op_status*)((int_t)ring->status_ops + (char*)ptr);
	deser_ops(ring->ops, ring);
}

ddq_ring unpack_ring(void *ptr){
    if(!ptr) return NULL;
    obj_mem mem = (obj_mem) ptr;
	mem->p = (char*)ptr + (int_t)mem->p;
	ddq_ring ring = mem->p;
	ring->mem = mem;
	deser_ring(mem->p);
	return ring;
}