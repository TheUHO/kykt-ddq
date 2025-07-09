
#include	<string.h>
#include	<stdlib.h>

#include	"error.h"
#include	"ast.h"
#include	"pool.h"
#include	"oplib.h"


void *	load_ast_file(const char *libname)
{
	void *	res;
	char *	buf;

	buf = malloc(strlen(libname)+5);
	strcpy(buf, libname);
	strcat(buf, ".ddq");
	buf = find_filename(buf);
	if (buf)
	{
		res = file2ast(buf);
		free(buf);
	}
	else
		ddq_error("File '%s' not found.\n", libname);

	return	res;
}

void	load_ast_close(void *handle)
{
	ast_delete(handle);
	return;
}


