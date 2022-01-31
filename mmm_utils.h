#ifndef MMM_UTILS_H
#define MMM_UTILS_H

void print2d1d(unsigned int n);

void printMatrix(int* M, unsigned int n, int print_ij);

int* generateIdentity(unsigned int n);

int* generateOneToNN(unsigned int n);

int matrixEq(int* C, int* E, unsigned int n);

#endif
