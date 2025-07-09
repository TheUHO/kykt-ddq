
all	:	libprocessor_call_private.a

lib%.a	:	%.o
	$(AR) rcs $@ $<

%.o	:	%.c
	$(CC) -c $< $(CFLAGS) -I../../ddq -o $@

clean	:
	$(RM) *.a *.o

.PHONY	:	all clean

