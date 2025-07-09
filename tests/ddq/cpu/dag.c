#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "dag.h"

// 生成随机DAG并存储节点信息
NodeInfo* generate_random_dag(int num_nodes, int num_edges) {
    if (num_edges > num_nodes * (num_nodes - 1) / 2) {
        printf("边数过多，无法生成DAG。\n");
        return NULL;
    }

    Edge* edges = (Edge*)malloc(num_edges * sizeof(Edge));
    int edge_count = 0;

    // 创建节点的随机排列，确保拓扑顺序
    int* nodes = (int*)malloc(num_nodes * sizeof(int));
    for (int i = 0; i < num_nodes; i++) {
        nodes[i] = i;
    }

    // 打乱节点顺序
    srand(time(NULL));
    for (int i = num_nodes - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = nodes[i];
        nodes[i] = nodes[j];
        nodes[j] = temp;
    }

     // 初始化节点信息
     NodeInfo* node_info = (NodeInfo*)malloc(num_nodes * sizeof(NodeInfo));
     for (int i = 0; i < num_nodes; i++) {
         node_info[i].node_id = i;
         node_info[i].input_count = 0;
         node_info[i].output_count = 0;
         node_info[i].inputs = NULL;  // 动态分配
         node_info[i].outputs = NULL; // 动态分配
     }
 
     // 按拓扑顺序生成边
     for (int i = 0; i < num_nodes; i++) {
         for (int j = i + 1; j < num_nodes; j++) {
             if (edge_count >= num_edges) break;
 
             // 随机决定是否添加边
             if (rand() % 2 == 0) {
                 edges[edge_count].from = nodes[i];
                 edges[edge_count].to = nodes[j];
                 edge_count++;
 
                 // 更新节点信息
                 int from = nodes[i];
                 int to = nodes[j];
 
                 // 动态调整输入和输出数组大小
                 node_info[from].outputs = (int*)realloc(node_info[from].outputs, (node_info[from].output_count + 1) * sizeof(int));
                 node_info[to].inputs = (int*)realloc(node_info[to].inputs, (node_info[to].input_count + 1) * sizeof(int));
 
                 node_info[from].outputs[node_info[from].output_count++] = to;
                 node_info[to].inputs[node_info[to].input_count++] = from;
             }
         }
         if (edge_count >= num_edges) break;
     }
 
     free(nodes);
     free(edges);
     return node_info;
 }

 // 生成线性结构的DAG并存储节点信息
NodeInfo* generate_linear_dag(int num_nodes) {
    if (num_nodes <= 0) {
        printf("节点数必须大于0。\n");
        return NULL;
    }

    // 初始化节点信息
    NodeInfo* node_info = (NodeInfo*)malloc(num_nodes * sizeof(NodeInfo));
    for (int i = 0; i < num_nodes; i++) {
        node_info[i].node_id = i;
        node_info[i].input_count = 0;
        node_info[i].output_count = 0;
        node_info[i].inputs = NULL;  // 动态分配
        node_info[i].outputs = NULL; // 动态分配
    }

    // 按线性结构生成边
    for (int i = 0; i < num_nodes - 1; i++) {
        int from = i;
        int to = i + 1;

        // 更新节点信息
        node_info[from].outputs = (int*)realloc(node_info[from].outputs, (node_info[from].output_count + 1) * sizeof(int));
        node_info[to].inputs = (int*)realloc(node_info[to].inputs, (node_info[to].input_count + 1) * sizeof(int));

        node_info[from].outputs[node_info[from].output_count++] = to;
        node_info[to].inputs[node_info[to].input_count++] = from;
    }

    return node_info;
}

 void free_node_info(NodeInfo* node_info, int num_nodes) {
    if (!node_info) return;

    for (int i = 0; i < num_nodes; i++) {
        free(node_info[i].inputs);
        free(node_info[i].outputs);
    }
    free(node_info);
}

// 打印节点信息
void print_node_info(NodeInfo* node_info, int num_nodes) {
    for (int i = 0; i < num_nodes; i++) {
        printf("Node %d:\n", node_info[i].node_id);
        printf("  Inputs (%d): ", node_info[i].input_count);
        for (int j = 0; j < node_info[i].input_count; j++) {
            printf("%d ", node_info[i].inputs[j]);
        }
        printf("\n");

        printf("  Outputs (%d): ", node_info[i].output_count);
        for (int j = 0; j < node_info[i].output_count; j++) {
            printf("%d ", node_info[i].outputs[j]);
        }
        printf("\n");
    }
}

// 测试生成DAG
int test_dag() {
    int num_nodes = 10;
    int num_edges = 15;

    NodeInfo* node_info = generate_random_dag(num_nodes, num_edges);

    if (node_info) {
        printf("生成的DAG节点信息：\n");
        print_node_info(node_info, num_nodes);

        // 释放内存
        for (int i = 0; i < num_nodes; i++) {
            free(node_info[i].inputs);
            free(node_info[i].outputs);
        }
        free(node_info);
    }

    return 0;
}

// int main() {
//     test_dag();
//     return 0;
// }