#include <stdio.h>

#define SIZE 128
#define INDEX(i, j) j + SIZE * i

void matPrint(int *mat)
{
    int i, j;

    for (i = 0; i < SIZE; i++)
    {
        printf("\n");
        for (j = 0; j < SIZE; j++)
        {
            printf("\t%d ", mat[INDEX(i,j)]);
        }
    }
    printf("\n");
}
