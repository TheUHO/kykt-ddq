CFLAGS= -g -I../../../common -I../../../ddq -I../../../oplib -I../../../operators -O0 -g -Denable_processor_direct -Denable_processor_pthread -Denable_processor_call -Denable_processor_fork -lm
install_path=../../../compiled_cpu
export LDLIBS=$(install_path)/lib/ddq/*.a
LDFLAGS=$(LDLIBS) -ldl -lpthread

DDQ_RUN=./compiled_cpu/bin/ddqrun

SRC = main.c
TARGET = cnn_app

all: clean $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

run_ddq: $(TARGET)
	export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:~/hanbin/lib/ && yhrun -p thdsp -n 1 $(DDQ_RUN) cnn_parallel.ddq

run_gdb:
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/hanbin/lib/ && yhrun -p thdsp -n 1  gdb ./$(EXEC)

clean:
	rm -f $(TARGET)

.PHONY: all clean run run_ddq