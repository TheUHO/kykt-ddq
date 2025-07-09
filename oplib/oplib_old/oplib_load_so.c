
#include	"oplib_private.h"

#include	<dlfcn.h>


void	oplib_load_so(char *libname, char *funcname)
{
	oplib_hash	h;
	void *		h_dl;

	//add lib in hashtable
	h = oplib_hash_pack(oplib_dl, libname, "");
	oplib_hash_get(&h);
	if (h)
		h_dl = h->p.d;
	else
	{
		h_dl = dlopen(libname, RTLD_LAZY | RTLD_GLOBAL);	// FIXME: 这里的flags参数是否合理？
		if (h_dl == NULL)
			ddq_error("oplib_load_so() : dlopen() with file '%s' failed!\n %s\n", libname, dlerror());
		h = oplib_hash_pack(oplib_dl, libname, "");
		h->p.d = h_dl;
		oplib_hash_put(h);
	}

	//add function in hashtable
	h = oplib_hash_pack(oplib_dl, libname, funcname);
	h->p.f = (task_f_wildcard *)dlsym(h_dl, funcname);		// FIXME: 其实dlsym返回的是void*，似乎能装下函数指针，但也许有UB危险？
	if (h->p.f == NULL)
		ddq_error("oplib_load_so() : dlsym() with function '%s' from file '%s' failed!\n %s\n", funcname, libname, dlerror());
	oplib_hash_put(h);
}


