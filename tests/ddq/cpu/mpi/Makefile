DDQ_HOME=/data/hanbin/workspace/ddq

CFLAGS=-O2 -g -fopenmp -I/usr/include/openblas -I$(DDQ_HOME)/common -I$(DDQ_HOME)/ddq -I$(DDQ_HOME)/oplib -I$(DDQ_HOME)/operators -Denable_processor_direct -Denable_processor_pthread -Denable_processor_call -Denable_processor_fork -Denable_processor_pthread_pool -Denable_processor_cuda



install_path=$(DDQ_HOME)/compiled_cuda
export LDLIBS=$(install_path)/lib/ddq/*.a
LDFLAGS=$(LDLIBS) -lopenblasp64 -lm -pthread -lcudart -L/usr/local/cuda/lib64 -lcublas

CC=mpicc

EXEC = ddq_stencil

# 查找当前目录下的所有 .c 文件
SRCS = ddq_stencil.c
OBJS = $(SRCS:.c=.o)

all: clean $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) ${CFLAGS} ${LDFLAGS} -o $@

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