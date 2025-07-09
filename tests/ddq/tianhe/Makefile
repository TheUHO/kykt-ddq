M3000_ENV=/thfs1/software/programming_env/mt3000_programming_env/mt3000_programming_env_20231227/
HTHREAD_HOME=$(M3000_ENV)/hthreads
LIBMT_HOME=${M3000_ENV}/libmt
CFLAGS= -g -I../../../common -I../../../ddq -I../../../oplib -I../../../operators -I$(HTHREAD_HOME)/include -O0 -g -I${LIBMT_HOME}/include -Denable_processor_direct -Denable_processor_pthread -Denable_processor_tianhe -Denable_processor_call -Denable_processor_fork -Doplib_load_tianhe -Doplib_load_so -lm
export LD_LIBRARY_PATH=${M3000_ENV}/third-party-lib:$LD_LIBRARY_PATH 
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/hanbin/lib/
install_path=../../../compiled_tianhe3
export LDLIBS=$(install_path)/lib/ddq/*.a
LDFLAGS= $(LDLIBS) -L/thfs1/home/lzz/hanbin/lib $(LIBMT_HOME)/lib/libmt.a -ldl -lhthread_host -lpthread

EXEC = test_matmul_multicore_multibuffer

all: clean $(EXEC)

%	:	%.c	
	-$(RM) $@
	$(CC) $< ${CFLAGS} ${LDFLAGS} -o $@

run:
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/hanbin/lib/ && yhrun -p thmt1 -n 1 -xcn[11522,11528,11572,11913,12086,12092,12037,12038,11760] $(EXEC)

run_log:
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/hanbin/lib/ && yhrun -p thmt1 -n 1  $(EXEC) &> txt_multibuffer

run_gdb:
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/hanbin/lib/ && yhrun -p thmt1 -n 1  gdb $(EXEC)

clean : 
	$(RM) $(EXEC)

.PHONY	: all clean