#include <stdio.h>
#include <stdint.h>
#include <Timer.h>
#include <util.h>


int main(int argc, char *argv[]) {
    int size;
    Timer totalTime;
    initTimer(&totalTime, "Total Time");

    if (argc != 2 ) {
        printf("Wrong arguments supplied!\n Taking size = SIZE_DEFAULT = %d", SIZE_DEFAULT);
        size = SIZE_DEFAULT;
    } else {
        size = atoi(argv[1]);
        if (size == 0) {
            size = SIZE_DEFAULT;
        }
        printf("size  = %d\n", size);
    }

    uint32_t **mat1, **mat2, **prod;

    MatrixInit(&mat1, size, 2);
    MatrixInit(&mat2,size,3);
    MatrixInit(&prod,size,0);

    #if defined (OUTPUT)
        printf("mat1 =\n");
        MatrixPrint(&mat1, size);
        printf("mat2 =\n");
        MatrixPrint(&mat2, size);
    #endif

    startTimer(&totalTime);
    MatrixMultiply(&mat1,&mat2,&prod,size);
    stopTimer(&totalTime);
    printTimer(&totalTime);

    #if defined (OUTPUT)
        printf("prod =\n");
        MatrixPrint(&prod, size);
        printf("\nDone !!! \n");
    #endif

    return 0;
}
