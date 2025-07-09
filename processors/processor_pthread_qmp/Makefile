
all	:	libprocessor_pthread_qmp_private.a

lib%.a	:	%.o
	$(AR) rcs $@ $<

%.o	:	%.c
	mpicc -c $< $(CFLAGS) -I../../ddq -o $@

clean	:
	$(RM) *.a *.o

.PHONY	:	all clean

