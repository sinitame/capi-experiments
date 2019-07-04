# Read/Write buffering example using (FPGA/HOST/GPU)

## Prerequisites

In order to run this example, you will need the following :

* A Power9 server (tested on AC922)
* Nvidia CUDA Toolkit (nvcc)
* SNAP sources [SNAP official repository](https://github.com/open-power/snap)
* A CAPI2.0 compatible FPGA board already configured to support SNAP (only AD9V3 supported for now)
* [capi-utils](https://github.com/ibm-capi/capi-utils) tools installed 

To run the code, FPGA card needs to be flashed with the action binaries. Binaries files can be found in `src/fpga/images/` and the card can be fashed using capi-utils functions:

* Step 1 : capi-flash-script XX_primary.bin XX_secondary.bin
* Step 2 : go to SNAP sources git directory and run `make software` and `source snap_path.sh` to compile and add snap utils functions to the path.
* Step 3 : run `snap_maint -v` to see if FPGA action is recognised by the HOST (action type should be : 0x1014100f)

Now you are sure that the FPGA is flashed with your action ! 

## Example overview

The aim of this example is to show different methods to exchange data between a HOST, an FPGA and a GPU. Even if this code is not useful in practice it shows how to design an application with FPGA and GPU acceleration and it allows to make performance measurements with different configurations.

Application structure is discribed in this [README](https://github.com/sinitame/capi-experiments/tree/read-write-example/fpga-gpu-examples).

Different part of the application can be compiled seperatly by using the top Makefile:

* **make fpga** will compile FPGA related code that can be run with `action_runner` with the following options:
  * Vector sizes (-s)          *will define the size of FPGA buffers : size is limited by FPGA max buffer size (1024x128 with this image)*
  * Number of iterations (-n)  *will define the number of read/writes performed within a run*
  * Enable verbosity (-v)
  
* **make gpu** will compile GPU related code that can be run with `kernel_runner` with the following options:
  * Vector sizes (-s)         *will define the size of GPU buffers* 
  * Number of iterations (-n) *will define the number of iteration performed within a run*
  * Enable verbosity (-v)
  * Host buffering (-H)       *set config 1, without this option there is no HOST buffering so we are in config 2*
  * Enable fpga emulator (-f) *emulate how FPGA would behave*
  * Waiting time (-w)         *wait delay to emulate different FPGA processing time*

* **make host** will compile main application (with FPGA and GPU parts). Application can be run with `main_application` with the following options:
  * Vector sizes (-s)          *will define the size of all buffers : size is limited by FPGA max buffer size (1024x128 with this image)*
  * Number of iterations (-n)  *will define the number of iteration performed within a run*
  * Enable verbosity (-v)
  * Host buffering (-H)         *set config 1, without this option there is no HOST buffering so we are in config 2*

## Implemented configurations

These configurations illustrate different use cases. The goal is to show performance measurements with host buffering (configuration 1) 
or with direct memory copy between FPGA and GPU (configuration 2)  in an application that needs to use both accelerators.


In all configurations, FPGA reads(writes) data from(to) HOST memory or GPU memory
when read(write) flag is set to 1. Data is read(written) into internal buffers and these buffers are switched between each iteration. 
When FPGA finishes its read(write) process, (read)write flag is set to 0 to notify the HOST that data is ready.

In all configurations, when told to do so, GPU reads data in ibuff, then compute
ibuff[i]*2 and writes the result in obuff.

**Important note :** Throughput results take into consideration (FPGA flags value
checking in memory + FPGA data copy from internal buffers to HOST or GPU memory
\+ GPU processing time + FPGA flags value checking in memory + FPGA data copy from GPU or HOST memory to internal
  buffers)

### Configuration 1

![Alt text](https://raw.githubusercontent.com/sinitame/capi-experiments/master/fpga-gpu-examples/read-write-example/doc/fpga-gpu-config-1.png "Config 1 figure")

![Alt text](https://raw.githubusercontent.com/sinitame/capi-experiments/master/fpga-gpu-examples/read-write-example/doc/fpga-gpu-config-1-time-line.png "Config 1 time line")

| Mode     |Action version| Vector size (uint32_t)   | Num Iterations | Total data transfer (bytes)* | Average iteration time (us) | Throughput |
| -------- | ------------ | ------------- | -------------- | --------------------------- | --------------------------- | ---------- |
|FPGA+GPU  |  0001        | 1024          | 10000          |  4096 (4KB) x 2             |           38                |  205 MB/s  |
|FGPA+GPU  |  0001        | 1024x128      | 10000          |  524288 (0.5MB) x 2         |           443               |  2.2 GB/s  |
|GPU only  |  N/A         | 1024          | 10000          |  4096 (4KB) x 2             |           36.2              |  215 MB/s  |
|GPU only  |  N/A         | 1024x128      | 10000          |  524288 (0.5MB) x 2         |           255               |  3.8 GB/s  |


* size in bytes = vector size * sizeof(uint32_t) and x2 is because we consider bidirectional data transferts.

### Configuration 2

![Alt text](https://raw.githubusercontent.com/sinitame/capi-experiments/master/fpga-gpu-examples/read-write-example/doc/fpga-gpu-config-2.png "Config 2 figure")

![Alt text](https://raw.githubusercontent.com/sinitame/capi-experiments/master/fpga-gpu-examples/read-write-example/doc/fpga-gpu-config-2-time-line.png "Config 2 time line")

| Mode     |Action version| Vector size (uint32_t)   | Num Iterations | Total data transfer (bytes)* | Average iteration time (us) | Throughput |
|--------- | ------------ | ------------- | -------------- | --------------------------- | --------------------------- | ---------- |
|FPGA+GPU  |  0001        | 1024          | 10000          |  4096 (4KB) x 2             |           16.8              |  465 MB/s  |
|FPGA+GPU  |  0001        | 1024x128      | 10000          |  524288 (0.5MB) x 2         |           301               |  3.2 GB/s  |
|GPU only  |  N/A         | 1024          | 10000          |  4096 (4KB) x 2             |           12.4              |  630 MB/s  |
|GPU only  |  N/A         | 1024x128      | 10000          |  524288 (0.5MB) x 2         |           12.8              |  76.2 GB/s |
|FPGA only |  0001        | 1024          | 10000          |  4096 (4KB) x 2             |           4.8               |  1.6 GB/s  |
|FPGA only |  0001        | 1024x128      | 10000          |  524288 (0.5MB) x 2         |           277               |  3.5 GB/s  |

* size in bytes = vector size * sizeof(uint32_t) and x2 is because we consider bidirectional data transferts.

### Configuration 3

Pipelining with CUDA Streams and CPU threads (Not implemented yet)
