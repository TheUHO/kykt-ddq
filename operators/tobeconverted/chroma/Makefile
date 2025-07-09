# export LD_LIBRARY_PATH=$HOME/Chroma/local/lib:$LD_LIBRARY_PATH
DDQ_PATH ?=${HOME}//ddq
CHROMA_PATH?=${HOME}/软件/usqcd
XML2_INC ?=/usr/include/libxml2  #/usr/local/libxml2/include/libxml2 

DDQ_OP_PATH=${DDQ_PATH}/compiled/operators_cpu_qmp

CFLAGS=-g -pthread -I${DDQ_PATH}/ddq -I${DDQ_PATH}/oplib -I${DDQ_PATH}/common -I${CHROMA_PATH}/include -I${XML2_INC}
LDFLAGS=-L${CHROMA_PATH}/lib -lxmlWriter -lqdp_lapack -lfiledb -lfilehash -lXPathReader -llime -lchromalib -lqdp -lqio -lqmp -lxml2 ${DDQ_PATH}/compiled/ddq_cpu_qmp.a  

vpath %.so ${DDQ_OP_PATH}

all: test_chroma_operators test_op_chroma op_chroma.so 

test_chroma_operators: test_chroma_operators.cpp chroma_operators.cpp
	g++ $^  ${CFLAGS} -o $@ ${LDFLAGS} -ldl

op_chroma.so: op_chroma.cpp chroma_operators.cpp
	g++ $^ -fPIC -shared  $(CFLAGS) -o ${DDQ_OP_PATH}/$@ ${LDFLAGS} 

test_op_chroma: test_op_chroma.c op_chroma.so 
	mpicc test_op_chroma.c $(CFLAGS) -Denable_processor_pthread -Denable_processor_pthread_qmp -Denable_processor_direct \
	-o $@ $(LDFLAGS) ${DDQ_OP_PATH}/op_chroma.so -ldl -lpthread 

test: test_op_chroma  test_chroma_operators
	#LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${CHROMA_PATH}/lib ./test_chroma_operators
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${CHROMA_PATH}/lib  ./test_op_chroma

test_mpi: test_op_chroma  test_chroma_operators
	#LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${CHROMA_PATH}/lib mpirun -np 4 ./test_chroma_operators -geom 1 1 2 2
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${CHROMA_PATH}/lib  mpirun -np 2 ./test_op_chroma  -geom 1 1 1 2

clean:
	rm -f test_op_chroma test_chroma_operators ${DDQ_OP_PATH}/op_chroma.so

