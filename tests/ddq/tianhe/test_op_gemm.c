#include	<stdio.h>
#include	<stdlib.h>
#include <time.h>
#include	<unistd.h>
#include <math.h>
#include <string.h>
#include	"ddq.h"
#include	"oplib.h"
#include	"mt_hybrid.h"
#include    "device/tianhe/host/types.h"
#include	"hthread_host.h"
#include "std/std_types/std_types.h"

void* mat_gen(int_t row_size, int_t col_size){
   double* res = hthread_malloc(0, sizeof(double) * row_size * col_size, HT_MEM_RW);

    // 生成随机数矩阵
    for(int i = 0; i < row_size; i++) {
        for(int j = 0; j < col_size; j++) {
            res[i*col_size + j] = (double)rand() / RAND_MAX * 10; // 生成0到1之间的随机数
        }
    }

    return res;
}

// 计算向量的模长
double vector_magnitude(double *vector, int size) {
    double magnitude = 0;
    for (int i = 0; i < size; i++) {
        magnitude += vector[i] * vector[i];
    }
    return sqrt(magnitude);
}

// 计算两个向量的点积
double dot_product(double *vectorA, double *vectorB, int size) {
    double dot = 0;
    for (int i = 0; i < size; i++) {
        dot += vectorA[i] * vectorB[i];
    }
    return dot;
}

// 计算余弦相似度
double cosine_similarity(double *vectorA, double *vectorB, int size) {
    double dot = dot_product(vectorA, vectorB, size);
    double magnitudeA = vector_magnitude(vectorA, size);
    double magnitudeB = vector_magnitude(vectorB, size);
	printf("magnitudeA %f, magnitudeB %f\n", magnitudeA,magnitudeB );
    return dot / (magnitudeA * magnitudeB);
}


int main(){
    // 初始化随机数生成器
    srand(time(NULL));

    load_tianhe_init();
    int Ma = 252;
    int Na = 48;
    int Ka = 512;
    int Ms = 6;
    double* A = mat_gen(Ma, Ka);
    double* B = mat_gen(Ka, Na);
    double* C = mat_gen(Ma, Na);
    double* C_ref = hthread_malloc(0, sizeof(double) * Ma * Na, HT_MEM_RW);
	memcpy(C_ref, C, Ma * Na  * sizeof(double));
    printf("before cal : C %f C_ref %f\n", C[0], C_ref[0]);
    printf("before cal : B \n");
    for(int i=0; i<48; i++){
        printf("%f ", B[i]);
    }
    printf("\n");
    printf("before cal : C \n");
    for(int i=0; i<48; i++){
        printf("%f ", C[i]);
    }
    printf("\n");

    unsigned long* args = hthread_malloc(0, sizeof(unsigned long) * 7 , HT_MEM_RW);
    args[0] = Ms;
    args[1] = Ma;
    args[2] = Na;
    args[3] = Ka;
    args[4] = A;
    args[5] = B;
    args[6] = C;

    int thread = hthread_group_create(0, 1, "test_op_gemm", 4, 3, args);
    hthread_group_wait(thread);


    for(int i=0; i<Ma; i++){
        for(int j=0; j<Na; j++){
            for(int k=0; k<Ka; k++){
                C_ref[i * Na + j] += A[i * Ka + k] * B[k * Na + j];
            }
        }
    }

    printf("similarity cal\n");
	double similarity = cosine_similarity(C, C_ref, Ma*Na);
    if(1 - similarity > 0.0001){
        printf("error!, similarity: %f %f %f\n", similarity, C[0], C_ref[0]);
        for(int i=0; i<Ma; i++){
            for(int j=0; j<Na; j++){
                printf("%d\t%d\t%f\t%f\n", i, j, C[i*Na+j], C_ref[i*Na+j]);
            }
        }
    }else{
		printf("success!, similarity: %f\n", similarity);
	}

    load_tianhe_finish();
    return 0;
}
