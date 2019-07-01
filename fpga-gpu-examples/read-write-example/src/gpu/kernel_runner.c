#include <kernel.h>

uint32_t *bufferA[MAX_STREAMS], *bufferB[MAX_STREAMS];
uint32_t *addr_read, *addr_write;
int flags[MAX_STREAMS] = {1};
int max_iteration = 0;
int vector_size = 0;
pthread_mutex_t lock;

void *fpga_emulator(void *sleep_time);

/*-----------------------------------------------
 *          Function: FPGA Emulator
 *-----------------------------------------------
 * Emulate how FPGA would behave as if it called
 * on the main application runner. This function 
 * is run on a seperate stream.
 *
 * sleep_time: emulates the action processing time
 */

void *fpga_emulator(void *sleep_time){
	int i = 0;
	size_t size = vector_size*sizeof(uint32_t);
	uint32_t *buffer1 = malloc(size);
	uint32_t *buffer2 = malloc(size);

	buffer1[0] = 1;
	buffer2[0] = 1;

	printf("Starting read_write_controller\n");
	while (i<max_iteration) {
		sleep(*((float*)sleep_time));
		if (flags[i%MAX_STREAMS] == 1){
			pthread_mutex_lock(&lock);
			//pointer switch
			switch (i%2){
				case 0:
					memcpy(buffer1,addr_read,size);
					memcpy(addr_write,buffer2,size);
					break;
				case 1:
					memcpy(buffer2,addr_read,size);
					memcpy(addr_write,buffer1,size);
					break;
			}
			// updating flag (mutex protected)
			flags[i%MAX_STREAMS] = 0;
			pthread_mutex_unlock(&lock);

			i++;
		}
	}
	return NULL;	
}

/*-----------------------------------------------
 *            Main application
 * ----------------------------------------------
 *
 * Main used to launch kernel_runner. It enables
 * testing of the GPU part of the application.
 *
 * Options that can be set using command line:
 * 	- n : Number of iterations
 * 	- s : Size of the vector to copy
 * 	- w : Wait time (used to emulate FPGA)
 * 	- H : Enable HOST buffering (config 1)
 * 	- v : Enable verbosity (for results checking)
 *
 * WARNING ! This code only works with MAX_STREAMS=1 at this stage
 * (MAX_STREAMS is defined in includes/kernel.h)
 */


int main(int argc, char*argv[]){

	uint32_t *ibuff[MAX_STREAMS], *obuff[MAX_STREAMS];
	int ch; 
	float sleep_time = 0;
	bool host_buffering = false, verbose = false;
	const char *num_iteration = NULL, *in_size = NULL, *wait_time = NULL;
	struct timeval begin_time, end_time; 
	unsigned long long int lcltime = 0x0ull;
	size_t size;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{ "vector_size",	 required_argument, NULL, 's' },
			{ "max_iteration",	 required_argument, NULL, 'n' },
			{ "sleep_time",	 	required_argument, NULL, 'w' },
			{ "host_buffering",	 no_argument, NULL, 'H' },
			{ "enable_verbosity",	 no_argument, NULL, 'v' },
			{ 0, no_argument, NULL, 0 },};		

		ch = getopt_long(argc, argv,
				"s:n:w:Hv",
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
			case 'w':
				wait_time = optarg;
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
	
	if (wait_time != NULL) {
		sleep_time = atof(wait_time);
		printf("sleep : %f \n",sleep_time);
	}


	size = vector_size*sizeof(uint32_t);

	////////////////////////////////////////////////////////////////
	//               MEMORY ALLOCATION ON GPU
	////////////////////////////////////////////////////////////////

	memory_allocation_gpu(ibuff,size);
	memory_allocation_gpu(obuff,size);

	if (!host_buffering){
		init_buffers(obuff,vector_size);
	}

	////////////////////////////////////////////////////////////////
	//               MEMORY ALLOCATION ON HOST
	////////////////////////////////////////////////////////////////

	if (host_buffering){	
		memory_allocation_host(bufferA,size);
		memory_allocation_host(bufferB,size);

		for (int i = 0; i < vector_size; i++){
			for (int stream = 0; stream < MAX_STREAMS; stream++){
				bufferA[stream][i] = 1;
				bufferB[stream][i] = i + 1000*stream;
			}
		}
	}

	///////////////////////////////////////////////////////////////
	//	 RUNNING READ/WRITE CONTROLLER ON SPECIFIC THREAD
	///////////////////////////////////////////////////////////////

	pthread_t thread;
	
	if (host_buffering){
		addr_read = bufferB[0];
		addr_write = bufferA[0];
	}else{
		addr_read = obuff[0];
		addr_write = ibuff[0];
	}

	printf("Running FPGA Emulator \n");
	if (pthread_create(&thread, NULL, &fpga_emulator, (void *) &sleep_time)){
		fprintf(stderr, "Error creating FPGA Emulator thread \n");
		return 1;
	}

	if (pthread_mutex_init(&lock, NULL) != 0){
		printf("Mutex initialization failed.\n");
		return 1;
	}

	///////////////////////////////////////////////////////////////
	//             RUNNING GPU KERNEL PIPELINING
	//////////////////////////////////////////////////////////////
	int stream =0,next_stream =0;
	
	printf("Starting pipelinning \n");
	gettimeofday(&begin_time, NULL);
	
	for (int iteration = 0; iteration < max_iteration; iteration++){
		stream = iteration % MAX_STREAMS;	
		next_stream = (iteration+1) % MAX_STREAMS;	

		//FPGA is writing data in buffer
		while(flags[stream] == 1){ 
			sleep(0.000001);
		}

		if (host_buffering){
			//Running kernel on GPU with HOST buffering (Config 1)
			run_new_stream_v1(bufferA[stream],bufferB[stream],ibuff[stream],obuff[stream],vector_size);	   	

			if (verbose){
				printf("Writting : [%d,%d, ... ,%d]\n",bufferA[0][0],bufferA[0][1],bufferA[0][vector_size-1]); 
				printf("Received : [%d,%d, ... ,%d]\n",bufferB[0][0],bufferB[0][1],bufferB[0][vector_size-1]); 

				addr_read = bufferB[next_stream];
				addr_write = bufferA[next_stream];
			}
		} else {
			//Running kernel on GPU without HOST buffering (Config 2)
			run_new_stream_v2(ibuff[stream],obuff[stream],vector_size);

			addr_read = obuff[next_stream];
			addr_write = ibuff[next_stream];

			if (verbose) {	   	
				printf("Writting : [%d,%d, ... ,%d]\n",ibuff[0][0],ibuff[0][1],ibuff[0][vector_size-1]); 
				printf("Received : [%d,%d, ... ,%d]\n",obuff[0][0],obuff[0][1],obuff[0][vector_size-1]); 
			}
		}	

		// FPGA can write new data	
		pthread_mutex_lock(&lock);	
		flags[stream] = 1;
		pthread_mutex_unlock(&lock);
	}

	gettimeofday(&end_time, NULL);

	pthread_join(thread, NULL);

	printf("Completed %d iterations successfully\n", max_iteration);
	
	// Display the time of the action excecution
	lcltime = (long long)(timediff_usec(&end_time, &begin_time));
	fprintf(stdout, "GPU average processing time for %u iteration is %f usec\n",
	max_iteration, (float)lcltime/(float)(max_iteration));
	
	if (host_buffering){
		free_host(bufferA);
		free_host(bufferB);
	}

	free_device(ibuff);
	free_device(obuff);
}
