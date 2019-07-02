# Read/Write buffering example using (FPGA/HOST/GPU)

## Prerequisites

In order to run this example, you will need the following :

* A Power9 server (tested on AC922)
* Nvidia cuda compiler (nvcc)
* SNAP sources [SNAP official repository](https://github.com/open-power/snap)
* A CAPI2.0 compatible FPGA board already configured to support SNAP (only AD9V3 supported for now)

## Example overview

The aim of this example is to show different methods to exchange data between a HOST, an FPGA and a GPU. Even if this code is not useful in practice it shows how to design an application with FPGA and GPU acceleration and it allows to make performance measurements with different configuration.

Application is structure is discribed in this [README](https://github.com/sinitame/capi-experiments/tree/read-write-example/fpga-gpu-examples).

Different part of the application can be compiled seperatly by using the top Makefile:

* **make fpga** will compile FPGA related code that can be run with `action_runner` with the following options:
  * Buffer sizes (-s)
  * Number of iterations (-n)
  * Enable verbosity (-v)
  
* **make gpu** will compile GPU related code that can be run with `kernel_runner` with the following options:
  * Buffer sizes (-s)
  * Number of iterations (-n)
  * Enable verbosity (-v)
  * Host buffering (-H) *(config 1 : default is config 2)*
  * Enable fpga emulator (-f) *emulate how FPGA would behave*
  * Waiting time (-w) *wait to emulate different FPGA processing time*

* **make host** will compile main application (with FPGA and GPU parts). Application can be run with `main_application` with the following options:
  * Buffer sizes (-s)
  * Number of iterations (-n)
  * Enable verbosity (-v)
  * Host buffering (-H) *(config 1: default is config 2)*

## Implemented configurations

These configurations illustrates different use cases. The goal is to show performance measurements with direct memory copy between FPGA and GPU (configuration 2) and with host buffering (configuration 1) in an application that needs to use both accelerators.


### Configuration 1

![Alt text](https://raw.githubusercontent.com/sinitame/capi-experiments/read-write-example/fpga-gpu-examples/read-write-example/doc/fpga-gpu-config-1.png "Config 1 figure")

![Alt text](https://raw.githubusercontent.com/sinitame/capi-experiments/read-write-example/fpga-gpu-examples/read-write-example/doc/fpga-gpu-config-1-time-line.png "Config 1 time line")



### Configuration 2

![Alt text](https://raw.githubusercontent.com/sinitame/capi-experiments/read-write-example/fpga-gpu-examples/read-write-example/doc/fpga-gpu-config-2.png "Config 2 figure")

![Alt text](https://raw.githubusercontent.com/sinitame/capi-experiments/read-write-example/fpga-gpu-examples/read-write-example/doc/fpga-gpu-config-2-time-line.png "Config 2 time line")


### Configuration 3
