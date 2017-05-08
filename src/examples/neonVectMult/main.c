#include <stdio.h>
#include <stdlib.h>

//#include "arm_neon.h"

void Timer_Init(void);
void Timer_DeInit(void);
long HiResTime(void);
int nElem = 1024 * 16;

void Print(float * array)
{
    int i;
    for (i=0; i<10; i++)
    {
        printf("%f ", array[i] );
    }

    printf("\n");
}


void Init(float * array)
{
    int i;
    for (i=0; i<nElem; i++)
    {
        array[i] = i * 0.5;
    }
}


void Mult(float * __restrict a, float * __restrict b, float * __restrict z)
{
    int i,j;
    float sum1 = 0.0;
    float sum2 = 0.0;

    for (i=0; i<nElem; i++)
    {
        z[i] = a[i] * b[i];
    }
}

int main (int argc, char ** argv)
{
    float * a = (float*) malloc ( nElem * sizeof(float) );
    float * b = (float*) malloc ( nElem * sizeof(float) );
    float * c = (float*) malloc ( nElem * sizeof(float) );	
    
	long tStart, tEnd;
	Timer_Init();
	
	
    Init(a);
    Init(b);
	
	tStart = HiResTime();
    Mult(a,b,c);
	tEnd = HiResTime();
	
    Print(a);
    Print(b);
    Print(c);

	printf("\n Execution Time = %d \n", (tEnd - tStart) );
	
	Timer_DeInit();
	free(a);
	free(b);
	free(c);
    return 0;
}



/************** Timing routine (for performance measurements) ***********/
/* unfortunately, this is generally assembly code and not very portable */


void Timer_Init(void)
{
#if defined(__arm__)
    int user_enable;
#if defined(__GNUC__)
#define COMPILER_ID "GCC"
asm ("mrc p15, 0, %0, c9, c14, 0" : "=r"(user_enable));
#elif defined(__CC_ARM)
#define COMPILER_ID "ARMCC"
#error "ARMCC not implemented"
#else
#error  "Unknown ARM compiler"
#endif
    if (!user_enable)
        printf("User mode enable is not set, no access to cycle counter\n");

#if defined(__GNUC__)
    /* Reset and enable */
    asm volatile ("mcr p15, 0, %0, c9, c12, 0" :: "r"(5));
    /* Enable cycle counter */
    asm volatile ("mcr p15, 0, %0, c9, c12, 1" :: "r"(0x80000000));
    /* Reset overflow flags */
    asm volatile ("mcr p15, 0, %0, c9, c12, 3" :: "r"(0x80000000));
#endif

#else
    printf("No support for RDTSC on this CPU platform\n");
#endif /* defined(__arm__) */
}

void Timer_DeInit(void)
{
#if defined(__GNUC__)
    /* Disable */
    asm volatile ("mcr p15, 0, %0, c9, c12, 0" :: "r"(0));
#endif
}

long HiResTime(void)
{
    long cycles;
#if defined(__arm__)
#if defined(__GNUC__)
asm volatile ("mrc p15, 0, %0, c9, c13, 0": "=r"(cycles));
#endif
    return cycles;
#else
    return 0;
#endif
}
