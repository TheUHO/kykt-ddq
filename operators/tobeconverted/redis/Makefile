
target	:	op_redis.so


CFLAGS=-I../../common -O2

%.so	:	CFLAGS +=-fPIC -shared

%.so	:	%.c	%.h
	$(CC) $< ${CFLAGS} -o $@

clean	:
	$(RM) *.so

.PHONY	:	target clean

