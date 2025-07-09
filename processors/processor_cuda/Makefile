
all	:	libprocessor_cuda.a

libprocessor_cuda.a	:	processor_cuda_private.o
	$(AR) rcs $@ $^

%.o	:	%.cu
	nvcc -c -O2 $< $(CU_CFLAGS) -o $@

clean	:
	$(RM) *.a *.o

.PHONY	:	all clean

