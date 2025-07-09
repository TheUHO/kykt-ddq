
#ifdef enable_processor_direct
ddq_enable(processor_direct)
#endif

#ifdef enable_processor_pthread
ddq_enable(processor_pthread)
#endif

#ifdef enable_processor_pthread_pool
ddq_enable(processor_pthread_pool)
#endif

#ifdef enable_processor_pthread_mpi
ddq_enable(processor_pthread_mpi)
#endif

#ifdef enable_processor_pthread_qmp
ddq_enable(processor_pthread_qmp)
#endif

#ifdef enable_processor_fork
ddq_enable(processor_fork)
#endif

#ifdef enable_processor_tianhe
ddq_enable(processor_tianhe)
#endif

#ifdef	enable_processor_cuda
ddq_enable(processor_cuda)
#endif

#ifdef	enable_processor_call
ddq_enable(processor_call)
#endif

