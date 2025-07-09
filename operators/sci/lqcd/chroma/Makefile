
CHROMA_PATH ?= ${HOME}/软件/usqcd
XML2_INC ?= /usr/include/libxml2  #/usr/local/libxml2/include/libxml
CC = g++
#LDFLAGS=-L${CHROMA_PATH}/lib -lxmlWriter -lqdp_lapack -lfiledb -lfilehash -lXPathReader -llime -lchromalib -lqdp -lqio -lqmp -lxml2

subdir = xml gauge ferm corr

all	:	op_chroma.so

op_chroma.so	:	CFLAGS += -pthread -I${CHROMA_PATH}/include -I${XML2_INC}

op_chroma.so	:	src/op_chroma.cpp src/chroma_operators.cpp
	$(CC) $^ $(CFLAGS) -fPIC -shared -I../../../../ddq -o $@ -L${CHROMA_PATH}/lib -lxmlWriter -lqdp_lapack -lfiledb -lfilehash -lXPathReader -llime -lchromalib -lqdp -lqio -lqmp -lxml2

install	:
	mkdir -p $(install_path)
	-cp -r *.ddq *.so $(subdir) $(install_path)/ 2>/dev/null || :

clean	:
	$(RM) *.so

.PHONY	:	all install clean

