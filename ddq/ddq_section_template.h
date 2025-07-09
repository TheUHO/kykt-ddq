
#define	ident(x)	x
#define stringify(x)	#x
#define	str(x)		stringify(x)
#define	def(hash, x)	ident(hash)define this_type x
#define	hash #
#define ddq_enable(x)	def(hash, x)
#include	"ddq_types_list.h"
#undef	ddq_enable
#undef	hash
#undef	inc
#undef	str
#undef	stringify
#undef	ident

