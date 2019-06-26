#include <kernel.h>

cudaStream_t streams[MAX_STREAMS];

inline cudaError_t checkCuda(cudaError_t result)
{
  if (result != cudaSuccess) {
    fprintf(stderr, "CUDA Runtime Error: %s\n", 
            cudaGetErrorString(result));
    assert(result == cudaSuccess);
  }
  return result;
}

// Data initialization kernel
__global__ void init_data(uint32_t *buff, const int vector_size){
	int idx = threadIdx.x+blockDim.x*blockIdx.x;
	int my_idx = idx;
	while (my_idx < vector_size){
		buff[my_idx] = my_idx;
		my_idx += gridDim.x*blockDim.x; // grid-striding loop
	}
}

// Vector addition kernel
__global__ void vector_add(uint32_t *ibuff, uint32_t *obuff, const int vector_size){
	
	int idx = threadIdx.x+blockDim.x*blockIdx.x;
	int my_idx = idx;
	while (my_idx < vector_size){
		obuff[my_idx] = ibuff[my_idx] + ibuff[my_idx];
		my_idx += gridDim.x*blockDim.x; // grid-striding loop
	}
}

void memory_allocation_gpu(uint32_t *buffer[MAX_STREAMS], size_t size){
	int result=0, device_id=0;

	printf("Memory allocation GPU\n");
	cudaDeviceGetAttribute (&result, cudaDevAttrConcurrentManagedAccess, device_id);
	for (int stream = 0; stream < MAX_STREAMS; stream++){
		checkCuda(cudaMallocManaged(&buffer[stream],size));
		if (result) {
			checkCuda(cudaMemAdvise(buffer[stream],size,cudaMemAdviseSetPreferredLocation,device_id));
		}
		checkCuda(cudaMemset(buffer[stream], 0, size));
	}
}

void memory_allocation_host(uint32_t *buffer[MAX_STREAMS], size_t size){

	printf("Memory allocation HOST\n");
	for (int stream = 0; stream < MAX_STREAMS; stream++){
		checkCuda(cudaHostAlloc(&buffer[stream], size, cudaHostAllocDefault));
	}
}

void init_buffer(uint32_t *buffer[MAX_STREAMS], int vector_size){
	int numBlocks, numThreadsPerBlock = 1024;
	cudaDeviceGetAttribute(&numBlocks, cudaDevAttrMultiProcessorCount, 0);	
	for (int stream = 0; stream < MAX_STREAMS; stream++){
		init_data<<<4*numBlocks, numThreadsPerBlock>>>(buffer[stream],vector_size);
	}
	cudaDeviceSynchronize();

}

void init_streams(){
	for (int stream = 0; stream < MAX_STREAMS; stream++){
		cudaStreamCreate(&streams[stream]);
	}
}

void run_new_stream_v1(uint32_t *bufferA, uint32_t *bufferB, uint32_t *ibuff, uint32_t *obuff, int vector_size, int stream){
	int numBlocks, numThreadsPerBlock = 1024;
	size_t size = vector_size*sizeof(uint32_t);
	
	//printf("Running kernel on GPU ..\n");
	cudaStream_t stream_i = streams[stream];
	cudaDeviceGetAttribute(&numBlocks, cudaDevAttrMultiProcessorCount, 0);	
	
	cudaMemcpyAsync(ibuff,bufferA, size, cudaMemcpyDeviceToHost, stream_i);
	vector_add<<<4*numBlocks, numThreadsPerBlock,0,stream_i>>>(ibuff,obuff,vector_size);
	cudaMemcpyAsync(bufferB, obuff, size, cudaMemcpyHostToDevice, stream_i);
	cudaStreamSynchronize(stream_i);
}

void run_new_stream_v2(uint32_t *ibuff, uint32_t *obuff, int vector_size, int stream){
	int numBlocks, numThreadsPerBlock = 1024;
	cudaStream_t stream_i = streams[stream];
	
	cudaDeviceGetAttribute(&numBlocks, cudaDevAttrMultiProcessorCount, 0);	
	vector_add<<<4*numBlocks, numThreadsPerBlock, 0, stream_i>>>(ibuff,obuff,vector_size);
	cudaStreamSynchronize(stream_i);
}


void free_host(uint32_t *buffer[MAX_STREAMS]){
	for (int i = 0; i < MAX_STREAMS; i++){
		cudaFreeHost(buffer[i]);
	}
}

void free_device(uint32_t *buffer[MAX_STREAMS]){
	for (int i = 0; i < MAX_STREAMS; i++){
		cudaFree(buffer[i]);
	}
}
