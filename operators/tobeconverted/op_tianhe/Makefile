
all	:	op_tianhe.so


%.so	:	CFLAGS +=-fPIC -shared

%.so	:	%.c	%.h
	$(CC) ${CFLAGS} -o $@ -Wl,-whole-archive $(HTHREAD_HOME)/lib/libhthread_host.a -Wl,-whole-archive ../../compiled/ddq_$(target).a -Wl,-no-whole-archive $<

clean	:
	$(RM) *.so *.o

.PHONY	:	all clean
