#ifndef __GPU_KERNEL_H__
#define __GPU_KERNEL_H__
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#define MAX_STREAMS 1

#ifdef __cplusplus
extern "C" {
#endif

void memory_allocation_host(uint32_t *buffer[MAX_STREAMS], size_t size);
void memory_allocation_gpu(uint32_t *buffer[MAX_STREAMS], size_t size);
void run_new_stream(uint32_t *bufferA, uint32_t *bufferB, uint32_t *ibuff, uint32_t *obuff, int vector_size);
void free_host(uint32_t *buffer[MAX_STREAMS]);
void free_device(uint32_t *buffer[MAX_STREAMS]);

#ifdef __cplusplus
}
#endif
#endif
