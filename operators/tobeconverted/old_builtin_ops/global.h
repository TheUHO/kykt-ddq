#ifndef	F_GLOBAL_H
#define	F_GLOBAL_H

#include	"pool.h"


void		global_init();

void		builtin_set(char *key, void *value);
void *		builtin_get(char *key);

//ast_node	cache_access(ast_node value);


#endif
