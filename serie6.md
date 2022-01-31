# Multiplication matrice-matrice en parallèle

Pour ce travail **évalué** vous devez implémenter la multiplication matrice-matrice carrées en parallèle.
L'algorithme est à choix, mais vous devez choisir entre l'agorithme de Cannon et de Fox.
Les deux algorithmes présentants de nombreuses similarités, aucun n'est vraiment plus difficile ou facile que l'autre.

Ces deux algorithmes ont été présenté en cours et des resources vous ont été distibuées [ici](https://gitedu.hesge.ch/hpc_etu/cours_2021).
Les noms des fichiers d'importance étant cette fois assez explicites.

Ce travail est à rendre pour le **vendredi de la dernière semaine de cours au plus tard à 23:59**, à savoir le vendredi 23 avril 2021.
Le dernier commit avant 23:59 se pris en compte, les commits suivant seront ignorés.

## Travail à accomplir

Votre travail doit être réalisé en C et utiliser la librairie OpenMPI pour paralléliser votre algorithme.
L'algorithme multipliera des matrices **d'entiers**, ceci dans le but de faciliter la comparaison des résultats des fonctions de tests.

Vous baserez votre travail sur une [structure qui vous est fournie ici](https://gitedu.hesge.ch/hpc_etu/serie_2021/-/tree/master/serie_6/files).
Lisez attentivement ce code avant de commencer.
Ce code compile et peut être exécuter sur Baobab.
Il est évident que l'exemple proposé n'est pas une solution et ne constitue pas une implémentation parallèle valide.

### Code et point d'entrée

Le point d'entrée de votre travail se situe dans du fichier `para_mmm.c` et de son header `para_mmm.h`, il s'agit de la fonction:
```
int* para_mmm(
    int* A, 
    int* B, 
    unsigned int n, 
    const int myRank, 
    const int nProc, 
    const int root
);
```
pour calculer le produit matricielle `C = A x B`,
où les arguments correspondent dans l'ordre:
- la matrice `A` de taille `n x n`,
- la matrice `B` de taille `n x n`,
- le nombre de lignes (ou colonnes) des matrices,
- le rang du processus,
- le nombre de rangs dans le communicateur `WORLD`,
- le rang du processus racine.

Normalement vous devriez pouvoir travailler **sans modifier** la signature `para_mmm`.
Ne modifiez pas la signature de cette fonction sans m'avoir contacté et fourni une excellente raison pour le faire.
L'argument du "mais c'est plus simple pour moi" n'est pas un argument valide.

Vous remarquerez que le point d'entrée du programme vous est fourni, il s'agit de `mmm.c`.
Ce dernier contient deux tests basiques pour tester votre implémentation.
Il est important de noter que je me servirai de ce point d'entrée dans l'état pour tester votre code.
Modulo quelques modifications de taille de matrice ou permutation d'arguments.
Pour ces raisons ne changez pas non plus le nom des fichiers fournis.

Vous pouvez ajouter autant de fonctions que vous le désirez dans le code fourni, à l'exception du point d'entrée du programme et tant que vous ne changez pas la fonction qui fait office de point d'entrée de votre travail.

Vous fournirez également un `Makefile` qui contiendra les trois cibles suivantes:
- clean, qui efface les fichiers issue de la compilation et de l'édition de liens,
- build, qui produit un executable nommé mmm.out,
- all, qui appelle successivement clean et build.

Un `Makefile` vous est également fourni comme aide de départ si nécessaire.

### Mesure de performances

Une fois votre programme terminé et testé vous procéderez à des mesures de performance sur Baobab.
On vous laisse juger ce qui est pertinent comme tests, comme mesures et la manière de les présenter.
Mais vous devrez fournir ces mesures et une brève discussion sur les performance de votre code.

Prenez des matrices suffisamment grandes: jetez un oeil à la mémoire disponible par noeud et n'hésitez pas à la remplire.
Il faut que le caclul prenne assez de temps pour que les mesures fassent du sens.

## Remarques sur MPI

On a vu en cours que les processus étaient identifiés par des coordonnées correspondant à celle d'une grille carthésienne.
Il est intéressant de noter que MPI propose des fonctions pour passer d'une identification des rangs "linéaire" à une identification des rangs sur une grille carthésienne de dimension `N`.
Pour accomplir ceci veuillez lire la documentation des fonctions suivantes ([RTFM](https://en.wikipedia.org/wiki/RTFM)):
- MPI_Dims_create
- MPI_Cart_create
- MPI_Cart_coords
- MPI_Cart_rank

dans la [documentation OpenMPI](https://www.open-mpi.org/doc/v4.1/).
Soyez également un peu curieux et jetez un coup d'oeil à la liste des fonctions, certains noms de fonctions peuvent vous inspirer.

Je vous donne également un code qui consrtuit des rangs carthésiens.
Ce code compile et s'exécute sur Baobab sans problème. 
Lisez-le attentivement avec la documentation des nouvelles fonctions utilisées:

```c
#include <stdio.h>
#include <math.h>

#include "mpi.h"

int main(int argc, char *argv[]) { 
  int nProc;
  int myRank;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nProc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  
  int q = (int) sqrt(nProc);
  if (q*q != nProc) {
    printf("Nb proc is not a square: %i", nProc);
  }
  else {
    const int nDims = 2;
    const int nrows = q; 
    const int ncols = q;
    
    int dims[nDims];
    dims[0] = 0;
    dims[1] = 0;
      
    MPI_Dims_create(nProc, nDims, dims);
    
    if (myRank == 0) {
      printf("Rank %i/%i. Grid: [%d x %d]\n", myRank, nProc, dims[0], dims[1]);
    }
      
    int periods[nDims];
    periods[0] = 0;
    periods[1] = 0;
    int reorder = 1;
    MPI_Comm comm2D;
    int ierr = MPI_Cart_create(MPI_COMM_WORLD, nDims, dims, periods, reorder, &comm2D);    
    if (ierr != 0) {
      printf("Error %d while creating Cart\n", ierr);
    }
    
    int gridRank;
    int coord[nDims];
        
    MPI_Cart_coords(comm2D, myRank, nDims, coord);
    MPI_Cart_rank(comm2D, coord, &gridRank);
    printf(
      "Rank %i/%i. Lin. rank: %i. Grid rank: (%i,%i)\n", 
      myRank, 
      nProc, 
      gridRank, 
      coord[0], 
      coord[1]
    );
    
    MPI_Comm_free(&comm2D);
  }
  
  MPI_Finalize();
  
  return 0;
}
```

## Remarques sur l'évaluation

Prenez note que ce travail est **individuel**:
- Un code qui ne compile pas, n'est pas corrigé et est sanctionné par la note minimale de 1.
- Un code qui ne s'exécute pas, n'est pas corrigé et est sanctionné par la note minimale de 1.
- Sans mesures de performance, une présentation et discussion adéquate de vos performance, vous ne pouvez pas obtenir la moyenne.
- Vous pouvez supposer que le nombre de processus demandé sera toujours adéquate lors de l'évaluation.

Votre code sera compilé avec le point d'entrée tel qu'il a été fourni.
Lors de l'évaluation, le module suivant sera chargé sur Baobab avant la compilation:
```
$ module load foss
```
et la compilation sera faite à l'aide de `make` et du `Makefile` que vous fournirez.
Cela veut dire que si vous changez les signatures, les noms de fichier ou encore ne suivez pas les instructions des cible de votre `Makefile`, on considère que votre code ne compile pas.
Cela veut également dire que si vous changez le nom de l'exécutable produit, on considère que votre code ne s'exécute pas.

Vous déposerez votre code dans [ce dépôt dédié](https://gitedu.hesge.ch/hpc_etu_2021/serie_6_rendu) dans un répertoire composé de :
- votre nom et votre prénom,
- sans espace, remplacé par des `_` (underscore),
- sans accents, utilisez donc la version non accentuée de votre nom ou prénom.

On en profite pour rappeler que Git n'efface pas les fichiers de vos camarades parce qu'ils ont le même nom dans des répertoires différents.
Si c'est le cas, c'est très probablement que vous avez récupré le code d'un de vos camarades en faisant un "cut and paste" au lieu d'un "copy and paste".
Et ensuite vous avez commité toutes les modifications sans comprendre ce que vous faites.

