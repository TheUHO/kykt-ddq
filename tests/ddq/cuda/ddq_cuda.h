// #include "dag.h"
// #include "matrix.h"
// #include "ddq_dag.h"
#ifdef __cplusplus
extern "C" {
#endif

ddq_ring generate_ddq_from_dag_cuda(int num_nodes, const NodeInfo* nodes, double** matrices);
#ifdef __cplusplus
}
#endif