#ifndef DDQ_TYPES_H
#define DDQ_TYPES_H

#define	ddq_enable(x)	x,
typedef	enum
{
	ddq_type_first = 0,
#include	"ddq_types_list.h"
	ddq_type_last
} ddq_type_t;
#undef	ddq_enable

#endif
