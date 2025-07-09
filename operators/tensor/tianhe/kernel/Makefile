all	:	op.o types.o tests.o

%.o	:	%.c %.h
	$(CC) -c $< ${CFLAGS} -o $@

clean	:
	$(RM) *.so *.o

.PHONY	:	all clean