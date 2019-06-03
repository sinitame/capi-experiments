#ifndef __GPU_KERNEL_H__
#define __GPU_KERNEL_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void cuda_add(uint32_t *A, uint32_t *B, uint32_t *C, int N);

#ifdef __cplusplus
}
#endif
#endif
