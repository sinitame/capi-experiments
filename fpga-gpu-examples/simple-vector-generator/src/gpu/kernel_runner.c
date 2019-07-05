#include <kernel.h>

static void usage(const char *prog)
{
	printf("\n Usage: %s [-h] [-v, --verbose]\n"
			"  -s, --vector_size <N>     	size of the uint32_t buffer array.\n"
			"  -H, --host_buffering      	enable host buffering to test config 1 (default is config 2).\n"
			"\n"
			"Example usage:\n"
			"-----------------------\n"
			"kernel_runner -s 1024 -v\n"
			"\n",
			prog);
}

/*-----------------------------------------------
 *            Main application
 * ----------------------------------------------
 *
 * Main used to launch kernel_runner. It enables
 * testing of the GPU part of the application.
 *
 * Options that can be set using command line:
 * 	- s : Size of the uint32_t buffer array
 * 	- H : Enable HOST buffering (config 1)
 * 	- v : Enable verbosity (for results checking)
 *
 */
int main(void){
	uint32_t *bufferA[MAX_STREAMS], *bufferB[MAX_STREAMS];
	uint32_t *ibuff = NULL, *obuff = NULL;
	int ch, vector_size=0; 
	bool host_buffering = false, verbose = false, fpga_emulation = false;
	const char *in_size = NULL;
	struct timeval begin_time, end_time; 
	unsigned long long int lcltime = 0x0ull;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{ "vector_size",	 required_argument, NULL, 's' },
			{ "host_buffering",	 no_argument, NULL, 'H' },
			{ "verbosity",	 	no_argument, NULL, 'v' },
			{ "help", no_argument, NULL, 'h' },
			{ 0, no_argument, NULL, 0 },};		

		ch = getopt_long(argc, argv,
				"s:Hvh",
				long_options, &option_index);
		if (ch == -1)
			break;

		switch (ch) {

			case 's':
				in_size = optarg;
				break;
			case 'H':
				host_buffering = true;
				break;		
			case 'v':
				verbose = true;
				break;		
			case 'h':
				usage(argv[0]);
				exit(EXIT_SUCCESS);
				break;
			default:
				usage(argv[0]);
				exit(EXIT_FAILURE);
				break;	
		}
	}

	if (argc == 1) {       // to provide help when program is called without argument
		usage(argv[0]);
		exit(EXIT_FAILURE);}

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

	size_t size;	size_t size = N*sizeof(uint32_t);

	////////////////////////////////////////////////////////////////
	//               MEMORY ALLOCATION ON GPU
	////////////////////////////////////////////////////////////////

	memory_allocation_gpu(ibuff,size);
	memory_allocation_gpu(obuff,size);

	if (!host_buffering){
		init_buffer(ibuff,vector_size);
	}

	if (host_buffering){	
		memory_allocation_host(bufferA,size);
		memory_allocation_host(bufferB,size);

		for (int i = 0; i < vector_size; i++){
			bufferA[i] = i;
			bufferB[i] = 1;
		}
	}

	gettimeofday(&begin_time, NULL);
	vector_add(ibuff,obuff,vector_size);
	gettimeofday(&end_time, NULL);

	if (verbose) {
		if (host_buffering){
			printf("Vector addition resuts\n");	
			printf("Input [%d,%d,%d, ... , %d]\n", bufferA[0],bufferA[1],bufferA[3],bufferA[N-1]);	
			printf("Output = [%d,%d,%d, ... , %d]\n", bufferB[0],bufferB[1],bufferB[3],bufferB[N-1]);	
		}else{
			printf("Vector addition resuts\n");	
			printf("Input [%d,%d,%d, ... , %d]\n", ibuff[0],ibuff[1],ibuff[3],ibuff[N-1]);	
			printf("Output = [%d,%d,%d, ... , %d]\n", obuff[0],obuff[1],obuff[3],obuff[N-1]);	
		}
	}

	ftime = (long long)(timediff_usec(&end_time, &begin_time));
	fprintf(stdout, "GPU average processing time for a vector of size %d is %f usec with config %d\n",
			vector_size, (float)lcltime,
			host_buffering ? 1 : 2);
	
	if (host_buffering){
		free_host(bufferA);
		free_host(bufferB);
	}

	free_device(ibuff);
	free_device(obuff);
	return 0;
}
