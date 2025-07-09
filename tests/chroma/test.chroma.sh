export LD_LIBRARY_PATH=/home/gongming/软件/usqcd/lib
#export OMP_NUM_THREADS=1

mpirun -np 4 ddqrun chroma.init.ddq chroma.initRNG.ddq test.chroma.ddq chroma.finish.ddq

