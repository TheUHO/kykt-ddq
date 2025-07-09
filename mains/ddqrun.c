#include	"ddq.h"
#include	"ast.h"
#include	"passes.h"
#include	"oplib.h"

#ifdef	USE_MPI
#include	<mpi.h>
#endif

int	main(int argc, char* argv[])
{
	ddq_ring	ring;
	int		i;

	load_builtin_init();
	ddq_loop_init();

	for (i=1; i<argc; i++)
	{
		ddq_log("Start running %s ...\n", argv[i]);
		ring = ast2ring(file2ast(argv[i]));
		ddq_loop(ring, 0);

#ifdef	USE_MPI
		if (i+1 < argc)
			MPI_Barrier(MPI_COMM_WORLD);
#endif

		ddq_log("Finish running %s .\n", argv[i]);
	}

	cache_clear();
	load_builtin_finish();

	return	0;
}

