#
# Copyright 2017 International Business Machines
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


ifndef SNAP_ROOT
$(error "SNAP_ROOT not set")
endif

ifeq (, $(shell which capi-flash-script))
$(error "capi-flash-card not installed")
endif

CFLAGS = -std=c99 -I$(SNAP_ROOT)/software/include -W -Wall -Werror -Wwrite-strings -Wextra -O2 -g
CFLAGS += -Wmissing-prototypes -D_GNU_SOURCE=1
LDLIBS += -lsnap -lcxl -lpthread -lcudart
LDFLAGS += -Wl,-rpath,$(SNAP_ROOT)/software/lib
LDFLAGS += -L$(SNAP_ROOT)/software/lib

BIN_DIR = ../../bin
BUILD_DIR = ../../build
GPU_DIR = ../gpu
FPGA_DIR = ../fpga
HOST_DIR = .
INCLUDE_DIR = ../../include

C_SRCS := snap_gpu_example.c
C_SRCS += $(notdir $(wildcard $(FPGA_DIR)/*.c))
OBJECTS := $(addprefix $(BUILD_DIR)/,$(C_SRCS:.c=.o))

CUDA_SRCS := $(notdir $(wildcard $(GPU_DIR)/*.cu))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(CUDA_SRCS:.cu=.cu.o))

all: snap_gpu_example

### Rules to build final executable
snap_gpu_example: $(OBJECTS) $(libs) $(LIBS)
	@echo " Linking all files to generate executable .."
	@$(CC) $(LDFLAGS) -I $(INCLUDE_DIR) $(OBJECTS) $(LDLIBS) -o $(BIN_DIR)/$@

$(BUILD_DIR)/%.o: $(HOST_DIR)/%.c
	@echo " Creating HOST object files .."
	@$(CC) -c $(CPPFLAGS) -I $(INCLUDE_DIR) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(FPGA_DIR)/%.c
	@echo " Creating FPGA sw code object files .."
	@$(CC) -c $(CPPFLAGS) -I $(INCLUDE_DIR) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.cu.o : $(GPU_DIR)/%.cu
	@echo " GPU enable so we compile cuda files .."
	@nvcc --compiler-bindir=/usr/bin/gcc-4 -I $(INCLUDE_DIR) -c $< -o $@

clean distclean:
	$(RM) $(BUILD_DIR)/* $(BIN_DIR)/* $(libs)

