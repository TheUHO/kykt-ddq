
all	:	processor_direct_private.o


%.o	:	%.c
	$(CC) -c $< ${CFLAGS} -o $@

clean	:
	$(RM) *.o

.PHONY	:	all clean

