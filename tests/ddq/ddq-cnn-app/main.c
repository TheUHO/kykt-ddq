#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ddq.h"
#include "oplib.h"
#include "error.h"

// --------------------- Matrix 数据结构 ---------------------
typedef struct {
    float *data;
    int rows;
    int cols;
} Matrix;

// 全局变量保存最终输出
static obj g_fc_out = NULL;

// 动态输入尺寸（全局变量从输入文件中读取）
int input_height;
int input_width;
int kernel_rows;
int kernel_cols;
// 归一化因子
#define NORMALIZATION_FACTOR 255.0f

#define POOL_SIZE         2

// 根据输入尺寸计算卷积、池化层输出尺寸，注意后续层尺寸依赖前一层输出
// 对于全连接层，采用展平后的尺寸计算
// TOTAL_LAYERS 在这里不再作为内部计数，而是由外部指定
       
// --------------------- Matrix 操作 ---------------------
Matrix* new_matrix(int rows, int cols) {
    printf("[new_matrix] allocating %dx%d matrix\n", rows, cols);
    Matrix *m = (Matrix*) malloc(sizeof(Matrix));
    if (!m) {
        fprintf(stderr, "[new_matrix] Error allocating Matrix struct\n");
        exit(EXIT_FAILURE);
    }
    m->rows = rows;
    m->cols = cols;
    m->data = (float*) calloc(rows * cols, sizeof(float));
    if (!m->data) {
        fprintf(stderr, "[new_matrix] Error allocating Matrix data\n");
        free(m);
        exit(EXIT_FAILURE);
    }
    // printf("[new_matrix] done\n");
    return m;
}

/**
 * 从文件读取输入数据，并进行归一化处理
 */
Matrix* read_and_normalize_input(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "无法打开输入文件：%s\n", filename);
        exit(EXIT_FAILURE);
    }
    if (fscanf(file, "%d %d", &input_height, &input_width) != 2) {
        fprintf(stderr, "输入文件格式错误，需第一行为尺寸\n");
        exit(EXIT_FAILURE);
    }
    Matrix* input = new_matrix(input_height, input_width);
    int total_elements = input->rows * input->cols;
    for (int i = 0; i < total_elements; i++) {
        if (fscanf(file, "%f", &input->data[i]) != 1) {
            fprintf(stderr, "读取输入数据时出错\n");
            exit(EXIT_FAILURE);
        }
    }
    fclose(file);
    // 归一化
    float max_val = 0.0f;
    for (int i = 0; i < total_elements; i++) {
        if (input->data[i] > max_val) {
            max_val = input->data[i];
        }
    }
    if (max_val != 0.0f) {
        for (int i = 0; i < total_elements; i++) {
            input->data[i] /= max_val;
        }
    }
    return input;
}

/**
 * 从文件读取卷积核
 */
Matrix* read_conv_kernel(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "无法打开卷积核文件：%s\n", filename);
        exit(EXIT_FAILURE);
    }
    if (fscanf(file, "%d %d", &kernel_rows, &kernel_cols) != 2) {
        fprintf(stderr, "卷积核文件格式错误，第一行应为两个整数\n");
        exit(EXIT_FAILURE);
    }
    Matrix* kernel = new_matrix(kernel_rows, kernel_cols);
    int total = kernel_rows * kernel_cols;
    for (int i = 0; i < total; i++){
        if (fscanf(file, "%f", &kernel->data[i]) != 1) {
            fprintf(stderr, "读取卷积核文件时出错\n");
            exit(EXIT_FAILURE);
        }
    }
    fclose(file);
    return kernel;
}

void free_matrix(Matrix *m) {
    if(m) {
        printf("[free_matrix] freeing matrix %dx%d\n", m->rows, m->cols);
        free(m->data);
        free(m);
        // printf("[free_matrix] done\n");
    }
}

// --------------------- CNN 算子实现 ---------------------

task_ret cnn_conv(void **inputs, void **outputs, void **attribute) {
    Matrix *in = (Matrix*) inputs[0];
    Matrix *kernel = (Matrix*) inputs[1];
    Matrix *out = (Matrix*) outputs[0];
    int conv_rows = in->rows - kernel->rows + 1;
    int conv_cols = in->cols - kernel->cols + 1;
    out->rows = conv_rows;
    out->cols = conv_cols;
    printf("[cnn_conv] input: %dx%d, kernel: %dx%d, conv output: %dx%d\n",
           in->rows, in->cols, kernel->rows, kernel->cols, conv_rows, conv_cols);
    for (int i = 0; i < conv_rows; i++) {
        for (int j = 0; j < conv_cols; j++) {
            float sum = 0.0f;
            for (int ki = 0; ki < kernel->rows; ki++) {
                for (int kj = 0; kj < kernel->cols; kj++) {
                    sum += in->data[(i+ki)*in->cols + (j+kj)] *
                           kernel->data[ki*kernel->cols + kj];
                }
            }
            out->data[i*out->cols+j] = sum;
        }
    }
    // printf("[cnn_conv] done\n");
    return task_ret_ok;
}

task_ret cnn_relu(void **inputs, void **outputs, void **attribute) {
    Matrix *in = (Matrix*) inputs[0];
    Matrix *out = (Matrix*) outputs[0];
    int total = in->rows * in->cols;
    printf("[cnn_relu] processing %d elements\n", total);
    for (int i = 0; i < total; i++){
        float val = in->data[i];
        out->data[i] = (val > 0.0f ? val : 0.0f);
    }
    // printf("[cnn_relu] done\n");
    return task_ret_ok;
}

task_ret cnn_pool(void **inputs, void **outputs, void **attribute) {
    Matrix *in = (Matrix*) inputs[0];
    Matrix *out = (Matrix*) outputs[0];
    int pool_rows = in->rows / POOL_SIZE;
    int pool_cols = in->cols / POOL_SIZE;
    out->rows = pool_rows;
    out->cols = pool_cols;
    printf("[cnn_pool] input: %dx%d, pool output: %dx%d\n", in->rows, in->cols, pool_rows, pool_cols);
    for (int i = 0; i < pool_rows; i++) {
        for (int j = 0; j < pool_cols; j++) {
            float max_val = -1e10;
            for (int pi = 0; pi < POOL_SIZE; pi++) {
                for (int pj = 0; pj < POOL_SIZE; pj++) {
                    int row = i * POOL_SIZE + pi;
                    int col = j * POOL_SIZE + pj;
                    float val = in->data[row * in->cols + col];
                    if (val > max_val) {
                        max_val = val;
                    }
                }
            }
            out->data[i*out->cols+j] = max_val;
        }
    }
    // printf("[cnn_pool] done\n");
    return task_ret_ok;
}

task_ret cnn_fc(void **inputs, void **outputs, void **attribute) {
    Matrix *in = (Matrix*) inputs[0];
    Matrix *fc_out = (Matrix*) outputs[0];
    int total = in->rows * in->cols;
    fc_out->rows = 1;
    fc_out->cols = total;
    printf("[cnn_fc] flattening input (%dx%d) to fc output (1x%d)\n", in->rows, in->cols, total);
    for (int i = 0; i < total; i++) {
        fc_out->data[i] = in->data[i];
    }
    // printf("[cnn_fc] done\n");
    return task_ret_ok;
}

task_ret f_print_matrix(void **inputs, void **outputs, void **attribute) {
    Matrix *m = (Matrix*) inputs[0];
    if (!m) {
        printf("[f_print_matrix] 输入矩阵为空\n");
        return task_ret_done;
    }
    printf("---- 打印矩阵 (%dx%d) ----\n", m->rows, m->cols);
    for (int i = 0; i < m->rows; i++){
        for (int j = 0; j < m->cols; j++){
            printf("%6.2f ", m->data[i*m->cols+j]);
        }
        printf("\n");
    }
    printf("---- 打印结束 ----\n");
    return task_ret_done;
}

/*
 * 修改后的 cnn_ring 函数：
 * 参数 num_layers 指定串联的 conv→relu→pool 层数，
 * 最终 fc 采用最后一层 pool 的输出。
 */
ddq_ring cnn_ring(Matrix* input, Matrix* conv_kernel, int num_layers) {
    ddq_ring ring = ddq_new(NULL, 2, 0);
    // 将初始输入封装
    obj cur_input = obj_import(ring, input, (destruct_f*)free_matrix, obj_prop_ready);
    
    ring->inputs[0] = cur_input; // 输入矩阵
    // 将卷积核封装
    ring->inputs[1] = obj_import(ring, conv_kernel, (destruct_f*)free_matrix, obj_prop_ready);
    
    // 循环构建串联层，每一层包括conv、relu、pool
    int cur_rows = input->rows, cur_cols = input->cols;
    obj conv_out = NULL, relu_out = NULL, pool_out = NULL;
    for (int i = 0; i < num_layers; i++) {
        // 计算 conv 输出尺寸
        int conv_rows = cur_rows - kernel_rows + 1;
        int conv_cols = cur_cols - kernel_cols + 1;
        conv_out = obj_import(ring, (obj)new_matrix(conv_rows, conv_cols),
            (destruct_f*)free_matrix, obj_prop_consumable);
        ddq_op t_conv = ddq_spawn(ring, processor_pthread, 2, 1);
        obj cnn_conv_obj = obj_import(ring, cnn_conv, NULL, obj_prop_ready);
        ddq_add_f(t_conv, cnn_conv_obj);
        ddq_add_inputs(t_conv, 0, cur_input);
        ddq_add_inputs(t_conv, 1, ring->inputs[1]); // 共用卷积核
        ddq_add_outputs(t_conv, 0, conv_out);
        
        // 激活层：尺寸与 conv 相同
        relu_out = obj_import(ring, (obj)new_matrix(conv_rows, conv_cols),
            (destruct_f*)free_matrix, obj_prop_consumable);
        ddq_op t_relu = ddq_spawn(ring, processor_pthread, 1, 1);
        obj cnn_relu_obj = obj_import(ring, cnn_relu, NULL, obj_prop_ready);
        ddq_add_f(t_relu, cnn_relu_obj);
        ddq_add_inputs(t_relu, 0, conv_out);
        ddq_add_outputs(t_relu, 0, relu_out);
        
        // 池化层：输出尺寸为 conv 输出尺寸 / POOL_SIZE
        int pool_rows = conv_rows / POOL_SIZE;
        int pool_cols = conv_cols / POOL_SIZE;
        pool_out = obj_import(ring, (obj)new_matrix(pool_rows, pool_cols),
            (destruct_f*)free_matrix, obj_prop_consumable);
        ddq_op t_pool = ddq_spawn(ring, processor_pthread, 1, 1);
        obj cnn_pool_obj = obj_import(ring, cnn_pool, NULL, obj_prop_ready);
        ddq_add_f(t_pool, cnn_pool_obj);
        ddq_add_inputs(t_pool, 0, relu_out);
        ddq_add_outputs(t_pool, 0, pool_out);
        
        // 更新当前输入和尺寸，供下一层使用
        cur_input = pool_out;
        cur_rows = pool_rows;
        cur_cols = pool_cols;
    }
    
    // 全连接层：将最后一层 pool 的输出展平
    g_fc_out = obj_import(ring, (obj)new_matrix(1, cur_rows*cur_cols),
                           (destruct_f*)free_matrix, obj_prop_consumable);
    ddq_op t_fc = ddq_spawn(ring, processor_pthread, 1, 1);
    obj cnn_fc_obj = obj_import(ring, cnn_fc, NULL, obj_prop_ready);
    ddq_add_f(t_fc, cnn_fc_obj);
    ddq_add_inputs(t_fc, 0, cur_input);
    ddq_add_outputs(t_fc, 0, g_fc_out);
    
    // 打印算子
    ddq_op t_print = ddq_spawn(ring, processor_pthread, 1, 0);
    obj print_matrix_obj = obj_import(ring, f_print_matrix, NULL, obj_prop_ready);
    ddq_add_f(t_print, print_matrix_obj);
    ddq_add_inputs(t_print, 0, g_fc_out);
    
    return ring;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("使用方法：./cnn_app <input_file_path> <conv_kernel_file_path> <num_layers>\n");
        return EXIT_FAILURE;
    }
    int num_layers = atoi(argv[3]);
    if (num_layers <= 0) {
        fprintf(stderr, "num_layers 必须大于0\n");
        return EXIT_FAILURE;
    }
    
    printf("Starting CNN DDQ execution...\n");
    
    Matrix* input = read_and_normalize_input(argv[1]);
    Matrix* conv_kernel = read_conv_kernel(argv[2]);
    
    ddq_ring ring = cnn_ring(input, conv_kernel, num_layers);
    printf("Calling ddq_update\n");
    ddq_update(ring);
    printf("Entering ddq_loop\n");
    ddq_loop(ring, 0);
    printf("CNN computation completed.\n");
    
    ddq_delete(ring);
    printf("Resources cleaned up, exiting.\n");
    
    return 0;
}