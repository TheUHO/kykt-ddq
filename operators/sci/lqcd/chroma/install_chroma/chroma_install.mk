# -*- Makefile -*-

CHROMA_PATH?=${HOME}/local/usqcd
DDQ_PATH?=${HOME}/local/ddq
CHROMA_BUILD_TYPE?=Debug

all:install-qmp install-qdpxx install-chroma

qmp:
	git clone -b devel --recursive https://github.com/usqcd-software/qmp qmp

qdpxx:
	git clone -b devel --recursive https://github.com/usqcd-software/qdpxx qdpxx

chroma:
	git clone -b devel --recursive https://github.com/JeffersonLab/chroma chroma

patch: qmp qdpxx chroma
	cd qmp && git apply ${DDQ_PATH}/operators/sci/lqcd/chroma/install_chroma/qmp_QMP_init.patch || true
	cd qdpxx && git apply ${DDQ_PATH}/operators/sci/lqcd/chroma/install_chroma/qdp_default_allocator.patch || true
	cd qdpxx && git apply ${DDQ_PATH}/operators/sci/lqcd/chroma/install_chroma/qdp_avoid_tests.patch || true
	cd qdpxx/other_libs/qio && git apply ${DDQ_PATH}/operators/sci/lqcd/chroma/install_chroma/qio_avoid_tests.patch || true
	cd chroma && git apply ${DDQ_PATH}/operators/sci/lqcd/chroma/install_chroma/chroma_avoid_mains.patch || true
	touch patch

install-qmp:qmp patch
	mkdir -p build/qmp && cd build/qmp && rm -rf *
	cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=${CHROMA_BUILD_TYPE} \
		-DQMP_MPI=ON -DCMAKE_INSTALL_PREFIX=${CHROMA_PATH} -B build/qmp -S qmp
	cmake --build build/qmp -j4
	cmake --install build/qmp

install-qdpxx:qdpxx patch install-qmp
	mkdir -p build/qdpxx && cd build/qdpxx && rm -rf *
	cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=${CHROMA_BUILD_TYPE} \
            -DCMAKE_INSTALL_PREFIX=${CHROMA_PATH} -B build/qdpxx -S qdpxx #QDP_USE_OPENMP=ON
	cmake --build build/qdpxx -j4
	cmake --install build/qdpxx

install-chroma:chroma patch install-qdpxx
	mkdir -p build/chroma && cd build/chroma && rm -rf *
	cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=${CHROMA_BUILD_TYPE} \
            -DChroma_ENABLE_OPENMP=ON -DCMAKE_INSTALL_PREFIX=${CHROMA_PATH} -B build/chroma -S chroma
	cmake --build build/chroma -j4
	cmake --install build/chroma

cont: chroma
	cmake --build build/chroma --config ${CHROMA_BUILD_TYPE} #--verbose
	cmake --install build/chroma

cont-qdpxx: qdpxx
	cmake --build build/qdpxx --config ${CHROMA_BUILD_TYPE} #--verbose
	cmake --install build/qdpxx

