#include <stdio.h>
#include <stdint.h>
#include <Timer.h>
#include <util.h>
#include <arm_neon.h>

void MatrixMultiplyNeon(uint32_t ***mat1, uint32_t ***mat2, uint32_t ***prod, uint32_t size) {
    int i, j, k;
    uint32_t c;
    uint32x4_t mat_vec, prod_vec;

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j += 4) {

            prod_vec = vdupq_n_u32(0);

            for (k = 0; k < size; k++) {
                // c = mat1[k + i*size];
                c = (*mat1)[i][k];
                // mat_vec = vld1q_u32(&mat2[j+k*size]);
                mat_vec = vld1q_u32(&(*mat2)[k][j]);
                prod_vec = vmlaq_n_u32(prod_vec, mat_vec, c);
            }
            // vst1q_u32(&prod22[j+i*size], prod_vec);
            vst1q_u32(&(*prod)[i][j], prod_vec);
        }
    }
}

int main(int argc, char *argv[]) {
    uint32_t size;
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
    MatrixInit(&mat2, size, 3);
    MatrixInit(&prod, size, 0);

    #if defined (OUTPUT)
        printf("mat1 =\n");
        MatrixPrint(&mat1, size);
        printf("mat2 =\n");
        MatrixPrint(&mat2, size);
    #endif

    startTimer(&totalTime);
    MatrixMultiplyNeon(&mat1,&mat2,&prod,size);
    stopTimer(&totalTime);
    printTimer(&totalTime);

    #if defined (OUTPUT)
        printf("prod =\n");
        MatrixPrint(&prod, size);
        printf("\nDone !!! \n");
    #endif

    return 0;
}