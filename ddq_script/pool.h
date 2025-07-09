#ifndef F_POOL_H
#define	F_POOL_H

#include	"uthash.h"

#include	"ast.h"


typedef	struct pool_node_t	pool_node_t;
typedef	pool_node_t *		pool_node;
struct	pool_node_t
{
	UT_hash_handle	hh;
	int		len;
	int		nref;
	ast_node_t	n;		// NOTE: 把n放在最后，对于字符串类型需要扩展空间
};

typedef	struct set_node_t	set_node_t;
typedef	set_node_t *		set_node;
struct	set_node_t
{
	UT_hash_handle	hh;
	int		deleted;
	pool_node	k;
};

struct	ast_t
{
	pool_node *	pool;
	set_node *	set;
	int		index_any;
};


ast		ast_new();
void		ast_delete(ast pool);

ast		ast_copy(ast pool);

void		ast_access_push();	// NOTE: 这两个函数是可选的，用于管理ast_access产生的临时节点
void		ast_access_pop();
ast_node	ast_access(ast pool, ast_type type, ast_value value);

ast_node	ast_insert(ast pool, ast_type type, ast_value value);
void		ast_insert_node(ast pool, ast_node node);

int		ast_is_in(ast pool, ast_node node);

void		ast_remove(ast pool, ast_node p);

int		ast_subst(ast pool, ast_node find, ast_node replace);		// NOTE: 有替换返回1，无替换返回0

ast		ast_collect_type(ast pool, ast_type type);
ast		ast_collect_n2(ast pool, ast_type type, ast_node left, ast_node right);			// NOTE: 当left或right为NULL时为不做相关限制，用户使用后需要用ast_delete析构返回值

void		ast_map_set(ast pool, ast_node key, ast_node value);
ast_node	ast_map_get(ast pool, ast_node key);


#define		ast_set_iter(pl, i)	for (i=*((ast)(pl))->set; i; i=i->hh.next) if (!i->deleted)		// NOTE: 注意在else前使用这些宏的时候，一定要用大括号包上！
#define		ast_iter(pl, i)	for (set_node this_loop_=*((ast)(pl))->set; this_loop_; this_loop_=this_loop_->hh.next) if (!this_loop_->deleted) if (i = &(this_loop_->k->n))
//#define		ast_set_iter(pl, i)	for (struct {int again; set_node tmp;} thisloop = {1, NULL}; thisloop.again--;) HASH_ITER(hh, *pl->set, i, thisloop.tmp) if (!i->deleted)	// NOTE: 循环中可以使用thisloop.again=1来实现再次循环
//#define		ast_iter(pl, i)	for (struct {int again; set_node i; set_node tmp;} thisloop = {1, NULL, NULL}; thisloop.again--;) HASH_ITER(hh, *pl->set, thisloop.i, thisloop.tmp) if (!thisloop.i->deleted) if (i = &(thisloop.i->k->n))	// NOTE: 循环中可以使用thisloop.again=1来实现再次循环


#endif
