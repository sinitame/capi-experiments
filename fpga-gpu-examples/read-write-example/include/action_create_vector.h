#ifndef __ACTION_PARALLEL_READ_WRITE_H__
#define __ACTION_PARALLEL_READ_WRITE_H__

/*
 * Copyright 2019 International Business Machines
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

#include <snap_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This number is unique and is declared in ~snap/ActionTypes.md */
#define PARALLEL_MEMCPY_ACTION_TYPE 0x1014100F
#define MAX_SIZE 1024*128

/* Data structure used to exchange information between action and application */
/* Size limit is 108 Bytes */
typedef struct parallel_memcpy_job {
	uint64_t vector_size;	/* input data */
	uint64_t max_iteration;
	struct snap_addr write;
	struct snap_addr read;
	struct snap_addr read_flag;
	struct snap_addr write_flag;
} parallel_memcpy_job_t;

#ifdef __cplusplus
}
#endif

#endif	/* __ACTION_PARALLEL_READ_WRITE_H__ */
