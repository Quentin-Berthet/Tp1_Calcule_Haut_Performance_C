#include <stdio.h>
#include <stdlib.h>

#include "mmm_utils.h"

void print2d1d(unsigned int n) {
    for (unsigned int i = 0; i < n; i++) {
        for (unsigned int j = 0; j < n; j++) {
            size_t kc = i * n + j;
            printf("(i,j) %i, %i -> %li\n", i, j, kc);
            for (unsigned int k = 0; k < n; k++) {
                size_t ka = i * n + k;
                size_t kb = k * n + j;
                printf("  (i,k) %i, %i -> %li\n", i, k, ka);
                printf("  (k,j) %i, %i -> %li\n", k, j, kb);
            }
        }
    }
}

void printMatrix(int *M, unsigned int n, int print_ij) {
    for (unsigned int i = 0; i < n; i++) {
        for (unsigned int j = 0; j < n; j++) {
            size_t k = i * n + j;
            if (print_ij) {
                printf("%i (%i, %i) ", M[k], i, j);
            } else {
                printf("%i, ", M[k]);
            }
        }
        printf("\n");
    }
}

int *generateIdentity(unsigned int n) {
    int *I = (int *) malloc(n * n * sizeof(int));
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            size_t k = i * n + j;
            if (i == j) {
                I[k] = 1;
            } else {
                I[k] = 0;
            }
        }
    }
    return I;
}

int *generateOneToNN(unsigned int n) {
    int *M = (int *) malloc(n * n * sizeof(int));
    for (size_t k = 0; k < n * n; k++) {
        M[k] = (int) k;
    }
    return M;
}

int matrixEq(int *C, int *E, unsigned int n) {
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            size_t k = i * n + j;
            if (C[k] != E[k]) {
                return 0;
            }
        }
    }
    return 1;
}
