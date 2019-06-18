#include <kernel.h>
#include <stdio.h>
#include <malloc.h>
#define N 1024

int main(void){

	uint32_t *A=NULL, *B=NULL;
	size_t size = N*sizeof(uint32_t);

	A = (uint32_t*)malloc(size);
	B = (uint32_t*)malloc(size);

	for (uint32_t i=0; i<N; i++){
		A[i] = i;
		B[i] = 0;
	}

	cuda_add(A,A,B,(int)N);
	
	printf("Vector addition resuts\n");	
	printf("A = [%d,%d,%d, ... , %d]\n", A[0],A[1],A[3],A[N-1]);	
	printf("A+A = [%d,%d,%d, ... , %d]\n", B[0],B[1],B[3],B[N-1]);	
	
	free(A);
	free(B);
	
	return 0;

}
