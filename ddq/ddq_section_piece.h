
#define	ident(x)	x
#define	stringify(x)	#x
#define	str(x)		stringify(x)

#undef	ddq_p
#undef	ddq_s
#define	ddq_s_t(t)	t##_share
#define	ddq_s_(t)	ddq_s_t(t)
#define ddq_s		((struct ddq_s_(this_type) *)ddq_type_share[this_type])
#define	ddq_p_t(t)	t##_t
#define	ddq_p_(t)	ddq_p_t(t)
#define	ddq_p		((struct ddq_p_(this_type) *)op)

#define	fn(a)		str(../processors/ident(a)/ident(a).c)
#include	fn(this_type)
#undef	fn

#undef	ddq_p_
#undef	ddq_p_t
#undef	ddq_s_
#undef	ddq_s_t

#undef	str
#undef	stringify
#undef	ident

#undef	this_type

