# M3000_ENV=/thfs1/software/programming_env/mt3000_programming_env/mt3000_programming_env_20231227/
# HTHREAD_HOME=$(M3000_ENV)/hthreads
# LIBMT_HOME=${M3000_ENV}/libmt
CFLAGS= -g -I../../../common -I../../../ddq -I../../../oplib -I../../../operators -O0 -g -Denable_processor_direct -Denable_processor_pthread -Denable_processor_call -Denable_processor_fork -lm
# export LD_LIBRARY_PATH=${M3000_ENV}/third-party-lib:$LD_LIBRARY_PATH 
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/hanbin/lib/
install_path=../../../compiled_cpu
export LDLIBS=$(install_path)/lib/ddq/*.a
LDFLAGS= $(LDLIBS) -ldl -lpthread

EXEC = test

all: clean $(EXEC)

%	:	%.c	
	-$(RM) $@
	$(CC) $< ${CFLAGS} ${LDFLAGS} -o $@

run:
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/hanbin/lib/ && yhrun -p thdsp -n 1 ./$(EXEC)

run_log:
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/hanbin/lib/ && yhrun -p thdsp -n 1  ./$(EXEC) &> txt_modify_loop

run_gdb:
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/hanbin/lib/ && yhrun -p thdsp -n 1  gdb ./$(EXEC)

clean : 
	$(RM) $(EXEC)

.PHONY	: all clean