all	:	ops.o types.o

%.o	:	%.c %.h
	$(CC) -c $< ${CFLAGS} -o $@

clean	:
	$(RM) *.so *.o

.PHONY	:	all clean