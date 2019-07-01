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

TARGET = kernel_runner

CFLAGS = -std=c99 -W -Wall -Werror -Wwrite-strings -Wextra -O2 -g
CFLAGS += -Wmissing-prototypes -D_GNU_SOURCE=1
LDLIBS += -lpthread -lcudart

BIN_DIR = ../../bin
BUILD_DIR = ../../build
GPU_DIR = .
FPGA_DIR = ../fpga
HOST_DIR = ../host
INCLUDE_DIR = ../../include

CUDA_SRCS := $(notdir $(wildcard $(GPU_DIR)/*.cu))
OBJECTS := $(addprefix $(BUILD_DIR)/,$(CUDA_SRCS:.cu=.cu.o))

C_SRCS := $(notdir $(wildcard $(GPU_DIR)/*.c))
RUNNER_OBJECTS := $(addprefix $(BUILD_DIR)/,$(C_SRCS:.c=.o))

all: runner

runner: $(TARGET)

compile_gpu: $(OBJECTS)

### Rules to build final executable
$(TARGET): $(OBJECTS) $(RUNNER_OBJECTS)
	@echo " Linking all GPU files to generate kernel_runner executable .."
	@$(CC) -I $(INCLUDE_DIR) $(LDLIBS) $^ -o $(BIN_DIR)/$@

$(BUILD_DIR)/%.o: $(GPU_DIR)/%.c 
	@echo " Creating GPU kernel_runner object files .."
	@$(CC) -I $(INCLUDE_DIR) $(CFLAGS) $(LDLIBS) -c $< -o $@

$(BUILD_DIR)/%.cu.o : $(GPU_DIR)/%.cu
	@echo " Creating CUDA object files .."
	@nvcc --default-stream per-thread -I $(INCLUDE_DIR) -c $< -o $@


$(BUILD_DIR):
	@mkdir -p $@

$(BIN_DIR):
	@mkdir -p $@


clean distclean:
	$(RM) $(OBJECTS) $(RUNNER_OBJECTS) $(BIN_DIR)/kernel_runner

