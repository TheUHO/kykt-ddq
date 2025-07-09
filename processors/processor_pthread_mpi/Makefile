
all	:	processor_pthread_mpi_private.o


%.o	:	%.c %.h
	$(CC) -c $< ${CFLAGS} -o $@

clean	:
	$(RM) *.o

.PHONY	:	all clean

