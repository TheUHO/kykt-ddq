
all	:	common_objs.o

%.o	:	%.c %.h
	$(CC) -c $(CFLAGS) $(LDFLAGS) $< -o $@

clean	:
	$(RM) *.o

.PHONY	:	all clean

