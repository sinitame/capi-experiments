#include <kernel.h>
#include <stdint.h>


__global__ void add_uint8(uint32_t *A, uint32_t *B, uint32_t *C, int N){

	int id = blockIdx.x*blockDim.x+threadIdx.x;
	if (id<N){
		C[id] = A[id] + B[id];
	}
}


void cuda_add(uint32_t *A, uint32_t *B, uint32_t *C, int N){

	size_t size = N*sizeof(uint32_t);
	uint32_t *d_A, *d_B, *d_C;

	cudaMalloc(&d_A, size);
	cudaMalloc(&d_B, size);
	cudaMalloc(&d_C, size);

	cudaMemcpy(d_A, A, size, cudaMemcpyHostToDevice);
	cudaMemcpy(d_B, B, size, cudaMemcpyHostToDevice);

	int blockSize = 64;
	int numBlocks = N/64 + 1;

	add_uint8<<<numBlocks,blockSize>>>(d_A,d_B,d_C,N);

	cudaMemcpy(C,d_C, size, cudaMemcpyDeviceToHost);
	
	cudaFree(d_A);
	cudaFree(d_B);
	cudaFree(d_C);
}

