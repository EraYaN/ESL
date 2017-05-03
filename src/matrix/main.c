#include <stdio.h>
#include <Timer.h>
#include <util.h>

void matMult(int *mat1, int *mat2, int *prod);

int main()
{
	int i, j;
    Timer totalTime;
    initTimer(&totalTime, "Total Time");

    int mat1[SIZE*SIZE], mat2[SIZE*SIZE], prod[SIZE*SIZE];
	for (i = 0; i < SIZE; i++)
	{
		for (j = 0; j < SIZE; j++)
		{
			mat1[INDEX(i, j)] = i + j * 2;
			mat2[INDEX(i, j)] = i + j * 3;
			prod[INDEX(i, j)] = 0;
		}
	}

    startTimer(&totalTime);
    matMult(mat1,mat2,prod);
    stopTimer(&totalTime);
    printTimer(&totalTime);

    printf("\nDone !!! \n");
    return 0;
}

void matMult(int *mat1, int *mat2, int *prod)
{
    int i, j, k;
    for (i = 0;i < SIZE; i++)
    {
        for (j = 0; j < SIZE; j++)
        {
            for(k=0;k<SIZE;k++)
                prod[INDEX(i, j)] = prod[INDEX(i, j)]+mat1[INDEX(i, k)] * mat2[INDEX(k, j)];
        }
    }
}
