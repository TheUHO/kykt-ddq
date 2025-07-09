
all	: libprocessor_tianhe_private.a processor_tianhe_kernel.o

processor_tianhe_private.o	:	processor_tianhe_private.c  $(LIBMT_HOME)/lib/libmt.a
	$(CC) -c $< $(CFLAGS) -I../../ddq -o $@

processor_tianhe_kernel.o : processor_tianhe_kernel.c
	$(DSP_CC) -c $< ${DSP_CFLAGS} -o $@

lib%.a	:	%.o
	$(AR) rcs $@ $<

clean	:
	$(RM) *.a *.o

.PHONY	:	all clean

