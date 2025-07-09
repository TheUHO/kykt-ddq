#ifndef	F_BASIC_TYPES_H_
#define	F_BASIC_TYPES_H_


// for datadesc.h, omi.h
typedef	long		int_t;
typedef	unsigned long	uint_t;		// FIXME: 为了避免UB的代码风险，是否应当尽量不使用unsigned类型呢？注意到，谷歌的内部标准要求如此。在C标准中，unsigned类型具有更好的跨平台性质，这点是否需要考虑？

// for ddq_plugin.h
typedef	unsigned int	flag_t;

// for ddq_plugin.h
typedef	unsigned long	ddq_label;


#endif
