#include <kernel.h>

uint32_t *bufferA[MAX_STREAMS], *bufferB[MAX_STREAMS];
uint32_t *ibuff[MAX_STREAMS], *obuff[MAX_STREAMS];
uint32_t *addr_read, *addr_write;
int volatile *flags[MAX_STREAMS] = {NULL};
int read_flag = 0, write_flag = 0;
int max_iteration = 10;
int vector_size = 1048576;
bool host_buffering = false, verbose = false;
pthread_mutex_t lock_fpga, runner_lock, flag_lock;

void *fpga_emulator(void *sleep_time);
void *stream_runner(void *stream_arg);

// Read/write controller thread
void *fpga_emulator(void *sleep_time){
	int i = 0;
	size_t size = vector_size*sizeof(uint32_t);
	uint32_t *buffer1 = malloc(size);
	uint32_t *buffer2 = malloc(size);

	buffer1[0] = 1;
	buffer2[0] = 1;

	printf("Starting read_write_controller\n");
	while (i<max_iteration) {
		sleep(0.000001);
		if ((read_flag == 1) && (write_flag == 1)){
		pthread_mutex_lock(&lock_fpga);
			sleep(*((float*)sleep_time));
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
			read_flag = 0;
			write_flag = 0;
			i++;
		pthread_mutex_unlock(&lock_fpga);
		}
	}
	return NULL;	
}

void *stream_runner(void *stream_arg){
	int stream_runner = *((int*)stream_arg);
	int i = stream_runner;
	while (i < max_iteration){
		//waiting to know if it can be run
		while( *flags[stream_runner] == 0){
			sleep(0.0000001);
		}
		pthread_mutex_lock(&runner_lock);
		if (host_buffering){
			//Running kernel on GPU
			run_new_stream_v1(bufferA[stream_runner], bufferB[stream_runner], ibuff[stream_runner],obuff[stream_runner],vector_size,stream_runner);	   	

			if (verbose){
				printf("Writting (%d:%d): [%d,%d, ... ,%d]\n",i,stream_runner,bufferA[stream_runner][0],bufferA[stream_runner][1],bufferA[stream_runner][vector_size-1]); 
				printf("Received (%d:%d): [%d,%d, ... ,%d]\n",i,stream_runner,bufferB[stream_runner][0],bufferB[stream_runner][1],bufferB[stream_runner][vector_size-1]); 
			}

		} else {
			//Running kernel on GPU
			run_new_stream_v2(ibuff[stream_runner], obuff[stream_runner], vector_size, stream_runner);

			if (verbose) {	   	
				printf("Writting (%d:%d): [%d,%d, ... ,%d]\n",i,stream_runner,ibuff[stream_runner][0],ibuff[stream_runner][1],ibuff[stream_runner][vector_size-1]); 
				printf("Received (%d:%d): [%d,%d, ... ,%d]\n",i,stream_runner,obuff[stream_runner][0],obuff[stream_runner][1],obuff[stream_runner][vector_size-1]); 
			}
		}
		pthread_mutex_unlock(&runner_lock);
		
		wait_completion(stream_runner);
		//Thread is available for new compute			
		pthread_mutex_lock(&flag_lock);
		*flags[stream_runner] = 0;
		pthread_mutex_unlock(&flag_lock);
		
		i += MAX_STREAMS;
	}
	return NULL;
}

int main(int argc, char*argv[]){

	int ch; 
	float sleep_time = 0;
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
			default:
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
		init_buffer(ibuff,vector_size);
		//init_buffer(obuff,vector_size);
	}

	////////////////////////////////////////////////////////////////
	//               MEMORY ALLOCATION ON HOST
	////////////////////////////////////////////////////////////////

	if (host_buffering){	
		memory_allocation_host(bufferA,size);
		memory_allocation_host(bufferB,size);

		for (int i = 0; i < vector_size; i++){
			for (int stream = 0; stream < MAX_STREAMS; stream++){
				//bufferA[stream][i] = 1;
				bufferA[stream][i] = i + 1000*stream;
				bufferB[stream][i] = i + 1000*stream;
			}
		}
	}

	for (int i = 0; i<MAX_STREAMS;i++){
		posix_memalign((void **)&flags[i], 128, sizeof(int));
		*flags[i] = 0;
	}

	///////////////////////////////////////////////////////////////
	//	 RUNNING FPGA EMULATOR ON SPECIFIC THREAD
	///////////////////////////////////////////////////////////////

	//pthread_t thread_fpga;
	//

	//printf("Running FPGA emulator thread \n");
	//if (pthread_create(&thread_fpga, NULL, &fpga_emulator, (void *) &sleep_time)){
	//	fprintf(stderr, "Error creating thread \n");
	//	return 1;
	//}
	//

	///////////////////////////////////////////////////////////////
	//	 RUNNING  STREAM RUNNER THREADS
	///////////////////////////////////////////////////////////////
	init_streams();	
	
	pthread_mutex_init(&runner_lock,NULL);
	pthread_mutex_init(&flag_lock,NULL);
	pthread_t threads[MAX_STREAMS];
	
	//pthread_attr_t attr;	
	//cpu_set_t cpus;
    	//pthread_attr_init(&attr);

	printf("Running stream runner threads \n");
	for (int stream = 0; stream < MAX_STREAMS; stream ++){
		int *arg = malloc(sizeof(*arg));
		*arg = stream;

       		//CPU_ZERO(&cpus);
       		//CPU_SET(stream, &cpus);
       		//pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);

		if (pthread_create(&threads[stream], NULL, &stream_runner, arg)){
			fprintf(stderr, "Error creating thread \n");
			return 1;
		}
	}

	printf("Starting pipelinning \n");
	gettimeofday(&begin_time, NULL);
	///////////////////////////////////////////////////////////////
	//             RUNNING GPU KERNEL PIPELINING
	//////////////////////////////////////////////////////////////
	int stream =0,next_stream =0;

	if (host_buffering){
		addr_read = bufferB[0];
		addr_write = bufferA[0];
	}else{
		addr_read = obuff[0];
		addr_write = ibuff[0];
	}

	//read_flag = 1;
	//write_flag = 1;
	

	for (int iteration = 0; iteration < max_iteration; iteration++){
		
		stream = iteration % MAX_STREAMS;
		next_stream = (iteration+1) % MAX_STREAMS;
		
		// FPGA is reading/writing data in buffer
		//while((read_flag == 1)||(write_flag == 1)){ 
		//	sleep(0.0001);
		//}

		// Telling the thread that it can run new job
		pthread_mutex_lock(&flag_lock);
		*flags[stream] = 1;
		pthread_mutex_unlock(&flag_lock);
	
		//printf("flags[%d,%d,%d]\n",*flags[0],*flags[1],*flags[2]);	
		// Wait if next thread is ready for new data
		while (*flags[next_stream] == 1){
			sleep(0.0000001);
		}
		
		//if (host_buffering) {	
		//	addr_read = bufferB[next_stream];
		//	addr_write = bufferA[next_stream];
		//} else {
		//	addr_read = obuff[next_stream];
		//	addr_write = ibuff[next_stream];
		//}

		// Tell FPGA to process new data
		//read_flag = 1;
		//write_flag = 1;

	}

	gettimeofday(&end_time, NULL);

	//pthread_join(thread_fpga, NULL);
	for (int stream = 0; stream < MAX_STREAMS; stream ++){
		if (pthread_join(threads[stream], NULL)){
			fprintf(stderr, "Error joining thread \n");
			return 1;
		}
	}

	printf("Completed %d iterations successfully\n", max_iteration);

	// Display the time of the action excecution
	lcltime = (long long)(timediff_usec(&end_time, &begin_time));
	fprintf(stdout, "SNAP action average processing time for %u iteration is %f usec\n",
	max_iteration, (float)lcltime/(float)(max_iteration));
	

	if (host_buffering){
		free_host(bufferA);
		free_host(bufferB);
	}
	free_device(ibuff);
	free_device(obuff);
}
