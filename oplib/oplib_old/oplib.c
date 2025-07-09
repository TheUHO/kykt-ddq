
#include	"oplib.h"
#include	"oplib_private.h"

#include	<stdlib.h>
#include	<string.h>


static	oplib_hash	phash;


oplib_hash	oplib_hash_pack(oplib_type type, char *libname, char *funcname)
{
	oplib_hash	h;

	// FIXME: 这里应当检查字符串指针的长度是否合理，防止意外或恶意的输入。

	if (libname == NULL)
		libname = "";
	if (funcname == NULL)
		funcname = "";

	h = (oplib_hash)malloc(sizeof(oplib_hash_t)+strlen(libname)+1+strlen(funcname)+1);
	h->len = sizeof(oplib_type)+strlen(libname)+1+strlen(funcname)+1;
	h->type = type;
	strcpy(h->buf, libname);
	strcpy(h->buf+strlen(libname)+1, funcname);

	return	h;
}

void	oplib_hash_put(oplib_hash h)
{
	oplib_hash	h0;

	HASH_REPLACE(hh, phash, type, h->len, h, h0);
	if (h0)
		free(h0);
}

void	oplib_hash_get(oplib_hash *h)
{
	oplib_hash	h0;

	HASH_FIND(hh, phash, &(*h)->type, (*h)->len, h0);

	free(*h);
	*h = h0;
}


void	oplib_put(task_f_wildcard *p, oplib_type type, char *libname, char *funcname)
{
	oplib_hash	h;

	h = oplib_hash_pack(type, libname, funcname);
	h->p.f = p;
	oplib_hash_put(h);
}

obj	oplib_get(oplib_type type, char *libname, char *funcname)
{
	char	*libdirect = "";
	char	*fundirect = "op_%s";
	char	*libso = "%s/op_%s.so";
	char	*funso = "op_%s";
	char	*libtianhe3 = "%s/op_%s.dat";	// FIXME
	char	*funtianhe3 = "%s";

	oplib_hash	h;
	char	*sl, *sf;

	switch (type)
	{
		case oplib_direct :
			sl = malloc(strlen(libdirect)+1);
			sf = malloc(strlen(fundirect)+strlen(funcname)+1);
			sprintf(sl, libdirect);		// FIXME: 这里有个warning?
			sprintf(sf, fundirect, funcname);
			break;
		case oplib_dl :
			sl = malloc(strlen(libso)+strlen(oplib_pref)+strlen(libname)+1);
			sf = malloc(strlen(funso)+strlen(funcname)+1);
			sprintf(sl, libso, oplib_pref, libname);
			sprintf(sf, funso, funcname);
			break;
		case oplib_tianhe :
		case oplib_tianhe_run :
			sl = malloc(strlen(libtianhe3)+strlen(oplib_pref)+strlen(libname)+1);
			sf = malloc(strlen(funtianhe3)+strlen(funcname)+1);
			sprintf(sl, libtianhe3, oplib_pref, libname);
			sprintf(sf, funtianhe3, funcname);
			break;
		default :
				ddq_error("");
			// TODO: 这里应当报错！
	}

	h = oplib_hash_pack(type, sl, sf);
	oplib_hash_get(&h);

	if (!h)
	{
		switch (type)
		{
			case oplib_direct :
				// TODO: 这里应当报错！
				break;
			case oplib_dl :
				oplib_load_so(sl, sf);
				break;
			case oplib_tianhe :
			case oplib_tianhe_run :
				// TODO
				break;
		}

		h = oplib_hash_pack(type, sl, sf);
		oplib_hash_get(&h);
	}

	free(sf);
	free(sl);
	return obj_import(h->p.f, NULL, obj_prop_ready);
}

void	oplib_init()
{
	phash = NULL;

	// FIXME: oplib_load_builtin没有放入oplib.h里，是考虑到静态编译的算子没道理动态管理。如果真的有这样奇怪需求，就需要与宏污染的代价权衡一下了。
#define	stringify(x)	#x
#define	str(x)		stringify(x)
#define	oplib_load_builtin(fn)	oplib_put((task_f_wildcard *)(fn), oplib_direct, "", str(fn))
#include "oplib_init.h"
#undef	oplib_load_builtin
#undef	str
#undef	stringify

}


