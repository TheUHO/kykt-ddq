
#include	"std_types.h"

void		obj_realloc(obj_mem po, int_t size);	// FIXME: 这个函数先临时这么弄，回头得考虑这类函数对于不同类型的统一管理

// load_builtin_register("str2obj", str2obj);
// load_builtin_register("strcat2obj", strcat2obj);
// load_builtin_register("obj_cat", obj_cat);
task_ret	str2obj(void **inputs, void **outputs, void **attribute);
task_ret	strcat2obj(void **inputs, void **outputs, void **attribute);
task_ret	obj_cat(void **inputs, void **outputs, void **attribute);

// load_builtin_register("obj_print", obj_print);
task_ret	obj_print(void **inputs, void **outputs, void **attribute);



