#include <arm_neon.h>
#include <stdio.h>
#include <Timer.h>
#include <util.h>

void matMult(uint32_t *mat1, uint32_t *mat2, uint32_t *prod);

int main()
{
	int i, j;
    Timer totalTime;
    initTimer(&totalTime, "Total Time");

    uint32_t mat1[SIZE*SIZE], mat2[SIZE*SIZE], prod[SIZE*SIZE];

	for (i = 0; i < SIZE; i++)
	{
		for (j = 0; j < SIZE; j += 4)
		{
			mat1[INDEX(i, j)] = i + j * 2;
			mat2[INDEX(i, j)] = i + j * 3;
		}	
	}

    startTimer(&totalTime);
    matMult(mat1,mat2,prod);
    stopTimer(&totalTime);
    printTimer(&totalTime);

    printf("\nDone !!! \n");
    return 0;
}

void matMult(uint32_t *mat1, uint32_t *mat2, uint32_t *prod)
{
    int i, j, k;
    uint32_t c;
    uint32x4_t mat_vec, prod_vec;

    for (i = 0; i < SIZE; i++)
    {
        for (j = 0; j < SIZE; j += 4)
        {
            prod_vec = vdupq_n_u32(0);

            for (k = 0; k < SIZE; k++)
            {
                c = mat1[INDEX(i, k)];
                mat_vec = vld1q_u32(&mat2[INDEX(k, j)]);
                prod_vec = vmlaq_n_u32(prod_vec, mat_vec, c);
            }

            vst1q_u32(&prod[INDEX(i, j)], prod_vec);
        }
    }
}
