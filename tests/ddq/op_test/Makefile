CFLAGS=-O0 -g -I../../../common -I../../../ -I../../../ddq -I../../../oplib -I../../../operators -I/usr/include/openblas -g -Denable_processor_direct -Denable_processor_pthread -Denable_processor_call -Denable_processor_fork -Denable_processor_pthread_pool

install_path=../../../compiled_cpu
export LDLIBS=$(install_path)/lib/ddq/*.a
LDFLAGS= $(LDLIBS) -ldl -lpthread -lopenblasp64 -lm

EXEC = buffer_test

# 查找当前目录下的所有 .c 文件
SRCS = $(wildcard *.c)
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