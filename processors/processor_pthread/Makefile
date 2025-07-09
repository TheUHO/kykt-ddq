
all	:	libprocessor_pthread.a

libprocessor_pthread.a	:	processor_pthread_private.o processor_pthread_public.o
	$(AR) rcs $@ $^

%.o	:	%.c
	$(CC) -c $< $(CFLAGS) -I../../ddq -o $@

clean	:
	$(RM) *.a *.o

.PHONY	:	all clean

