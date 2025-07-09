CFLAGS=-O2 -g -fopenmp -I/usr/include/openblas -I../../../common -I../../../ddq -I../../../oplib -I../../../operators -Denable_processor_direct -Denable_processor_pthread -Denable_processor_call -Denable_processor_fork -Denable_processor_pthread_pool


install_path=../../../compiled_cuda
export LDLIBS=$(install_path)/lib/ddq/*.a
LDFLAGS=$(LDLIBS) -lopenblasp64 -lm -pthread -lcudart -L/usr/local/cuda/lib64 -lcublas

CC=gcc

EXEC = test_pool

# 查找当前目录下的所有 .c 文件
SRCS = dag.c ddq_dag.c test.c matrix.c
OBJS = $(SRCS:.c=.o)

all: clean $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) ../cuda/ddq_cuda.o ${CFLAGS} ${LDFLAGS} -o $@

%.o: %.c
	$(CC) -c $< ${CFLAGS} -o $@

%	:	%.c	
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