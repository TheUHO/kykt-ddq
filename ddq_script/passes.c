
#include	<string.h>

#include	"passes.h"
#include	"oplib.h"


/*
 * 1. 功能辅助函数：
 * 	1. 缓存操作：用于load文件
 * 	2. 集合操作：用于查询、筛选、合并节点集合
 * 2. 编译辅助函数：
 * 	1. 清理操作：编译过程中的一些清理工作
 * 	2. 装载操作：把嵌套的装载文件合并到一起
 * 	3. 发射操作：编译为ddq的对象和算子
 * 3. 编译主程序：
 *
 */

/*
 * 	0. 调试工具：
 */

void	ast_pool_print(ast pool)
{
	ast_node	i;
	printf("\nast_pool_print :\n");
	ast_iter(pool, i)
	{
		ast_print(i);
		printf("\n");
	}
	printf("end of ast_pool_print.\n\n");
}


/*
 *	1.1 缓存操作：
 */

static	ast	cache = NULL;

static	void *	cache_get(ast_node key)
{
	ast_node	res;

	if (!cache)
		return	NULL;

	res = ast_map_get(cache, key);

	if (res && res->t == ast_type_ptr)
		return	res->v.p;
	else
		return	NULL;
}

static	void	cache_set(ast_node key, void *value)
{
	if (!cache)
		cache = ast_new();

	ast_map_set(cache, key, ast_access(cache, ast_type_ptr, (ast_value){.p=value}));
}

static	void	cache_load(ast pool)
{
	ast		buf;
	void *		p;
	ast_node	i, j;

	buf = ast_collect_type(pool, ast_type_load_ast);
	ast_iter(buf, i)
		if (!cache_get(i))
		{
			p = load_ast_file(i->v.n1->v.s);
			cache_set(i, p);	// NOTE: map: ast_type_load_ast -> ast
			cache_load((ast)p);
		}
	ast_delete(buf);

	buf = ast_collect_type(pool, ast_type_defcall);
	ast_iter(buf, i)
	{
		if (i->v.n2[1]->t == ast_type_load_op) // 非load_op的defcall会被单独处理
			if (!cache_get(i))
				cache_set(i, cache_get(i->v.n2[1]->v.n2[0])); // NOTE: map: ast_type_defcall -> ast, pass_defcall后 map: ast_type_defcall -> packed_ring
	}
	ast_delete(buf);

	buf = ast_collect_type(pool, ast_type_load_builtin);
	ast_iter(buf, i)
		if (!cache_get(i))
			cache_set(i, load_builtin(i->v.n1->v.s));	// NOTE: map: ast_type_load_builtin -> void *
	ast_delete(buf);

#ifdef	oplib_load_so
	buf = ast_collect_type(pool, ast_type_load_so_file);
	ast_iter(buf, i)
		if (!cache_get(i))
			cache_set(i, load_so_open(i->v.n1->v.s));	// NOTE: map: ast_type_load_so_file -> void * (handle returned from dlopen())
	ast_delete(buf);

	buf = ast_collect_type(pool, ast_type_load_so);
	ast_iter(buf, i)
		if (!cache_get(i))
			cache_set(i, load_so_sym(cache_get(i->v.n2[0]), i->v.n2[1]->v.s));	// NOTE: map: ast_type_load_so -> void * (returned from dlsym(), mostly pointers to functions)
	ast_delete(buf);
#endif

	// TODO: 其他load节点
	// ......
}

void		cache_clear()	// NOTE: 从主程序里调用这个函数
{
	ast_node	i;

	ast_iter(cache, i)
		switch (i->v.n2[0]->t)
		{
			case	ast_type_defcall :
				// TODO: free i->v.n2[1]->v.p as packed_ring
				;
				break;
			case	ast_type_load_ast :
				ast_delete(i->v.n2[1]->v.p);
				break;
			case	ast_type_load_builtin :
				break;
			case	ast_type_load_so_file :
				load_so_close(i->v.n2[1]->v.p);
				break;
			case	ast_type_load_so :
				break;
			default	:
				break;
		}

	ast_delete(cache);
}


/*
 *	1.2 集合操作：
 */

static	ast	ast_find_eq(ast pool, ast_node n)
{
	ast		res, nm, pr;
	ast_node	i, j, k, l;

	res = ast_new();
	ast_insert_node(res, n);

	ast_iter(res, i)
		if (ast_gtype(i) == ast_group_type_lvalue)	// 只有lvalue才能传递相等关系
		{
			ast_iter(pool, j)
			{
				if (j->t == ast_type_eq && j->v.n2[0] == i)
					ast_insert_node(res, j->v.n2[1]);
				if (j->t == ast_type_eq && j->v.n2[1] == i)
					ast_insert_node(res, j->v.n2[0]);
			}
			if (i->t == ast_type_prop)		// FIXME: 确认属性传导是这样的吗？
			{
				nm = ast_find_eq(pool, i->v.n2[0]);
				pr = ast_find_eq(pool, i->v.n2[1]);
				ast_iter(nm, k)
					ast_iter(pr, l)
						ast_insert(res, ast_type_prop, (ast_value){.n2={k, l}});
				ast_delete(pr);
				ast_delete(nm);
			}
		}

	return	res;
}

// 把节点按相等关系归并到多个ast中，并把这些ast以ast_type_ptr形式放入返回的ast中
static	ast	ast_group_eq(ast pool, ast nodes)
{
	ast		res, res0, t;
	ast_node	i, j, k;

	res = ast_new();

	ast_iter(nodes, i)
		if (ast_gtype(i) == ast_group_type_lvalue)
		{
			t = NULL;
			ast_iter(res, j)
				if (ast_is_in(j->v.p, i))
				{
					t = j->v.p;
					break;
				}
			if (!t)
				ast_insert(res, ast_type_ptr, (ast_value){.p=ast_find_eq(pool, i)});
		}
	ast_iter(nodes, i)
		if (ast_gtype(i) != ast_group_type_lvalue)
		{
			t = NULL;
			ast_iter(res, j)
				if (ast_is_in(j->v.p, i))
				{
					t = j->v.p;
					break;
				}
			if (!t)
				ast_insert(res, ast_type_ptr, (ast_value){.p=ast_find_eq(pool, i)});
		}

	/* FIXME: 是否需要如此操作？
	// 最后用rvalue把相关的组合并起来，使得每个组的元素各不同
	res0 = res;
	res = ast_new();
	ast_iter(res0, i)
	{
		t = NULL;
		ast_iter(res, j)
			if (ast_dist(i->v.p) == ast_dist(j->v.p))
			{
				t = j->v.p;
				break;
			}
		if (t)
		{
			ast_iter(i, k)
				ast_insert_node(t, k);
		}
		else
			ast_insert_node(res, i);
	}
	ast_delete(res0);
	*/

	return	res;
}

static	void	ast_group_delete(ast g)
{
	ast_node	i;

	ast_iter(g, i)
		ast_delete(i->v.p);
	ast_delete(g);
}

// (在相等关系的组里)找出唯一一个rvalue
static	ast_node	ast_dist(ast pool)
{
	ast_node	res = NULL;
	ast_node	i;

	ast_iter(pool, i)
	{
		if (ast_gtype(i) == ast_group_type_rvalue)
		{
			if (res && res != i)
				ddq_error("Conflict values in equations.\n");
			res = i;
		}
		else if (ast_gtype(i) != ast_group_type_lvalue)
		{
			ddq_error("Wrong value in equations.\n");
		}
	}

	return	res;
}

static	ast	ast_get_props(ast pool, ast nodes)
{
	ast		res;
	ast		ps, psg, ns;
	ast		gt;
	ast_node	i, j, k, p, v, vt;

	ast_access_push();

	ps = ast_new();
	ast_iter(pool, i)
		if (i->t == ast_type_eq)
		{
			if (i->v.n2[0]->t == ast_type_prop && ast_is_in(nodes, i->v.n2[0]->v.n2[0]))
				ast_insert_node(ps, i->v.n2[0]->v.n2[1]);
			if (i->v.n2[1]->t == ast_type_prop && ast_is_in(nodes, i->v.n2[1]->v.n2[0]))
				ast_insert_node(ps, i->v.n2[1]->v.n2[1]);
		}
	ast_iter(ps, i)
	{
		ns = ast_find_eq(pool, i);
		ast_iter(ns, j)
			ast_insert_node(ps, j);
		ast_delete(ns);
	}
	psg = ast_group_eq(pool, ps);
	ast_delete(ps);

	res = ast_new();

	ast_iter(nodes, i)
		ast_iter(psg, j)
		{
			p = ast_dist(j->v.p);
			if (!p)
				ddq_warning("Missing parameter name.\n");
			else
			{
				ast_iter(j->v.p, k)
				{
					gt = ast_find_eq(pool, ast_access(pool, ast_type_prop, (ast_value){.n2={i, k}}));
					v = ast_dist(gt);
					ast_delete(gt);
					if (v)
					{
						vt = ast_map_get(res, p);
						if (vt && v != vt)
							ddq_error("Conflict values for one parameter.\n");
						if (!vt)
							ast_map_set(res, p, v);
					}
				}
			}
		}
	ast_group_delete(psg);

	ast_access_pop();

	return	res;
}

static	ast	ast_get_prop_names(ast pool, ast_node n)
{
	ast		res, ns;
	ast_node	i, j;

	res = ast_new();

	ns = ast_find_eq(pool, n);
	ast_iter(pool, i)
		if (i->t == ast_type_eq)
		{
			if (i->v.n2[0]->t == ast_type_prop && ast_is_in(ns, i->v.n2[0]->v.n2[0]))
				ast_insert_node(res, i->v.n2[0]->v.n2[1]);
			if (i->v.n2[1]->t == ast_type_prop && ast_is_in(ns, i->v.n2[1]->v.n2[0]))
				ast_insert_node(res, i->v.n2[1]->v.n2[1]);
		}
	ast_delete(ns);

	ast_iter(res, i)
	{
		ns = ast_find_eq(pool, i);
		ast_iter(ns, j)
			ast_insert_node(res, j);
		ast_delete(ns);
	}

	return	res;
}

static	void	ast_merge(ast pool, ast inc)
{
	ast		buf, anys;
	ast_node	i;

	buf = ast_copy(inc);
	anys = ast_collect_type(buf, ast_type_any);
	if (pool->index_any < buf->index_any)
		pool->index_any = buf->index_any;
	ast_iter(anys, i)
		ast_subst(buf, i, ast_access(pool, ast_type_any, (ast_value){.i=-1}));
	ast_iter(buf, i)
		ast_insert_node(pool, i);
	ast_delete(anys);
	ast_delete(buf);
}


/*
 * 	2.1 清理操作：
 */

static	void	pass_clear_name(ast pool)
{
	ast		buf;
	ast_node	i;

	buf = ast_collect_type(pool, ast_type_name);
	ast_iter(buf, i)
		ast_subst(pool, i, ast_access(pool, ast_type_any, (ast_value){.i=-1}));
	ast_delete(buf);
}


/*
 * 	2.2 装载操作：
 */

static	void	pass_defcall(ast pool)
{
	ast			eqs, props;
	ast_node	nodel, booltrue, proc, call, anchor, father;
	void *		ring;
	ast_node	i, j, k, l, m;
	ast			load_pool;
	ast			defcalls, fathers, anchors, ops, obs, builtins, attributes;

	nodel = ast_access(pool, ast_type_str, (ast_value){.s0="nodel"});
	booltrue = ast_access(pool, ast_type_bool, (ast_value){.i=1});
	proc = ast_access(pool, ast_type_str, (ast_value){.s0="processor"});
	call = ast_access(pool, ast_type_str, (ast_value){.s0="call"});

	defcalls = ast_new();
	ast_iter(pool, i) // 拿到所有的defcalls
		if (i->t == ast_type_defcall)
			ast_insert_node(defcalls, i);

	ast_iter(defcalls, i)
	{
		if (i->v.n2[1]->t == ast_type_name) // 预处理g:=f，这里处理继承的属性
		{
			fathers = ast_new(); // 存放ast_type_name
			father = i->v.n2[1];
			ast_insert_node(fathers, father);
			int flag = 1;
			while (flag) {
				flag--;
				ast_iter(defcalls, j)
					if (j->v.n2[0] == father) {
						if (j->v.n2[1]->t == ast_type_name) { // 是否存在f:=ast_type_name
							father = j->v.n2[1];
							ast_insert_node(fathers, father);
							flag++;
							break;
						}
						else {
							father = j;
						}
					}
			}

			// 此时fathers里存放了节点i的所有父亲节点的name，找出他们的属性
			// 此时father为i节点的最高级父亲节点
			ast_iter(fathers, l)
			{
				attributes = ast_collect_n2(pool, ast_type_prop, l, NULL); // 找到所有的属性
				ast_iter(attributes, j)
					ast_iter(pool, k)
						if (k->t == ast_type_eq && k->v.n2[0] == j)
							ast_insert(pool, ast_type_eq, (ast_value){.n2={ast_access(pool, ast_type_prop, (ast_value){.n2={i->v.n2[0], k->v.n2[0]->v.n2[1]}}), k->v.n2[1]}});
				ast_delete(attributes);
			}

			// 给每个有父节点的defcall复制一个load_op的ast
			cache_set(i, ast_copy(cache_get(father)));
			
			ast_delete(fathers);
		}
	}
	ast_delete(defcalls);

	ast_iter(pool, i) 
		if (i->t == ast_type_defcall)	// NOTE: 格式是固定的！
		{
			load_pool = ast_copy(cache_get(i));
			if (!load_pool) ddq_error("defcall load error.\n");
			
			pass_load_ast(load_pool);
			builtins = ast_collect_type(load_pool, ast_type_load_builtin);
			eqs = ast_find_eq(pool, i->v.n2[0]); // f := load_op，这里寻找与f构成eq关系的结点

			ring = pack_ring(ast2ring(ast_copy(cache_get(i)))); // 这里如果不copy一份会导致cache_clear出错
			cache_set(i, ring); // TODO: 每个defcall对应一个packed_ring, 这个packed_ring最后也需要free

			anchors = ast_new();
			ast_iter(eqs, j)
				if (j->t == ast_type_anchor)  // 寻找f, f可能多次出现, 有多组输入输出
				{
					anchor = ast_access(pool, ast_type_any, (ast_value){.i=-1});
					ast_insert(anchors, ast_type_any, anchor->v);
					ast_insert(pool, ast_type_eq, (ast_value){.n2={j, anchor}});
					ast_insert(pool, ast_type_eq, (ast_value){.n2={anchor, ast_access(pool, ast_type_ptr, (ast_value){.p=ring})}});
					ast_insert(pool, ast_type_eq, (ast_value){.n2={ast_access(pool, ast_type_prop, (ast_value){.n2={anchor, nodel}}), booltrue}});
					ast_insert(pool, ast_type_eq, (ast_value){.n2={ast_access(pool, ast_type_prop, (ast_value){.n2={anchor, proc}}), call}}); // 使用call驱动
				}

			// 寻找f的输入输出，给f的输入输出添加load_builtin
			obs = ast_new();
			ast_iter(anchors, anchor)
			{
				ops = ast_collect_n2(pool, ast_type_prop, anchor, proc);
				ast_iter(ops, j) // 正常来说ops里只有一个
				{
					props = ast_get_prop_names(pool, i->v.n2[0]);
					ast_iter(props, k)
						if (k->t == ast_type_int && k->v.i != 0) {	// f的输入输出
							ast_insert(obs, ast_type_prop, (ast_value){.n2={j->v.n2[0], k}});
						}
					ast_delete(props);
				}
				ast_delete(ops);
			}

			ast_iter(obs, j) // 遍历f的输入输出
			{
				l = ast_access(pool, ast_type_any, (ast_value){.i=-1});
				m = ast_access(pool, ast_type_any, (ast_value){.i=-1});
				ast_insert(pool, ast_type_eq, (ast_value){.n2={j, l}});
				ast_insert(pool, ast_type_eq, (ast_value){.n2={l, m}});
				ast_iter(builtins, k)
					if (strstr(k->v.n1->v.s, "new")) // TODO:这里只添加了new的builtin用来生成obj, 后续还可以添加ser等
						ast_insert(pool, ast_type_eq, (ast_value){.n2={ast_access(pool, ast_type_prop, (ast_value){.n2={m, ast_access(pool, ast_type_str, (ast_value){.s0="new"})}}), k}});
			}

			ast_delete(obs);
			ast_delete(builtins);

			ast_iter(eqs, j)
				ast_iter(eqs, k)
					ast_subst(pool, ast_access(pool, ast_type_eq, (ast_value){.n2={j, k}}), ast_access(pool, ast_type_any, (ast_value){.i=-1}));

			// 下面处理f.somparam = blah
			this_loop_->deleted = 1; // 先把defcall节点删除,然后再进行处理
			ast_iter(eqs, j)
				if (j->t != ast_type_anchor) {
					attributes = ast_collect_n2(pool, ast_type_prop, j, NULL); // 找到所有的属性
				}

			ast_iter(attributes, j) // f有几组输入输出(有几个processor), 就要有对应组数的属性
				ast_iter(pool, k)
					if (k->t == ast_type_eq && k->v.n2[0] == j) {
						ast_iter(anchors, l)
							ast_insert(pool, ast_type_eq, (ast_value){.n2={ast_access(pool, ast_type_prop, (ast_value){.n2={l, k->v.n2[0]->v.n2[1]}}), k->v.n2[1]}});
						this_loop_->deleted = 1;
					}

			ast_delete(anchors);
			ast_delete(attributes);
			ast_delete(eqs);
			ast_delete(load_pool);
		}
}

static	int	ast_merge_connect(ast pool, ast inc, ast_node name)
{
	ast_node	i;
	int		res = 0;

	ast_iter(inc, i)
	{
		ast_merge(pool, cache_get(i->v.n2[0]));		// NOTE: should be ast_type_load_ast
		ast_insert(pool, ast_type_eq, (ast_value){.n2={i, name}});
		ast_subst(pool, i, ast_access(pool, ast_type_any, (ast_value){.i=-1}));
		pass_clear_name(pool);
		res = 1;
	}
	ast_delete(inc);

	return	res;
}

void	pass_load_ast(ast pool)
{
	ast		loads;
	ast_node	newast, i;
	int		flag;

	ast_access_push();
	do {
		flag = 0;

		cache_load(pool); // pass_load_files, 加载缓存

		pass_defcall(pool);

		pass_clear_name(pool);

		// FIXME: 好像文件不存在时会导致ast_copy内存访问错误，需要检查处理

		loads = ast_collect_type(pool, ast_type_load_op); // pass_merge_ast
		newast = ast_access(pool, ast_type_name, (ast_value){.n1=ast_access(pool, ast_type_str, (ast_value){.s0="newop"})});
		flag += ast_merge_connect(pool, loads, newast);

		loads = ast_collect_type(pool, ast_type_load_type); // pass_merge_ast
		newast = ast_access(pool, ast_type_name, (ast_value){.n1=ast_access(pool, ast_type_str, (ast_value){.s0="newtype"})});
		flag += ast_merge_connect(pool, loads, newast);
	} while (flag);
	ast_access_pop();
}


/*
 * 	2.3 发射操作
 */

static	ast_node	ast_cast_bool2int(ast pool, ast_node n)
{
	if (!n)
		return	ast_access(pool, ast_type_int, (ast_value){.i=-1});
	if (n == ast_access(pool, ast_type_bool, (ast_value){.i=1}))
		return	ast_access(pool, ast_type_int, (ast_value){.i=1});
	else if (n == ast_access(pool, ast_type_bool, (ast_value){.i=0}))
		return	ast_access(pool, ast_type_int, (ast_value){.i=0});
	else
			ddq_error("Wrong values for bool node.\n");
}

static	ast_node	ast_cast_ptr(ast pool, ast_node n)
{
	void *	p;

	if (!n)
		return	ast_access(pool, ast_type_ptr, (ast_value){.p=NULL});
	if (n->t == ast_type_ptr)
		return	n;
	else if (n->t == ast_type_load_builtin || n->t == ast_type_load_so)
	{
		p = cache_get(n);
		if (!p)
			ddq_warning("Failed loading pointer with node type#%d.\n", n->t);
		return	ast_access(pool, ast_type_ptr, (ast_value){.p=p});
	}
	else
		ddq_error("Wrong values for pointer node.\n");
}

static	ast_node	ast_cast_obj(ddq_ring ring, ast pool, ast_node n, int consumable, void *fd) // ast_node2obj
{
	void *		pbuf;

	if (!n)
		return	NULL;

	switch (n->t)
	{
		case	ast_type_load_builtin :
		case	ast_type_load_so :
		case	ast_type_ptr :
			return	ast_access(pool, ast_type_obj, (ast_value){.p=obj_import(ring, ast_cast_ptr(pool, n)->v.p, fd, (consumable==1) ? obj_prop_consumable|obj_prop_ready : obj_prop_ready)});

		case	ast_type_int :
		case	ast_type_bool :
			pbuf = malloc(sizeof(int));	// FIXME: 把这里的malloc/free换成builtin函数
			*(int*)(pbuf) = n->v.i;
			return	ast_access(pool, ast_type_obj, (ast_value){.p=obj_import(ring, pbuf, free, (consumable==1) ? obj_prop_consumable|obj_prop_ready : obj_prop_ready)});

		case	ast_type_real :
			pbuf = malloc(sizeof(double));
			*(double*)(pbuf) = n->v.r;
			return	ast_access(pool, ast_type_obj, (ast_value){.p=obj_import(ring, pbuf, free, (consumable==1) ? obj_prop_consumable|obj_prop_ready : obj_prop_ready)});

		case	ast_type_str :
			pbuf = malloc(strlen(n->v.s)+1);
			strcpy(pbuf, n->v.s);
			return	ast_access(pool, ast_type_obj, (ast_value){.p=obj_import(ring, pbuf, free, (consumable==1) ? obj_prop_consumable|obj_prop_ready : obj_prop_ready)});

		default	:
			ddq_error("Cannot convert type#%d to object.\n", n->t);
	}
}

// 负责处理所有不是关键词的属性，为obg中的每个元素生成meta，并且将每个meta和obg中的对应元素建立起映射并返回
ast pass_create_metas(ast pool, ast obg)
{
	#define MAX_LENGTH  20
	#define KEYWORD_NUM  8 // 根据需要后续可以进行调整

	ast		res;
	ast		keywords, props, eqs;
	ast_node	name, attribute, t;
	ast_node	i, j, k;
	meta	op_meta, new_meta, now_meta; // now_meta是现在最新的meta，op_meta是第一个meta，new_meta是新创建的meta

	res = ast_new();

	char keyword[KEYWORD_NUM][MAX_LENGTH] = {{"processor"},
											  {"consumable"},
                             				  {"new"},
							 				  {"del"},
											  {"nodel"},
											  {"ser"},
											  {"deser"},
										      {"size"}};
	keywords = ast_new();
	for (int i = 0; i < KEYWORD_NUM; i++) { // 关键词列表
		ast_insert_node(keywords, ast_access(pool, ast_type_str, (ast_value){.s0=keyword[i]}));
	}	

	props = ast_collect_type(pool, ast_type_prop); // 属性一定包含在prop中
	ast_iter(obg, i)
	{
		eqs = i->v.p; // 与当前ob构成eq的节点
		op_meta = new_meta = now_meta = NULL;
		attribute = t = NULL;
		ast_iter(props, j)
		{
			if (ast_is_in(eqs, j->v.n2[0]) && j->v.n2[1]->t == ast_type_str && !ast_is_in(keywords, j->v.n2[1]))  // 左边在eqs中，右边是str且不是keywords
			{
				if (!t) t = j->v.n2[0]; // t = 变量名/op名对应的any
				if (t != j->v.n2[0]) ddq_error("pass_create_metas() : Find attribute error.\n"); // 同一变量名/op名对应的any应该相同
			}
		}

		if (t)
		{
			ast_iter(pool, j)
			{
				if (j->t == ast_type_eq && j->v.n2[0]->v.n2[0] == t && j->v.n2[0]->v.n2[1]->t == ast_type_str && !ast_is_in(keywords, j->v.n2[0]->v.n2[1])) // 先找到该str所在的eq
				{
					attribute = j->v.n2[1]; // attribute = 属性对应的any
					name = j->v.n2[0]->v.n2[1]; // 属性的名
					ast_iter(pool, k)
					{
						if (k->t == ast_type_eq && k->v.n2[0] == attribute) // 根据any找到属性的值
						{
							attribute = k->v.n2[1]; // attribute = 属性的值
							break;
						}
					}

					switch (attribute->t)
						{
							case ast_type_int:
							case ast_type_bool:
								new_meta = meta_new(name->v.s, meta_type_int, (void*)&(attribute->v.i));
								break;
							case ast_type_real:
								new_meta = meta_new(name->v.s, meta_type_double, (void*)&(attribute->v.r));
								break;
							case ast_type_str:
								new_meta = meta_new(name->v.s, meta_type_string, (void*)(attribute->v.s));
								break;
							default:
								ddq_warning("pass_create_meta() : Unsupported meta type.\n");
								break;
						}

					if (new_meta)
					{
						if (!op_meta) 
						{
							op_meta = new_meta;
							now_meta = new_meta;
						}
						else 
						{
							now_meta->next = new_meta;
							now_meta = now_meta->next;
						}
					}
				}
			}
		}

		ast_map_set(res, i, ast_access(pool, ast_type_ptr, (ast_value){.p=op_meta}));

	}

	ast_delete(props);
	ast_delete(keywords);
	#undef MAX_LENGTH
	#undef KEYWORD_NUM

	return res;
}

ddq_ring	pass_create_ring(ast pool, int_t ring_input_num, int_t ring_output_num, ast_node newop)
{
	ddq_ring	res;
	ast		ops, obs, obg, props, eqs;
	ast		consumable, fn, fd, object;
	ast		dups;
	ast		gt;
	ast		defcallmap;
	ast		ob_metas;
	ast_node	n_consumable, n_new, n_del, n_nodel;
	ast_node	i, j, k, t1, t2;
	int		flag;
	int		maxin, maxout, ind;
	ddq_op		op;
	ddq_type_t	iproc;
	char *		buf;
	void *		pt;

	res = ddq_new(NULL, ring_input_num, ring_output_num);

	ast_access_push();

	ops = ast_collect_n2(pool, ast_type_prop, NULL, ast_access(pool, ast_type_str, (ast_value){.s0="processor"}));
	obs = ast_new(); // 存放需要生成obj的node
	ast_iter(ops, i)
	{
		ast_insert_node(obs, i->v.n2[0]);
		props = ast_get_prop_names(pool, i->v.n2[0]);
		ast_iter(props, j)
			if (j->t == ast_type_int && j->v.i != 0)	// FIXME: 0号参数有没有用处？
				ast_insert(obs, ast_type_prop, (ast_value){.n2={i->v.n2[0], j}});
		ast_delete(props);
	}
	obg = ast_group_eq(pool, obs); // 获取每个obj的eq

	n_consumable = ast_access(pool, ast_type_str, (ast_value){.s0="consumable"});
	n_new = ast_access(pool, ast_type_str, (ast_value){.s0="new"});
	n_del = ast_access(pool, ast_type_str, (ast_value){.s0="del"});
	n_nodel = ast_access(pool, ast_type_str, (ast_value){.s0="nodel"});
	consumable = ast_new();
	fn = ast_new();
	fd = ast_new();
	defcallmap = ast_new(); // map: 内部输入输出 -> 外部输入输出
	ast_iter(obg, i)
	{
		eqs = i->v.p; // eqs = ast_find_eq(pool, ob)，ob是obg对应的obs中的元素
		if (newop->t == ast_type_anchor)
			ast_iter(eqs, j)
			{ 
				if (j->t == ast_type_prop && j->v.n2[0] == newop) // 建立起内部输入输出到外部输入输出的映射
				{
					if (j->v.n2[1]->t == ast_type_int)
						ast_map_set(defcallmap, i, j->v.n2[1]);
					else 
						ddq_error("Map from ring-inside to ring-outside error.\n");
				}
			}
		props = ast_get_props(pool, i->v.p);
		ast_map_set(consumable, i, ast_cast_bool2int(pool, ast_map_get(props, n_consumable)));
		ast_map_set(fn, i, ast_cast_ptr(pool, ast_map_get(props, n_new)));
		ast_map_set(fd, i, (ast_cast_bool2int(pool, ast_map_get(props, n_nodel))->v.i == 1) ? ast_access(pool, ast_type_ptr, (ast_value){.p=NULL}) : ast_cast_ptr(pool, ast_map_get(props, n_del)));
		ast_delete(props);
	}

	// TODO: 这个函数随后应该处理metadata，即不认识的所有字符串属性
	// 目前还有一些关键属性可以处理，包括size, ser, deser等

	ob_metas = pass_create_metas(pool, obg);
	ast_delete(obs);

	dups = ast_new();
	ast_iter(pool, i)
		if (i->t == ast_type_dup)
		{
			t1 = NULL;
			t2 = NULL;
			ast_iter(obg, j)
			{
				if (ast_is_in(j->v.p, i->v.n2[0]))
					t1 = j;
				else if (ast_is_in(j->v.p, i->v.n2[1]))
					t2 = j;
			}
			if (t1 && t2)
				ast_insert(dups, ast_type_dup, (ast_value){.n2={t1, t2}});
		}

	object = ast_new(); // 创建的obj
	ast_iter(obg, i)
	{
		j = ast_cast_obj(res, pool, ast_dist(i->v.p), ast_map_get(consumable, i)->v.i, ast_map_get(fd, i)->v.p); // ast_node2obj
		if (j) {// 如果创建obj成功，加入object
			ast_map_set(object, i, j);
		}
	}

	for (flag = 1; flag;)
	{
		ast_iter(dups, i)  // 创建dup的obj
		{
			t1 = ast_map_get(object, i->v.n2[0]);
			t2 = ast_map_get(object, i->v.n2[1]);
			if (t1 && !t2)
			{
				ast_map_set(object, i->v.n2[1], ast_access(pool, ast_type_obj, (ast_value){.p=obj_dup(res, t1->v.p)}));
				ast_remove(dups, i);
			}
			else if (!t1 && t2)
			{
				ast_map_set(object, i->v.n2[0], ast_access(pool, ast_type_obj, (ast_value){.p=obj_dup(res, t2->v.p)}));
				ast_remove(dups, i);
			}
			else if (t1 && t2)
				ddq_error("Conflict dup objects.\n");
		}

		flag = 0;
		ast_iter(obg, i)  // 检查每个obj是否都已经成功创建
			if (!ast_map_get(object, i))
			{
				t1 = ast_map_get(fn, i);
				t2 = ast_map_get(fd, i);
				if (t1->v.p) // 处理.new = load_builtin
				{
					ast_map_set(object, i, ast_access(pool, ast_type_obj, (ast_value){.p=obj_new(res, t1->v.p, t2->v.p, (ast_map_get(consumable, i)->v.i == 0) ? 0 : obj_prop_consumable)}));
					flag = 1;
					break;
				}
				else if (ast_map_get(defcallmap, i))
				{
					ast_map_set(object, i, ast_access(pool, ast_type_obj, (ast_value){.p=obj_import(res, NULL, NULL, obj_prop_toassign)}));
					flag = 1;
					break;
				}
				else {
					ddq_error("No construct_f for obj.\n");
				}
			}
	}

	// 此处应检查dups应已空、obg中所有元素在object和ob_metas里都有映射
	ast_iter(dups, i)
		ddq_error("Dups not empty.\n");

	ast_iter(obg, i)
	{
		if (!ast_map_get(object, i))
			ddq_error("Map not complete from obg to object.\n");
		if (!ast_map_get(ob_metas, i))
			ddq_error("Map not complete from obg to ob_metas.\n");
	}

	ast_delete(dups);
	ast_delete(consumable);
	ast_delete(fn);
	ast_delete(fd);

	// obj创建完毕，执行ast_node2op

	ast_iter(ops, i)
	{
		gt = ast_find_eq(pool, i);
		j = ast_dist(gt);
		ast_delete(gt);
		if (j && j->t == ast_type_str)
		{
			buf = malloc(strlen(j->v.s)+strlen("processor_")+1);
			strcpy(buf, "processor_");
			strcat(buf, j->v.s);
			iproc = ddq_type_first;
#define	stringify1(x)	#x
#define	stringify(x)	stringify1(x)
#define	ddq_enable(x)	{ if (strcmp(buf, stringify(x))==0) iproc = x; }
#include	"ddq_types_list.h"
#undef	ddq_enable
#undef	stringify
#undef	stringify1
			free(buf);
			if (iproc == ddq_type_first)
				ddq_error("Unknown processor name.\n");
		}
		else
			ddq_error("Wrong processor name.\n");

		maxin = maxout = 0;
		props = ast_get_prop_names(pool, i->v.n2[0]);
		ast_iter(props, j)
			if (j->t == ast_type_int)
			{
				if (j->v.i < maxin)
					maxin = j->v.i;
				if (j->v.i > maxout)
					maxout = j->v.i;
			}
		ast_delete(props);

		op = ddq_spawn(res, iproc, -maxin, maxout);
		ast_iter(obg, j)
			if (ast_is_in(j->v.p, i->v.n2[0]))
			{
				// ddq_add_f(op, ast_map_get(object, j)->v.p);
				op->f = ast_map_get(object, j)->v.p;
				op->metadata = ast_map_get(ob_metas, j)->v.p;
				break;
			}

		for (ind = maxin; ind <= maxout; ind ++) // 检查op参数
			if (ind != 0)
			{
				ast_iter(obg, j)
					if (ast_is_in(j->v.p, ast_access(pool, ast_type_prop, (ast_value){.n2={i->v.n2[0], ast_access(pool, ast_type_int, (ast_value){.i=ind})}})))
					{
						pt = ast_map_get(object, j)->v.p;
						if (!pt)
							ddq_error("Lack of parameters of operator, missing #%d.\n", ind);
						if (ind < 0) 
						{
							// ddq_add_inputs(op, -ind-1, pt);
							op->inputs[-ind-1] = pt;
							op->inputs[-ind-1]->metadata = ast_map_get(ob_metas, j)->v.p;
							if (ast_map_get(defcallmap, j)) // 如果非空，说明是外部的输入和输出
							{
								int_t idx = ast_map_get(defcallmap, j)->v.i;
								op->inputs[-ind-1]->p = (void*)(idx); // 进行这个操作之前，p为NULL
								op->inputs[-ind-1]->prop = obj_prop_toassign;
							}
						}
						else 
						{
							// ddq_add_outputs(op, ind-1, pt);
							op->outputs[ind-1] = pt;
							op->outputs[ind-1]->metadata = ast_map_get(ob_metas, j)->v.p;
							if (ast_map_get(defcallmap, j)) // 如果非空，说明是外部的输入和输出
							{
								int_t idx = ast_map_get(defcallmap, j)->v.i;
								op->outputs[ind-1]->p = (void*)(idx); // 进行这个操作之前，p为NULL
								op->outputs[ind-1]->prop = obj_prop_toassign;
							}
						}
					}
			}
	}

	ast_delete(ops);
	ast_group_delete(obg);
	ast_delete(object);
	ast_delete(ob_metas);
	ast_delete(defcallmap);

	ast_access_pop();

	return	res;
}



/*
 * 3. 编译主程序：
 */

ddq_ring	ast2ring(ast pool)
{
	ddq_ring	res;
	ast_node	i, j;
	ast_node	newop;
	int_t		input_num = 0, output_num = 0;

	ast_access_push();

	// 寻找“newop”，如果有，计算输入和输出的个数传入pass_create_ring
	newop = ast_access(pool, ast_type_str, (ast_value){.s0="newop"});
	ast_iter(pool, i)
	{
		if (i->t == ast_type_eq && i->v.n2[1]->t == ast_type_name && i->v.n2[1]->v.n1 == newop)
		{
			ast props = ast_get_prop_names(pool, i->v.n2[0]);
			ast_iter(props, j)
			if (j->t == ast_type_int)
			{
				if (j->v.i < 0) input_num++;
				if (j->v.i > 0) output_num++;
			}
			ast_delete(props);
			newop = i->v.n2[0];
			break;
		}
	}

	pass_load_ast(pool);

	res = pass_create_ring(pool, input_num, output_num, newop);

	ast_delete(pool);
	ast_access_pop();

	return	res;
}



