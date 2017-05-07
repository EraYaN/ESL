#include <stdio.h>
#include <stdint.h>
#include <Timer.h>
#include <arm_neon.h>
#include <util.h>
#include "matrix.h"

void matrixMultiplyNEON(uint32_t **mat1, uint32_t **mat2, uint32_t **prod, uint32_t size) {
    int i, j, k;
    uint32_t c;
    uint32x4_t mat_vec, prod_vec;

    for (j = 0; j < size; j += 4) {
        for (i = 0; i < size; i++) {
            prod_vec = vdupq_n_u32(0);

            for (k = 0; k < size; k++) {
                c = mat1[i][k];
                mat_vec = vld1q_u32(&(mat2[k][j]));
                prod_vec = vmlaq_n_u32(prod_vec, mat_vec, c);
            }

            vst1q_u32(&(prod[i][j]), prod_vec);
        }
    }
}

int main(int argc, char *argv[]) {
    uint32_t **mat1, **mat2, **prod;
    uint32_t size;
    Timer totalTime;
    short verbose = 0;
    initTimer(&totalTime, "Total Time");

    if (argc < 2) {
        printf("No size input argument supplied.\nAssuming a default size of = %dx%d\n", SIZE_DEFAULT, SIZE_DEFAULT);
        size = SIZE_DEFAULT;
    }
    else {
        size = atoi(argv[1]);
        if (size < 4 || size > SIZE_MAXIMUM || size % 4 != 0) {
            printf("Invalid size input argument supplied. Size should be in range %d to %d and a multiple of %d.\nAssuming a default size of = %dx%d\n", SIZE_MINIMUM, SIZE_MAXIMUM, SIZE_MINIMUM, SIZE_DEFAULT, SIZE_DEFAULT);
            size = SIZE_DEFAULT;
        }
        printf("size = %d\t", size);
    }

    if (argc > 2) {
        printf("Running in verbose mode.\n");
        verbose = 1;
    }

    matrixInit(&mat1, size, 2);
    matrixInit(&mat2, size, 3);
    matrixInit(&prod, size, 0);

    if (verbose) {
        printf("mat1 =\n");
        matrixPrint(mat1, size);
        printf("mat2 =\n");
        matrixPrint(mat2, size);
    }

    startTimer(&totalTime);
    matrixMultiplyNEON(mat1, mat2, prod, size);
    stopTimer(&totalTime);
    printTimer(&totalTime);

    if (verbose) {
        printf("prod =\n");
        matrixPrint(prod, size);
        printf("\nDone !!! \n");
    }

    return 0;
}
