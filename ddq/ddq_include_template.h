
#define	ident(x)	x
#define stringify(x)	#x
#define	str(x)		stringify(x)
#define inc(hash, x)	ident(hash)include str(../processors/ident(x)/ident(x).h)
#define	hash #
#define ddq_enable(x)	inc(hash, x)
#include	"ddq_types_list.h"
#undef	ddq_enable
#undef	hash
#undef	inc
#undef	str
#undef	stringify
#undef	ident

