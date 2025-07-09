#ifndef DAG_H
#define DAG_H
typedef struct {
    int from;
    int to;
} Edge;

typedef struct {
    int node_id;
    int input_count;
    int output_count;
    int* inputs;  // 输入节点列表
    int* outputs; // 输出节点列表
} NodeInfo;

NodeInfo* generate_random_dag(int num_nodes, int num_edges);
NodeInfo* generate_linear_dag(int num_nodes);
void free_node_info(NodeInfo* node_info, int num_nodes);
#endif