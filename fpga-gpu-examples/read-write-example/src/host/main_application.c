/*
 * Copyright 2017 International Business Machines
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * FPGA to GPU READ/WRITE Example
 *
 * Demonstration how to perform read(write) between FPGA and a GPU on Power server with CAPI.
 * FPGA reads(writes) data in parallel from(to) HOST or GPU memory and store values into
 * internal buffer. 
 * GPU reads(writes) data from(to) HOST or internal memory perform a simple operation
 * on the read vector and then writes the result to HOST or internal memory.
 * Synchronization between FPGA and GPU is handled by flags stored in the HOST.
 * Data used in this example are vectors of uint32_t with a length defined at runtime (vector_size). 
 * 
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
#include <stdbool.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <action_create_vector.h>
#include <snap_hls_if.h>

#include <kernel.h>

// Function that fills the MMIO registers / data structure 
// // these are all data exchanged between the application and the action
static void snap_prepare_parallel_memcpy(struct snap_job *cjob,
		struct parallel_memcpy_job *mjob,
		int size,int max_iteration,uint8_t type,
		void *addr_read,void *addr_write,
		void *addr_read_flag, void *addr_write_flag)
{
	fprintf(stderr, "  prepare parallel_memcpy job of %ld bytes size\n", sizeof(*mjob));

	assert(sizeof(*mjob) <= SNAP_JOBSIZE);
	memset(mjob, 0, sizeof(*mjob));

	mjob->vector_size = (uint64_t)size;
	mjob->max_iteration = (uint64_t)max_iteration;

	// Setting output params : where result will be written in host memory
	snap_addr_set(&mjob->read, addr_read, size*sizeof(uint32_t), type,
			SNAP_ADDRFLAG_ADDR | SNAP_ADDRFLAG_SRC |SNAP_ADDRFLAG_END);

	snap_addr_set(&mjob->write, addr_write, size*sizeof(uint32_t), type,
			SNAP_ADDRFLAG_ADDR | SNAP_ADDRFLAG_DST |SNAP_ADDRFLAG_END);

	snap_addr_set(&mjob->read_flag, addr_read_flag, 64, type,
			SNAP_ADDRFLAG_ADDR | SNAP_ADDRFLAG_SRC |SNAP_ADDRFLAG_END);

	snap_addr_set(&mjob->write_flag, addr_write_flag, 64, type,
			SNAP_ADDRFLAG_ADDR | SNAP_ADDRFLAG_SRC |SNAP_ADDRFLAG_END);

	snap_job_set(cjob, mjob, sizeof(*mjob), NULL, 0);
}

static void update_flag(uint8_t **flag, uint8_t flag_value, uint64_t addr){
	for (int i = 0; i < (int)sizeof(uint64_t); i++){
		(*flag)[i+1] = (addr >> 8*i) & 0xFF;
	}
	(*flag)[0] = (uint8_t)flag_value;

}


static void usage(const char *prog)
{
	printf("\n Usage: %s [-h] [-v, --verbose]\n"
			"  -s, --vector_size <N>     	size of the uint32_t buffer arrays.\n"
			"  -n, --num_iteration <N>   	number of iterations in a run.\n"
			"  -H, --host_buffering      	enable host buffering to test config 1 (default is config 2).\n"
			"\n"
 			"----------------------------------------------------\n"
			"WARNING ! This code only works with MAX_STREAMS=1 at this stage\n"
			"(MAX_STREAMS is defined in includes/kernel.h)\n"
			"\n"
			"WARNING ! This code only works with vector_size < 131072 \n"
			"because of FPGA in-memory limitations on this version of the image).\n"
			"\n"
			"\n"
			"REMARKS: Two config can be tested with this code: \n"
			"Config 1 : Data is copied on the HOST before being sent to the GPU(-H to enable this config)\n"
			"Config 2 : Data is copied directly from the HOST to the GPU (default config)\n"
			"----------------------------------------------------\n"
			"\n"
			"Example usage:\n"
			"-----------------------\n"
			"main_application -s 1024 -n 10 -v\n"
			"\n",
			prog);
}

/*-----------------------------------------------
 *            Main application
 * ----------------------------------------------
 *
 * Main used to launch main_application. It enables
 * testing of the all application (FPGA+GPU).
 *
 * Options that can be set using command line:
 * 	- n : Number of iterations
 * 	- s : Size of the uint32_t buffer arrays
 * 	- w : Wait time (used to emulate FPGA)
 * 	- H : Enable HOST buffering (config 1)
 * 	- v : Enable verbosity (for results checking)
 * 	- f : Enable FPGA Emulation
 *
 * WARNING ! This code only works with MAX_STREAMS=1 at this stage
 * (MAX_STREAMS is defined in includes/kernel.h)
 *
 * WARNING ! This code only works with vector_size < 131072
 * because of FPGA in-memory limitations on this version of the image.
 */

int main(int argc, char *argv[])
{
	// Init of all the default values used 
	int ch = 0;
	int card_no = 0;
	struct snap_card *card = NULL;
	struct snap_action *action = NULL;
	char device[128];
	struct snap_job cjob;
	struct parallel_memcpy_job mjob;
	const char *num_iteration = NULL;
	const char *in_size = NULL;
	uint32_t *ibuff[MAX_STREAMS];
	uint32_t *obuff[MAX_STREAMS];
	uint32_t *bufferA[MAX_STREAMS];
	uint32_t *bufferB[MAX_STREAMS];
	uint64_t addr_read = 0x0ull;
	uint64_t addr_write = 0x0ull;
	uint64_t addr_read_flag = 0x0ull;
	uint64_t addr_write_flag = 0x0ull;
	uint8_t *write_flag = NULL, *read_flag = NULL;
	struct timeval etime, stime, begin_time, end_time;
	unsigned long long int lcltime = 0x0ull;
	uint32_t type = SNAP_ADDRTYPE_HOST_DRAM;
	int max_iteration = 0, vector_size = 0;
	bool host_buffering = false, verbose = false;
	//int flags[MAX_STREAMS] = {1};
	int exit_code = EXIT_SUCCESS;
	snap_action_flag_t action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);

	// collecting the command line arguments
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{ "vector_size",	 required_argument, NULL, 's' },
			{ "num_iteration",	 required_argument, NULL, 'n' },
			{ "host_buffering",	 no_argument, NULL, 'H' },
			{ "verbose",	 no_argument, NULL, 'v' },
			{ "help", no_argument, NULL, 'h' },
			{ 0, no_argument, NULL, 0 },};		

		ch = getopt_long(argc, argv,
				"s:n:Hvh",
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
		if (vector_size>MAX_SIZE){
			printf("Vector size should smaller than %d \n",MAX_SIZE);
			exit(EXIT_FAILURE);
		}
	}

	if (num_iteration != NULL) {
		max_iteration = atoi(num_iteration);
	}


	size_t size = vector_size*sizeof(uint32_t);

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

		// Data initialization
		for (int i = 0; i < vector_size; i++){
			for (int stream = 0; stream < MAX_STREAMS; stream++){
				bufferB[stream][i] = i + 1000*stream;
			}
		}
	}

	write_flag = snap_malloc(64);
	read_flag = snap_malloc(64);
	memset(write_flag, 0x0, 64);
	memset(read_flag, 0x0, 64);

	////////////////////////////////////////////////////////////////
	//               FPGA ACTION PREPARATION
	////////////////////////////////////////////////////////////////

	// prepare params to be written in MMIO registers for action
	type  = SNAP_ADDRTYPE_HOST_DRAM;
	if (host_buffering){
		addr_read = (unsigned long)bufferB[0];
		addr_write = (unsigned long)bufferA[0];
	} else {
		addr_read = (unsigned long)obuff[0];
		addr_write = (unsigned long)ibuff[0];
	}

	addr_write_flag = (unsigned long)write_flag;
	addr_read_flag = (unsigned long)read_flag;

	// Display the parameters that will be used for the example
	printf("PARAMETERS:\n"
			"  vector_size:      %d\n"
			"  max_iteration:    %d\n"
			"  addr_read:        %016llx\n"
			"  addr_write:       %016llx\n"
			"  addr_read_flag:   %016llx\n"
			"  addr_write_flag:  %016llx\n",
			vector_size, max_iteration,
			(long long)addr_read,(long long)addr_write,
			(long long)addr_read_flag,(long long)addr_write_flag);	


	// Allocate the card that will be used
	snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", card_no);
	card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);
	if (card == NULL) {
		fprintf(stderr, "err: failed to open card %u: %s\n",
				card_no, strerror(errno));
		fprintf(stderr, "Default mode is FPGA mode.\n");
		fprintf(stderr, "Did you want to run CPU mode ? => add SNAP_CONFIG=CPU before your command.\n");
		fprintf(stderr, "Otherwise make sure you ran snap_find_card and snap_maint for your selected card.\n");
		goto out_error;
	}
	// Attach the action that will be used on the allocated card
	action = snap_attach_action(card, PARALLEL_MEMCPY_ACTION_TYPE, action_irq, 60);
	if (action == NULL) {
		fprintf(stderr, "err: failed to attach action %u: %s\n",
				card_no, strerror(errno));
		goto out_error1;
	}
	// Fill the stucture of data exchanged with the action
	snap_prepare_parallel_memcpy(&cjob, &mjob,vector_size,max_iteration,type,
			(void *)addr_read, (void *)addr_write, 
			(void *)addr_read_flag,(void *)addr_write_flag);



	/////////////////////////////////////////////////////////////////////////
	//                RUNNING FPGA ACTION
	////////////////////////////////////////////////////////////////////////
	gettimeofday(&stime, NULL);

	int rc = 0;
	rc =snap_action_sync_execute_job_set_regs(action, &cjob);
	if (rc != 0){
		printf("error while setting registers");
	}
	/* Start Action and wait for finish */
	if (verbose){
		printf("Starting FPGA action .. \n");
	}

	snap_action_start(action);

	//--- Collect the timestamp AFTER the call of the action
	gettimeofday(&etime, NULL);

	// FPGA can read vector and write buffer
	update_flag(&read_flag, 1, addr_read);
	update_flag(&write_flag, 1, addr_write);

	gettimeofday(&begin_time, NULL);

	///////////////////////////////////////////////////////////////
	//             RUNNING GPU KERNEL PIPELINING
	//////////////////////////////////////////////////////////////
	int stream =0;
	//int next_stream =0;

	for (int iteration = 0; iteration < max_iteration; iteration++){
		stream = iteration % MAX_STREAMS;	

		//FPGA is writing data in buffer
		while((read_flag[0] == 1) || (write_flag[0] == 1)){ 
			sleep(0.000002);
		}

		if (host_buffering){
			//Running kernel on GPU
			run_new_stream_v1(bufferA[stream],bufferB[stream],ibuff[stream],obuff[stream],vector_size);	   	
			// Uptdating read/write addresses for FPGA
			addr_read = (unsigned long)bufferB[stream];
			addr_write = (unsigned long)bufferA[stream];

			if (verbose){
				printf("Writting : [%d,%d, ... ,%d]\n",bufferA[0][0],bufferA[0][1],bufferA[0][vector_size-1]); 
				printf("Received : [%d,%d, ... ,%d]\n",bufferB[0][0],bufferB[0][1],bufferB[0][vector_size-1]); 
			}
		} else {
			//Running kernel on GPU
			run_new_stream_v2(ibuff[stream],obuff[stream],vector_size);
			// Updating read/write adresses for FPGA
			addr_read = (unsigned long)obuff[stream];
			addr_write = (unsigned long)ibuff[stream];

			if (verbose) {	   	
				printf("Writting : [%d,%d, ... ,%d]\n",ibuff[0][0],ibuff[0][1],ibuff[0][vector_size-1]); 
				printf("Received : [%d,%d, ... ,%d]\n",obuff[0][0],obuff[0][1],obuff[0][vector_size-1]); 
			}
		}	

		// FPGA can write new data	
		update_flag(&read_flag, 1, addr_read);
		update_flag(&write_flag, 1, addr_write);

	}

	gettimeofday(&end_time, NULL);

	switch(cjob.retc) {
		case SNAP_RETC_SUCCESS:
			fprintf(stdout, "SUCCESS\n");
			break;
		case SNAP_RETC_TIMEOUT:
			fprintf(stdout, "ACTION TIMEOUT\n");
			break;
		case SNAP_RETC_FAILURE:
			fprintf(stdout, "FAILED\n");
			fprintf(stderr, "err: Unexpected RETC=%x!\n", cjob.retc);
			break;
		default:
			break;
	}

	// Display the time of the action call
	fprintf(stdout, "SNAP registers set + action start took %lld usec\n",
			(long long)timediff_usec(&etime, &stime));

	// Display the time of the action excecution
	lcltime = (long long)(timediff_usec(&end_time, &begin_time));
	fprintf(stdout, "SNAP action average processing time for %u iteration is %f usec with config %d\n",
			max_iteration, (float)lcltime/(float)(max_iteration),
			host_buffering ? 1 : 2);

	// Detach action + disallocate the card
	snap_detach_action(action);
	snap_card_free(card);

	if (host_buffering){
		free_host(bufferA);
		free_host(bufferB);
	}
	free_device(ibuff);
	free_device(obuff);
	free(read_flag);
	free(write_flag);
	exit(exit_code);

out_error1:
	snap_card_free(card);
out_error:
	__free(bufferA);
	__free(bufferB);
	__free(read_flag);
	__free(write_flag);
	exit(EXIT_FAILURE);
}
