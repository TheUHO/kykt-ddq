#ifndef	F_ERROR_H_
#define	F_ERROR_H_

#include	<stdio.h>		
#include	<stdlib.h>

#define aligned8(n) ((int)((n+7)/8)*8)

#define ACCESS_DATA(ptr, type) \
		((type)(ptr))
#define ACCESS_INPUTS(index, type) \
		ACCESS_DATA(inputs[index], type)

#define ACCESS_OUTPUTS(index, type) \
		ACCESS_DATA(outputs[index], type)

#define WRITE_OUTPUTS(index, type, data) \
		*(type*)outputs[index] = data



#define DDQ_LOG_INIT_SIZE 16
#define NAME_DDQ_LOG "ddq_log"
#define NAME_DDQ_LOG_SIZE "ddq_log_size"

#ifdef ddq_op_region
	#include "ddq.h"
	#include "ddq_plugin.h"
	#include <string.h>
	#define ddq_log0(...) \
		do{ \
			meta m_log = name2meta(inputs[-1], NAME_DDQ_LOG); \
			meta m_log_size = name2meta(inputs[-1], NAME_DDQ_LOG_SIZE); \
			char* log_ptr = m_log->value.sval; \
			int log_size = m_log_size->value.ival; \
			int used_size = strlen(log_ptr);\
			int tmp_size;	\
			while((tmp_size = snprintf(log_ptr + used_size, log_size - used_size, __VA_ARGS__)) >= 	log_size - used_size) \
			{	\
				log_size *= 2;	\
				char* s = malloc(log_size);	\
				strncpy(s, log_ptr, used_size);	\
				free(log_ptr);	\
				log_ptr = s;	\
			}	\
			m_log->value.sval = log_ptr;	\
			m_log_size->value.ival = log_size;	\
		} while (0)
#elif defined(enable_processor_dsp)
	#include <hthread_device.h>

	#define	aligned_malloc(required_bytes, alignment)	\
		({	\
		int offset = alignment - 1 + sizeof(void*);	\
		void* p1 = (void*)scalar_malloc(required_bytes + offset);	\
		if (p1 == NULL) ddq_error("dsp aligned_malloc() : out of memory! %d\n",get_sm_free_space());	\
		void** p2 = (void**)( ( (size_t)p1 + offset ) & ~(alignment - 1) );	\
		p2[-1] = p1;	\
		(void*)p2;	\
		})

	#define aligned_free(p2)	\
	({	\
		void* p1 = ((void**)p2)[-1];	\
		scalar_free(p1);	\
	})

	#define	malloc(s)	aligned_malloc(s, 8)

	#define free(s)		aligned_free(s)

	#define ddq_log0(...) \
		do { \
			hthread_printf(__VA_ARGS__); \
		} while (0)
#else

	#define ddq_log0(...)	\
		do {	\
			fprintf(stdout, __VA_ARGS__);	\
			fflush(stdout);	\
		} while (0)

	#define	malloc_wrap(s)	\
	({	\
	 void	*p = malloc(s);	\
	 if (p == NULL) ddq_error("malloc_wrap() : out of memory!\n");	\
	 p;	\
	 })

	#define	malloc(s)	malloc_wrap(s)

	#define p_free(s)	\
	({	\
		free(s);	\
	})

	#define free(s) p_free(s);
#endif


#define	ddq_error(...)	\
	do {	\
		ddq_log0("Error occurs at Line #%d of File '%s : '", __LINE__, __FILE__);	\
		ddq_log0(__VA_ARGS__);	\
		exit(-1);	\
	} while (0)

#define	ddq_warning(...)	\
	do {	\
		ddq_log0("Warning at Line #%d of File '%s : '", __LINE__, __FILE__);	\
		ddq_log0(__VA_ARGS__);	\
	} while (0)

#define	ddq_log(...)	\
	do {	\
		ddq_log0("Log at Line #%d of File '%s : '", __LINE__, __FILE__);	\
		ddq_log0(__VA_ARGS__);	\
	} while (0)

// 发布版本用这个宏取消所有log：
//#define	ddq_log(...)

#endif