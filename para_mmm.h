#ifndef PARA_MMM_H
#define PARA_MMM_H

// implementation de la multiplication
// matrice-matrice carrée
int *matMatMult(const int *A, const int *B, const unsigned int n);

// cette fonction est le point d'entrée de votre
// algorithme parallèle
int *para_mmm(int *A, int *B, unsigned int n, const int myRank, const int nProc, const int root);

#endif
