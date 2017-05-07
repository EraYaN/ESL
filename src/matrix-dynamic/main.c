#include <stdio.h>
#include <stdint.h>
#include <Timer.h>
#include <util.h>
#include <matrix.h>


int main(int argc, char *argv[]) {
    uint32_t **mat1, **mat2, **prod;
    int size;
    Timer totalTime;
    short verbose = 0;
    initTimer(&totalTime, "Total Time");

    if (argc < 2) {
        printf("No size input argument supplied.\nAssuming a default size of = %dx%d\n", SIZE_DEFAULT, SIZE_DEFAULT);   
        size = SIZE_DEFAULT;
    }
    else {
        size = atoi(argv[1]);
        if (size == 0 || size > SIZE_MAXIMUM) {
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
    matrixMultiply(mat1, mat2, prod, size);
    stopTimer(&totalTime);
    printTimer(&totalTime);

    if (verbose) {
        printf("prod =\n");
        matrixPrint(prod, size);
        printf("\nDone !!! \n");
    }

    return 0;
}
