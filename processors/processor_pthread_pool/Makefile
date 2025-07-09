
all	:	libprocessor_pthread_pool.a

libprocessor_pthread_pool.a	:	processor_pthread_pool_private.o
	$(AR) rcs $@ $^

%.o	:	%.c
	$(CC) -c $< $(CFLAGS) -I../../ddq -o $@

clean	:
	$(RM) *.a *.o

.PHONY	:	all clean

