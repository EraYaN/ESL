#include <stdio.h>

#define SIZE_DEFAULT 16 // This size is used when no argument is supplied

// Dynamically Allocated Matrix Init
void MatrixInit(uint32_t ***matrix, uint32_t size, uint32_t salt) {
    uint32_t i,j;
    *matrix = malloc(size * sizeof(**matrix));
    if (*matrix == NULL) {
        printf("ERROR: malloc() failed!\n");
        exit(-1);
    }
    for (i = 0; i < size; i++) {
        (*matrix)[i] = malloc(size * sizeof(*(*matrix)[i]));
        if ((*matrix)[i] == NULL) {
            printf("ERROR: malloc() failed!\n");
            exit(-1);
        }
        for (j = 0; j < size; j++) {
            (*matrix)[i][j] = i+j*salt;
        }
    }
}

// Dynamically Allocated Matrix print
void MatrixPrint(uint32_t ***matrix, uint32_t size) {
    uint32_t i,j;
    for(i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            // SYSTEM_1Print(" %d", matrix[i][j]);
            printf("\t%d", (*matrix)[i][j]);
        }
        // SYSTEM_0Print("\n");
        printf("\n");
    }
}

// Dynamically Allocated Matrix Multiply
void MatrixMultiply(uint32_t ***mat1, uint32_t ***mat2, uint32_t ***prod, uint32_t size) {
    int i, j, k;
    for (i = 0;i < size; i++) {
        for (j = 0; j < size; j++) {
            (*prod)[i][j]=0;
            for(k=0;k<size;k++) {
                (*prod)[i][j] = (*prod)[i][j]+(*mat1)[i][k] * (*mat2)[k][j];
            }
        }
    }
}
