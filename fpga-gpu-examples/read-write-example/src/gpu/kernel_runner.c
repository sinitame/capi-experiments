#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <kernel.h>
#include <stdbool.h>
#include <getopt.h>

uint32_t *bufferA[MAX_STREAMS], *bufferB[MAX_STREAMS];
int flags[MAX_STREAMS] = {1};
int max_iteration = 10;
pthread_mutex_t lock;

void *read_write_controller(void *verbose);

// Read/write controller thread
void *read_write_controller(void *verbose){
	int i = 0;
	printf("Starting read_write_controller\n");
	while (i<max_iteration) {
		sleep(0.000005);
		if (flags[i%MAX_STREAMS] == 1){
			
			if ((bool)verbose){
				// Printing bufferA and bufferB element
				printf("A : %d\n",bufferA[i%MAX_STREAMS][1]);
				printf("B : %d\n",bufferB[i%MAX_STREAMS][1]);
			}	
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

int main(int argc, char*argv[]){

	uint32_t *ibuff[MAX_STREAMS], *obuff[MAX_STREAMS];
	int ch, vector_size=0, max_iteration=0;
	bool host_buffering = false, verbose = false;
	const char *num_iteration = NULL, *in_size = NULL;
	size_t size;
	
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{ "vector_size",	 required_argument, NULL, 's' },
			{ "max_iteration",	 required_argument, NULL, 'n' },
			{ "host_buffering",	 no_argument, NULL, 'H' },
			{ "enable_verbosity",	 no_argument, NULL, 'v' },
			{ 0, no_argument, NULL, 0 },};		

		ch = getopt_long(argc, argv,
				"s:n:Hv",
				long_options, &option_index);
		if (ch == -1)
			break;

		switch (ch) {

			case 's':
				in_size = optarg;
				break;
			case 'n':
				num_iteration = optarg;
				break;
			case 'H':
				host_buffering = true;
				break;		
			case 'v':
				verbose = true;
				break;		
		}
	}

	if (in_size != NULL) {
		vector_size = atoi(in_size);
	}

	if (num_iteration != NULL) {
		max_iteration = atoi(num_iteration);
	}

	
	size = vector_size*sizeof(uint32_t);

	////////////////////////////////////////////////////////////////
	//               MEMORY ALLOCATION ON GPU
	////////////////////////////////////////////////////////////////
	
	memory_allocation_gpu(ibuff,size);
	memory_allocation_gpu(obuff,size);
	
	////////////////////////////////////////////////////////////////
	//               MEMORY ALLOCATION ON HOST
	////////////////////////////////////////////////////////////////

	if (host_buffering){	
		memory_allocation_host(bufferA,size);
		memory_allocation_host(bufferB,size);
		
		for (int i = 0; i < vector_size; i++){
			for (int stream = 0; stream < MAX_STREAMS; stream++){
				bufferA[stream][i] = i + 1000*stream;
			}
		}
	}
	
	///////////////////////////////////////////////////////////////
	//	 RUNNING READ/WRITE CONTROLLER ON SPECIFIC THREAD
	///////////////////////////////////////////////////////////////

	pthread_t thread;
	printf("Running thread \n");
	if (pthread_create(&thread, NULL, &read_write_controller, (void *)((bool)(verbose && host_buffering)))){
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
		
		if(host_buffering){ 
			run_new_stream_v1(bufferA[stream],bufferB[stream],ibuff[stream],obuff[stream],vector_size, stream);	   	
		} else {
			run_new_stream_v2(ibuff[stream],obuff[stream],vector_size);	   	
		}	
		
		// FPGA can write new data	
		pthread_mutex_lock(&lock);	
		flags[stream] = 1;
		pthread_mutex_unlock(&lock);
	}
	
	pthread_join(thread, NULL);
	printf("Completed %d iterations successfully\n", max_iteration);
	
	if (host_buffering){
		free_host(bufferA);
		free_host(bufferB);
	}
	free_device(ibuff);
	free_device(obuff);
}
