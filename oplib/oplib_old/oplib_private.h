#ifndef	F_OPLIB_PRIVATE_H
#define	F_OPLIB_PRIVATE_H

#include	"oplib.h"

#include	"uthash.h"


struct	oplib_hash_t
{
	UT_hash_handle	hh;

	union
	{
		void		* d;
		task_f_wildcard	* f;
	} p;

	//key len
	uint_t		len;
	//key ptr
	oplib_type	type;
	char		buf[];
};
typedef	struct oplib_hash_t	oplib_hash_t;
typedef	oplib_hash_t *	oplib_hash;


oplib_hash	oplib_hash_pack(oplib_type type, char *libname, char *funcname);
void	oplib_hash_put(oplib_hash h);
void	oplib_hash_get(oplib_hash *h);

void oplib_init_tianhe();


#endif
