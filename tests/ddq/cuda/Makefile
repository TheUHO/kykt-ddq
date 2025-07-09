CFLAGS=-O2 -g -I../cpu -I../../../common -I../../../ -I../../../ddq -I../../../oplib -I../../../operators -I/usr/include/openblas -g -Denable_processor_direct -Denable_processor_pthread -Denable_processor_call -Denable_processor_fork -Denable_processor_pthread_pool -Denable_processor_cuda


install_path=../../../compiled_cuda
export LDLIBS=$(install_path)/lib/ddq/*.a
LDFLAGS= $(LDLIBS) -ldl -lpthread -lopenblasp64 -lm -lcudart -L/usr/local/cuda/lib64 -lcublas

NVCC=nvcc
CC=gcc

EXEC = test

# 查找当前目录下的所有 .c 文件
SRCS = $(wildcard *.cu)
OBJS = $(SRCS:.cu=.o)

all: $(OBJS) $(EXEC)

$(EXEC): $(OBJS) ../cpu/matrix.o ../cpu/dag.o
	$(CC) $(OBJS) ../cpu/matrix.o ../cpu/dag.o ${CFLAGS} ${LDFLAGS} -o $@

%.o: %.cu
	$(NVCC) -c $< ${CFLAGS} -o $@

%	:	%.cu
	-$(RM) $@
	$(CC) $< ${CFLAGS} ${LDFLAGS} -o $@

run:
	make && ./$(EXEC)

run_log:
	make && ./$(EXEC) &> txt

run_gdb:
	make && gdb ./$(EXEC)

clean : 
	$(RM) $(EXEC) ${OBJS}

.PHONY	: all clean