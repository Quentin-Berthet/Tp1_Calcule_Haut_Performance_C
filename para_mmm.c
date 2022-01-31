#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "mmm_utils.h"
#include "para_mmm.h"

#define INDEX_2D_TO_1D(y, x, nb_columns) ((y) * nb_columns + (x))

#define TAG_CHUNK_A 1
#define TAG_CHUNK_B 2
#define TAG_SHIFT_CHUNK_B 3
#define TAG_CHUNK_C 4

// implementation de la multiplication
// matrice-matrice carrée
int *matMatMult(const int *A, const int *B, const unsigned int n) {
    int *C = (int *) malloc(n * n * sizeof(int));
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            size_t l = i * n + j;
            C[l] = 0;
            for (size_t k = 0; k < n; k++) {
                C[l] += A[i * n + k] * B[k * n + j];
            }
        }
    }
    return C;
}

static void localMatMatMult(const int *chunkA, const int *chunkB, int *chunkC, const unsigned int chunkN) {
    for (size_t i = 0; i < chunkN; i++) { // row
        for (size_t j = 0; j < chunkN; j++) { // column
            size_t l = INDEX_2D_TO_1D(i, j, chunkN);
            for (size_t k = 0; k < chunkN; k++) {
                chunkC[l] += chunkA[INDEX_2D_TO_1D(i, k, chunkN)] * chunkB[INDEX_2D_TO_1D(k, j, chunkN)];
            }
        }
    }
}

static void createSubarrayType(MPI_Datatype *subarray_type, int n, int chunkN, int startX, int startY) {
    int dimensionsFullArray[2] = {n, n};
    int dimensionsSubarray[2] = {chunkN, chunkN};
    int startCoordinates[2] = {startY, startX};
    MPI_Type_create_subarray(2, dimensionsFullArray, dimensionsSubarray, startCoordinates, MPI_ORDER_C, MPI_INT,
                             subarray_type);
    MPI_Type_commit(subarray_type);
}

static int createGridCommunicators(MPI_Comm *cartComm, MPI_Comm *rowComm, MPI_Comm *colComm, int nProc) {
    int gridN = (int) sqrt(nProc);
    int dimensions[2] = {gridN, gridN};
    int periods[2] = {false, true}; // Cyclic on column for B matrix

    MPI_Cart_create(MPI_COMM_WORLD, 2, dimensions, periods, 1, cartComm);

    /* Create row communicator */
    int remainDims[2] = {false, true};
    MPI_Cart_sub(*cartComm, remainDims, rowComm);

    /* Create column communicator */
    remainDims[0] = true; // rows
    remainDims[1] = false; // columns
    MPI_Cart_sub(*cartComm, remainDims, colComm);
    return gridN;
}

static int *divideMatrix(int *A, int *B, unsigned int n, unsigned int chunkN, const int nProc) {
    int *indexes = calloc((size_t) nProc * 2, sizeof(int));
    for (int i = 0, y = 0, x = 0; i < nProc; ++i) {
        MPI_Datatype subarrayType;
        createSubarrayType(&subarrayType, (int) n, (int) chunkN, x, y);
        indexes[i * 2] = y;
        indexes[i * 2 + 1] = x;

        MPI_Send(A, 1, subarrayType, i, TAG_CHUNK_A, MPI_COMM_WORLD);
        MPI_Send(B, 1, subarrayType, i, TAG_CHUNK_B, MPI_COMM_WORLD);

        MPI_Type_free(&subarrayType);

        x += (int) chunkN;
        if (x >= (int) n) {
            x = 0;
            y += (int) chunkN;
        }
    }
    return indexes;
}

static void shiftBMatrix(int *chunkB, int chunkNN, const int coordinates[2], int gridN, MPI_Comm *colComm) {
    int source = (coordinates[0] + 1) % gridN;
    int dest = coordinates[0] - 1 < 0 ? gridN - 1 : coordinates[0] - 1;
    MPI_Sendrecv_replace(chunkB, chunkNN, MPI_INT, dest, TAG_SHIFT_CHUNK_B, source, TAG_SHIFT_CHUNK_B, *colComm,
                         MPI_STATUS_IGNORE);
}

static void
chunkCToMatrixC(int *C, unsigned int n, const int *chunkC, unsigned int chunkN, const int *indexes, int rank) {
    unsigned int y = (unsigned int) indexes[rank * 2];
    unsigned int x = (unsigned int) indexes[rank * 2 + 1];

    for (unsigned int i = 0; i < chunkN; ++i) {
        for (unsigned int j = 0; j < chunkN; ++j) {
            C[INDEX_2D_TO_1D(y + i, x + j, n)] = chunkC[INDEX_2D_TO_1D(i, j, chunkN)];
        }
    }
}

// cette fonction est le point d'entrée de votre
// algorithme parallèle
int *para_mmm(
        int *A,
        int *B,
        const unsigned int n,
        const int myRank,
        const int nProc,
        const int root
) {
    double start = MPI_Wtime();

    MPI_Comm cartComm, rowComm, colComm;
    int gridN = createGridCommunicators(&cartComm, &rowComm, &colComm, nProc);

    int myCartRank;
    MPI_Comm_rank(cartComm, &myCartRank);

    int coordinates[2] = {0};
    MPI_Cart_coords(cartComm, myCartRank, 2, coordinates);

    unsigned int chunkN = (unsigned int) (n / sqrt(nProc));
    int chunkNN = (int) (chunkN * chunkN);

    int *chunkA = calloc((size_t) chunkNN, sizeof(int));
    int *tmpA = calloc((size_t) chunkNN, sizeof(int));
    int *chunkB = calloc((size_t) chunkNN, sizeof(int));
    int *chunkC = calloc((size_t) chunkNN, sizeof(int));
    int *indexes = NULL;

    MPI_Request requests[2];
    MPI_Irecv(chunkA, chunkNN, MPI_INT, root, TAG_CHUNK_A, MPI_COMM_WORLD, &requests[0]);
    MPI_Irecv(chunkB, chunkNN, MPI_INT, root, TAG_CHUNK_B, MPI_COMM_WORLD, &requests[1]);

    if (myRank == root) {
        indexes = divideMatrix(A, B, n, chunkN, nProc);
    }

    MPI_Waitall(2, requests, MPI_STATUSES_IGNORE);

    int nbStages = gridN;
    int coordY = coordinates[0]; // row
    int coordX = coordinates[1]; // column
    for (int stage = 0; stage < nbStages; ++stage) {
        int sender = (coordY + stage) % nbStages;
        if (coordX == sender) {
            memcpy(tmpA, chunkA, (size_t) chunkNN * sizeof(int));
        }
        MPI_Bcast(tmpA, chunkNN, MPI_INT, sender, rowComm);
        localMatMatMult(tmpA, chunkB, chunkC, (unsigned int) chunkN);
        shiftBMatrix(chunkB, chunkNN, coordinates, gridN, &colComm);
    }

    int *C = NULL;
    if (myRank == root) {
        C = calloc(n * n, sizeof(int));
        chunkCToMatrixC(C, n, chunkC, chunkN, indexes, myRank);
        /* nProc - 1 because we have already treated root process */
        for (int k = 0; k < nProc - 1; ++k) {
            MPI_Status status;
            MPI_Recv(chunkC, chunkNN, MPI_INT, MPI_ANY_SOURCE, TAG_CHUNK_C, MPI_COMM_WORLD, &status);
            chunkCToMatrixC(C, n, chunkC, chunkN, indexes, status.MPI_SOURCE);
        }
        free(indexes);
    } else {
        MPI_Send(chunkC, chunkNN, MPI_INT, root, TAG_CHUNK_C, MPI_COMM_WORLD);
    }

    MPI_Comm_free(&cartComm);
    MPI_Comm_free(&rowComm);
    MPI_Comm_free(&colComm);

    free(chunkA);
    free(tmpA);
    free(chunkB);
    free(chunkC);
    double finish = MPI_Wtime();
    if (myRank == root) {
        printf("%d;%f\n", nProc, finish - start);
    }
    return C;
}
