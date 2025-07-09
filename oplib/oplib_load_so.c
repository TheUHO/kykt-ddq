
#include	<dlfcn.h>
#include	<string.h>
#include	<stdlib.h>

#include	"error.h"
#include	"oplib.h"


void	load_so_init()
{
	return;
}

void *	load_so_open(const char *libname)
{
	void *	res;
	char *	buf;

	buf = malloc(strlen(libname)+4);
	strcpy(buf, libname);
	strcat(buf, ".so");
	buf = find_filename(buf);
	if (buf)
	{
		res = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
		if (!res)
			ddq_error("Error: Failed loading file '%s' and get message '%s'.\n", buf, dlerror());
		free(buf);
	}
	else
		ddq_error("Error: file not found for so function '%s'.\n", libname);

	return	res;
}

void *	load_so_sym(void *restrict handle, const char *restrict symbol)
{
	void *	res;

	res = dlsym(handle, symbol);
	if (!res)
		ddq_error("Error: function not found '%s'.\n", symbol);

	return	res;
}

int	load_so_close(void *handle)
{
	return	dlclose(handle);
}

void	load_so_finish()
{
	return;
}

