#ifndef	F_OPLIB_H
#define	F_OPLIB_H


char *	find_filename(char *fn);

void	load_builtin_init();
void *	load_builtin(char *key);
void	load_builtin_finish();
// void	load_builtin_register(char *key, void *value);		// NOTE: internal use only


void *	load_ast_file(const char *libname);
void	load_ast_close(void *handle);

#ifdef	oplib_load_so
void	load_so_init();
void *	load_so_open(const char *libname);
void *	load_so_sym(void *restrict handle, const char *restrict symbol);
int	load_so_close(void *handle);
void	load_so_finish();
#endif

#ifdef oplib_load_tianhe
void	load_tianhe_init();
void    load_tianhe_finish();
void*	load_tianhe(char *funcname);
#endif

#endif
