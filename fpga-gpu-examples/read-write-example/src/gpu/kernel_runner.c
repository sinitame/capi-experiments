#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <kernel.h>

uint32_t *bufferA[MAX_STREAMS], *bufferB[MAX_STREAMS];
int flags[MAX_STREAMS] = {1};
int max_iteration = 10;
pthread_mutex_t lock;

void *read_write_controller(void *);

// Read/write controller thread
void *read_write_controller(void *unused){
	(void)unused;
	int i = 0;
	printf("Starting read_write_controller\n");
	while (i<max_iteration) {
		sleep(0.0001);
		if (flags[i%MAX_STREAMS] == 1){

			// Printing bufferA and bufferB element
			printf("A : %d\n",bufferA[i%MAX_STREAMS][1]);
			printf("B : %d\n",bufferB[i%MAX_STREAMS][1]);
			
			// updating flag (mutex protected)
			pthread_mutex_lock(&lock);
			flags[i%MAX_STREAMS] = 0;
			//printf("iteration (%d) : [%d,%d,%d]\n",i,flags[0],flags[1],flags[2]);
			pthread_mutex_unlock(&lock);

			i++;
		}
	}
	return NULL;	
}

int main(){

	uint32_t *ibuff[MAX_STREAMS], *obuff[MAX_STREAMS];
	int vector_size = 1024*1024;
	size_t size = vector_size*sizeof(uint32_t);

	////////////////////////////////////////////////////////////////
	//               MEMORY ALLOCATION ON GPU
	////////////////////////////////////////////////////////////////
	
	memory_allocation_gpu(ibuff,size);
	memory_allocation_gpu(obuff,size);
	
	////////////////////////////////////////////////////////////////
	//               MEMORY ALLOCATION ON HOST
	////////////////////////////////////////////////////////////////
	
	memory_allocation_host(bufferA,size);
	memory_allocation_host(bufferB,size);
	
	for (int i = 0; i < vector_size; i++){
		for (int stream = 0; stream < MAX_STREAMS; stream++){
			bufferA[stream][i] = i + 1000*stream;
		}
	}
	
	///////////////////////////////////////////////////////////////
	//	 RUNNING READ/WRITE CONTROLLER ON SPECIFIC THREAD
	///////////////////////////////////////////////////////////////

	pthread_t thread;
	printf("Running thread \n");
	if (pthread_create(&thread, NULL, &read_write_controller, NULL)){
		fprintf(stderr, "Error creating thread \n");
		return 1;
	}
 	
	if (pthread_mutex_init(&lock, NULL) != 0){
        	printf("Mutex initialization failed.\n");
       	 	return 1;
	}

	///////////////////////////////////////////////////////////////
	//             RUNNING GPU KERNEL PIPELINING
	//////////////////////////////////////////////////////////////
	int stream =0;
	
	for (int iteration = 0; iteration < max_iteration; iteration++){
		stream = iteration % MAX_STREAMS;	
		
		//FPGA is writing data in buffer
		while(flags[stream] == 1){ 
			sleep(0.0001);
		}
		 
		run_new_stream(bufferA[stream],bufferB[stream],ibuff[stream],obuff[stream],vector_size);	   	
		
		// FPGA can write new data	
		pthread_mutex_lock(&lock);	
		flags[stream] = 1;
		pthread_mutex_unlock(&lock);
	}
	
	pthread_join(thread, NULL);
	printf("Completed %d iterations successfully\n", max_iteration);
	
	free_host(bufferA);
	free_host(bufferB);
	free_device(ibuff);
	free_device(obuff);
}
