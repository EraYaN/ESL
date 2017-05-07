#include <stdio.h>
#include <stdint.h>
#include <Timer.h>
#include <util.h>
#include <matrix.h>


int main(int argc, char *argv[]) {
    uint32_t **mat1, **mat2, **prod;
    int size;
    Timer totalTime;
    initTimer(&totalTime, "Total Time");

    if (argc != 2) {
        printf("Wrong arguments supplied!\n Taking size = SIZE_DEFAULT = %d", SIZE_DEFAULT);
        size = SIZE_DEFAULT;
    }
    else {
        size = atoi(argv[1]);
        if (size == 0 || size > SIZE_MAXIMUM) {
            size = SIZE_DEFAULT;
        }
        printf("size  = %d\t", size);
    }

    matrixInit(&mat1, size, 2);
    matrixInit(&mat2, size, 3);
    matrixInit(&prod, size, 0);

#if defined (OUTPUT)
    printf("mat1 =\n");
    matrixPrint(mat1, size);
    printf("mat2 =\n");
    matrixPrint(mat2, size);
#endif

    startTimer(&totalTime);
    matrixMultiply(mat1, mat2, prod, size);
    stopTimer(&totalTime);
    printTimer(&totalTime);

#if defined (OUTPUT)
    printf("prod =\n");
    matrixPrint(prod, size);
    printf("\nDone !!! \n");
#endif

    return 0;
}
