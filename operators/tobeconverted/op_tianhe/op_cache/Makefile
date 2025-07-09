#tianhe3_cpu make op_cache
#tianhe3_dsp make op_cache_kernel

ifeq ($(target),tianhe3_cpu)
all	:	op_cache
else ifeq ($(target),tianhe3_dsp)
all	:	op_cache_kernel
else
all	:
	$(error op_cache supports only tianhe3_cpu/tianhe3_dsp targets)
endif

op_cache: op_cache.so

op_cache_kernel: op_cache_tianhe.o private_cache.o

op_cache_tianhe.o	:	op_cache_tianhe.c
	${CC} ${CFLAGS} op_cache_tianhe.c -o op_cache_tianhe.o

private_cache.o 	: private_cache.c
	${CC} ${CFLAGS} private_cache.c -o private_cache.o

%.so	:	CFLAGS +=-fPIC -shared

%.so	:	%.c	%.h
	$(CC) $< ${CFLAGS} -o $@

clean	:
	$(RM) *.so *.o

.PHONY	:	all clean
