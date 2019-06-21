# Examples of projects using FPGA and GPUs

This repository contains several projects examples showing how to make usage of FPGA accelerators and GPU devices on IBM POWER9 server. Those examples shows how to deals with these different accelerators in one main application using SNAP framework and Nvidia CUDA features (Unified memory, streams, synchronization)

## Prerequisites :
- User already familiar with SNAP (An open source framework that allows to easily take full advantage of CAPI technology on IBM POWER systems)
- User has gone through SNAP tutorials and is familiar with SNAP workflow and terminology.
- User is familiar with CUDA
- All FPGA Images have been precompiled on x86 server using SNAP
- SNAP is installed on the server
- Nvidia driver and toolkit are installed on the server


__Examples list__ 

Example        | Description           | Key Concepts / Keywords 
---------------|-----------------------|-------------------------
[simple-vector-generator/][]| Simple example showing how an FPGA and a GPU can exchange data using SNAP Framework and NVIDIA CUDA|__Key__ __Concepts__<br> - Cross compilation <br> - SNAP Framework utilization<br> - CUDA kernel definition and memory copy handling
[read-write-example/][]| This example shows different ways to efficiently exchange data between an FPGA and a GPU and to measure performance of each methods|__Key__ __Concepts__<br> - Cross compilation <br> - CUDA Streams <br> - Asynchronous memory copy and processing


## Projects structure

```
project/
  ├── README.md
  ├── Makefile
  ├── include/
  │   ├── kernel.h
  │   └── ...
  └── src/                          # Contains all project sources
      ├── fpga/                     # Contains all FPGA related sources
      |   ├── Makefile (fpga)
      │   ├── action_sw.c
      │   ├── action_runner.c       # File to run only the FPGA related part of the project
      |   └── images                # FPGA images directory
      |       ├── primary.bin
      |       └── secondary.bin     
      ├── gpu/                      # Constains all GPU related sources
      │   ├── Makefile (gpu)
      │   ├── kernel.cu
      |   └── kernel_runner.cu      # File to run only the GPU related part of the project
      └── host/                     # Contains all HOST related sources
          ├── Makefile (host)
          ├── application_main.c    # File to run the all project (FPGA + GPU + HOST)
          ├── application_1.c
          ├── application_2.c
          └── application_3.c
```

[simple-vector-generator/]:simple-vector-generator/
[read-write-example/]:read-write-example/
