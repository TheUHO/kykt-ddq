#ifndef DDQ_DAG_H
#define DDQ_DAG_H
#include "dag.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>

#include	"ddq.h"
#include	"oplib.h"
#include    "error.h"

#include    "std/std_ops/std_ops.h"
#include    "dag.h"
#include <stdbool.h>


ddq_ring generate_ddq_from_dag_raw(int num_nodes, NodeInfo* nodes);
ddq_ring generate_ddq_from_dag_optimized(int num_nodes,const NodeInfo* nodes, double **matrixs);
ddq_ring generate_ddq_from_dag_pthread_pool(int num_nodes,const NodeInfo* nodes, double **matrixs);
#endif