#!/bin/bash
# shell source for local environment variables

export CHROMA_PATH=${HOME}/local/usqcd
export CHROMA_BUILD_TYPE=Debug
export XML2_INC=/usr/include/libxml2  #/usr/local/libxml2/include/libxml2 
export DDQ_PATH=${HOME}/simdev/usqcd/ddq.git

export LD_LIBRARY_PATH=${CHROMA_PATH}/lib:$LD_LIBRARY_PATH
export PATH=${CHROMA_PATH}/bin:${PATH}

