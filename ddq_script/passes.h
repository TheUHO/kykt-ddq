#ifndef	F_PASSES_H
#define	F_PASSES_H

#include	"ast.h"
#include	"pool.h"
#include	"ddq.h"


void	cache_clear();

void	pass_load_ast(ast pool);
void	pass_create_objs(ast pool);
void	pass_dup_objs(ast pool);
void	pass_create_ops(ast pool, ddq_ring ring);
ast     pass_create_metas(ast pool, ast obg);

ddq_ring	ast2ring(ast pool);



#endif
