
all	:	op_fib.so


%.so	:	CFLAGS +=-fPIC -shared

%.so	:	%.c	%.h
	$(CC) $< ${CFLAGS} -o $@

clean	:
	-$(RM) *.so

.PHONY	:	all clean

