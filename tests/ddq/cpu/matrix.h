#ifndef MATRIX_H
#define MATRIX_H
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include <string.h>
#include <stdbool.h>

double** generate_matrix_array(int num_matrices, int rows, int cols);
void free_matrix_array(double** matrices, int num_matrices);
void clear_cpu_cache();
void write_matrix_array_to_file(const char* filename, double** matrices, int num_matrices, int rows, int cols);
double** read_matrix_array_from_file(const char* filename, int* num_matrices, int* rows, int* cols);
bool compare_matrix_arrays(double** array1, double** array2, int num_matrices, int rows, int cols);
#endif