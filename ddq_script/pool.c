
#include	<stdint.h>
#include	<string.h>

#include	"pool.h"


#define	ast2pool(p)	((pool_node)((char *)p-(intptr_t)&(((pool_node)NULL)->n)))


ast	ast_new()
{
	static pool_node	pool = NULL;

	ast	res;

	res = (ast) malloc(sizeof(ast_t));
	res->pool = &pool;
	res->set = (set_node *) malloc(sizeof(set_node));
	*res->set = NULL;
	res->index_any = 0;

	return	res;
}

void	ast_delete(ast pool)
{
	set_node	i, tmp1;
	pool_node	j, tmp2;
	int		flag;

	HASH_ITER(hh, *pool->set, i, tmp1)
	{
		i->k->nref --;
		HASH_DELETE(hh, *pool->set, i);
		free(i);
	}

	for (flag=1; flag--;)
		HASH_ITER(hh, *pool->pool, j, tmp2)
			if (j->nref == 0)			// FIXME: 与ast_access()并发时有冲突，或者把gc单独放在别的地方？
			{
				if (ast_type_n1_start < j->n.t && j->n.t < ast_type_n1_end)
					if (j->n.v.n1)
						ast2pool(j->n.v.n1)->nref --;
				if (ast_type_n2_start < j->n.t && j->n.t < ast_type_n2_end)
				{
					if (j->n.v.n2[0])
						ast2pool(j->n.v.n2[0])->nref --;
					if (j->n.v.n2[1])
						ast2pool(j->n.v.n2[1])->nref --;
				}
				//if (j->n.t == ast_type_str) ddq_log("deleting \"%s\"\n", j->n.v.s);	// debug
				HASH_DELETE(hh, *pool->pool, j);
				free(j);
				flag = 1;
			}

	free(pool->set);
	free(pool);
}

ast	ast_copy(ast pool)
{
	ast		res;
	ast_node	i;

	res = ast_new();
	res->index_any = pool->index_any;
	ast_iter(pool, i)
		ast_insert_node(res, i);

	return	res;
}

static pool_node	ast_pool_insert(ast pool, ast_type type, ast_value value)
{
	pool_node	res, buf;
	int		lenext, i;

	if (type == ast_type_str && strlen(value.s0) + 1 > sizeof(ast_value))
		lenext = strlen(value.s0) + 1 - sizeof(ast_value);
	else
		lenext = 0;
	buf = (pool_node)malloc(sizeof(pool_node_t) + lenext);
	memset(buf, 0, sizeof(pool_node_t) + lenext);
	buf->nref = 0;
	buf->n.t = type;
	buf->n.v = value;
	buf->len = (intptr_t)&(((ast_node)NULL)->v);

	if (ast_type_n1_start < type && type < ast_type_n1_end)
	{
		buf->len += sizeof(buf->n.v.n1);
		if (value.n1)
			buf->n.v.n1 = &(ast_pool_insert(pool, value.n1->t, (value.n1->t==ast_type_str)?(ast_value){.s0=value.n1->v.s}:value.n1->v)->n);
	}
	else if (ast_type_n2_start < type && type < ast_type_n2_end)
	{
		buf->len += sizeof(buf->n.v.n2);
		if (value.n2[0])
			buf->n.v.n2[0] = &(ast_pool_insert(pool, value.n2[0]->t, (value.n2[0]->t==ast_type_str)?(ast_value){.s0=value.n2[0]->v.s}:value.n2[0]->v)->n);
		if (value.n2[1])
			buf->n.v.n2[1] = &(ast_pool_insert(pool, value.n2[1]->t, (value.n2[1]->t==ast_type_str)?(ast_value){.s0=value.n2[1]->v.s}:value.n2[1]->v)->n);
	}
	else switch (type)
	{
		case	ast_type_any :
		case	ast_type_int :
		case	ast_type_bool :
			buf->len += sizeof(buf->n.v.i);
			break;
		case	ast_type_real :
			buf->len += sizeof(buf->n.v.r);
			break;
		case	ast_type_ptr :
		case	ast_type_obj :
			buf->len += sizeof(buf->n.v.p);
			break;
		case	ast_type_fun :
			buf->len += sizeof(buf->n.v.f);
			break;
		case	ast_type_str :
			buf->len += strlen(value.s0) + 1;
			for (i=0; i<=strlen(value.s0); i++)	// FIXME: 这里如果使用strcpy，会被libc内置的越界检查报错？
				buf->n.v.s[i] = value.s0[i];
			break;
		default	:
			;		// FIXME: 这里应该报错
	}

	//if (type == ast_type_str) ddq_log("IN  (%p)\"%s\" (%p)\"%s\" #%d\n",&buf->n, (char*)&buf->n.v, buf->n.v.s, buf->n.v.s, buf->len);	// debug
	HASH_FIND(hh, *pool->pool, &buf->n, buf->len, res);

	if (res)
		free(buf);
	else
	{
		res = buf;
		HASH_ADD(hh, *pool->pool, n, buf->len, res);
		if (ast_type_n1_start < res->n.t && res->n.t < ast_type_n1_end)
			if (res->n.v.n1)
				ast2pool(res->n.v.n1)->nref ++;
		if (ast_type_n2_start < res->n.t && res->n.t < ast_type_n2_end)
		{
			if (res->n.v.n2[0])
				ast2pool(res->n.v.n2[0])->nref ++;
			if (res->n.v.n2[1])
				ast2pool(res->n.v.n2[1])->nref ++;
		}
	}
	//if (type == ast_type_str) ddq_log("OUT (%p)\"%s\" (%p)\"%s\" #%d\n",&res->n, (char*)&res->n.v, res->n.v.s, res->n.v.s, res->len);	// debug

	return res;
}

static void	ast_set_insert(set_node *set, pool_node p)
{
	set_node	res;

	HASH_FIND(hh, *set, &p, sizeof(pool_node), res);

	if (!res)
	{
		res = malloc(sizeof(set_node_t));
		res->k = p;
		res->deleted = 0;
		HASH_ADD(hh, *set, k, sizeof(pool_node), res);
		//printf("ast_set_insert (%d->%d) ", p->nref, p->nref+1); ast_print(&p->n); printf("\n"); 	// debug
		p->nref ++;
	}
	else if (res->deleted)
		res->deleted = 0;
}

typedef	enum
{
	ast_access_cache_type_push,
	ast_access_cache_type_pop,
	ast_access_cache_type_insert
} ast_access_cache_type;

typedef	struct ast_access_cache_stack_t	ast_access_cache_stack_t;
typedef	ast_access_cache_stack_t *	ast_access_cache_stack;
struct	ast_access_cache_stack_t
{
	ast	c;
	ast_access_cache_stack	prev;
};

static void	ast_access_cache(ast_access_cache_type type, pool_node node)
{
	static	ast_access_cache_stack	cache = NULL;
	ast_access_cache_stack	p;

	if (!cache)
	{
		cache = malloc(sizeof(ast_access_cache_stack_t));
		cache->c = ast_new();
		cache->prev = NULL;
	}

	switch (type)
	{
		case	ast_access_cache_type_push :
			p = malloc(sizeof(ast_access_cache_stack_t));
			p->c = ast_new();
			p->prev = cache;
			cache = p;
			break;
		case	ast_access_cache_type_pop :
			ast_delete(cache->c);
			p = cache->prev;
			free(cache);
			cache = p;
			break;
		case	ast_access_cache_type_insert :
			ast_set_insert(cache->c->set, node);
			break;
	}
}

void	ast_access_push()
{
	ast_access_cache(ast_access_cache_type_push, NULL);
}

void	ast_access_pop()
{
	ast_access_cache(ast_access_cache_type_pop, NULL);
}

ast_node	ast_access(ast pool, ast_type type, ast_value value)
{
	pool_node	p;

	if (type == ast_type_any && value.i == -1)
		value.i = ++ pool->index_any;		// FIXME: 要考虑溢出问题吗？

	p = ast_pool_insert(pool, type, value);
	ast_access_cache(ast_access_cache_type_insert, p);

	return	&p->n;
}

ast_node	ast_insert(ast pool, ast_type type, ast_value value)
{
	ast_node	res;

	res = ast_access(pool, type, value);
	ast_set_insert(pool->set, ast2pool(res));

	return	res;
}

void		ast_insert_node(ast pool, ast_node node)
{
	pool_node	res;

	HASH_FIND(hh, *pool->pool, node, ast2pool(node)->len, res);

	if (!res)
		ast_insert(pool, node->t, node->v);
	else
		ast_set_insert(pool->set, res);
}

int		ast_is_in(ast pool, ast_node node)
{
	ast_node	i;

	ast_iter(pool, i)
		if (i == node)
			return	1;

	return	0;
}

void		ast_remove(ast pool, ast_node p)
{
	set_node	i;

	ast_set_iter(pool, i)
		if (&(i->k->n) == p)
			i->deleted = 1;
}

static ast_node	ast_pool_subst(ast pool, ast_node node, ast_node find, ast_node replace)
{
	ast_value	buf;

	if (node == find)
		return	replace;

	if (ast_type_n1_start < node->t && node->t < ast_type_n1_end)
		if (node->v.n1)
		{
			buf.n1 = ast_pool_subst(pool, node->v.n1, find, replace);
			if (node->v.n1 != buf.n1)
				return	ast_access(pool, node->t, buf);
		}
	if (ast_type_n2_start < node->t && node->t < ast_type_n2_end)
	{
		buf.n2[0] = node->v.n2[0] ? ast_pool_subst(pool, node->v.n2[0], find, replace) : NULL;
		buf.n2[1] = node->v.n2[1] ? ast_pool_subst(pool, node->v.n2[1], find, replace) : NULL;
		if (buf.n2[0] != node->v.n2[0] || buf.n2[1] != node->v.n2[1])
			return	ast_access(pool, node->t, buf);
	}

	return	node;
}

int		ast_subst(ast pool, ast_node find, ast_node replace)		// FIXME: 这个函数需要仔细检查正确性，包括递归替换等情况
{
	set_node	i;
	ast_node	j;
	int		res;

	res = 0;
	ast_set_iter(pool, i)
	{
		j = ast_pool_subst(pool, &(i->k->n), find, replace);
		if (&(i->k->n) != j)
		{
			i->deleted = 1;
			ast_set_insert(pool->set, ast2pool(j));
			res = 1;
		}
	}

	return	res;
}

static	void	ast_collect_type_find(ast res, ast_node node, ast_type type)
{
	if (type == node->t)
		ast_insert_node(res, node);

	if (ast_type_n1_start < node->t && node->t < ast_type_n1_end)
		if (node->v.n1)
			ast_collect_type_find(res, node->v.n1, type);
	if (ast_type_n2_start < node->t && node->t < ast_type_n2_end)
	{
		if (node->v.n2[0])
			ast_collect_type_find(res, node->v.n2[0], type);
		if (node->v.n2[1])
			ast_collect_type_find(res, node->v.n2[1], type);
	}
}

ast		ast_collect_type(ast pool, ast_type type)
{
	ast		res;
	ast_node	i;

	res = ast_new();
	ast_iter(pool, i)
		ast_collect_type_find(res, i, type);

	return	res;
}

static	void	ast_collect_n2_find(ast res, ast_node node, ast_type type, ast_node left, ast_node right)
{
	if (type == node->t && (!left || left == node->v.n2[0]) && (!right || right == node->v.n2[1]))
		ast_insert_node(res, node);

	if (ast_type_n1_start < node->t && node->t < ast_type_n1_end)
		if (node->v.n1)
			ast_collect_n2_find(res, node->v.n1, type, left, right);
	if (ast_type_n2_start < node->t && node->t < ast_type_n2_end)
	{
		if (node->v.n2[0])
			ast_collect_n2_find(res, node->v.n2[0], type, left, right);
		if (node->v.n2[1])
			ast_collect_n2_find(res, node->v.n2[1], type, left, right);
	}
}

ast		ast_collect_n2(ast pool, ast_type type, ast_node left, ast_node right)
{
	ast		res;
	ast_node	i;

	res = ast_new();
	ast_iter(pool, i)
		ast_collect_n2_find(res, i, type, left, right);

	return	res;
}

void		ast_map_set(ast pool, ast_node key, ast_node value)
{
	ast_node	i;

	if (key && value)
		ast_insert(pool, ast_type_map, (ast_value){.n2={key, value}});	// FIXME: 目前没有判断冲突，是否需要保证key的唯一？
}

ast_node	ast_map_get(ast pool, ast_node key)
{
	ast_node	i;

	ast_iter(pool, i)
		if (i->v.n2[0] == key)
			return	i->v.n2[1];

	return	NULL;
}


