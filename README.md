# Examples of projects using IBM Coherent Accelerator Protocol Interface (CAPI) on POWER

This repository contains several projects examples showing how to make usage of FPGA accelerators on IBM POWER9 server. The focus of these examples is to show the different possibilities offered by IBM POWER9 server for applications that could benefit from both FPGA acceleration.

## Prerequisites :
- User already familiar with SNAP (An open source framework that allows to easily take full advantage of CAPI technology on IBM POWER systems)
- User has gone through SNAP tutorials and is familiar with SNAP workflow and terminology.
- All FPGA Images have been precompiled on x86 server using SNAP


__Examples list__ 

Example        | Description           | Key Concepts / Keywords 
---------------|-----------------------|-------------------------
[fpga-gpu-example/][]| Simple example showing how an FPGA and a GPU can exchange data using SNAP Framework and NVIDIA CUDA|__Key__ __Concepts__<br> - Cross compilation <br> - SNAP Framework utilization<br> - CUDA kernel definition and memory copy handling

[fpga-gpu-example/]:fpga-gpu-example/
