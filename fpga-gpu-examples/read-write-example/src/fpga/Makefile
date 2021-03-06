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

CFLAGS = -std=c99 -I$(SNAP_ROOT)/software/include -W -Wall -Werror -Wwrite-strings -Wextra -O2 -g
CFLAGS += -Wmissing-prototypes -D_GNU_SOURCE=1
LDLIBS += -lsnap -lcxl -lpthread
LDFLAGS += -Wl,-rpath,$(SNAP_ROOT)/software/lib
LDFLAGS += -L$(SNAP_ROOT)/software/lib

BIN_DIR = ../../bin
BUILD_DIR = ../../build
GPU_DIR = ../gpu
FPGA_DIR = .
HOST_DIR = ../host
INCLUDE_DIR = ../../include


C_SRCS += $(notdir $(wildcard $(FPGA_DIR)/*.c))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(C_SRCS:.c=.o))

all: $(BUILD_DIR) $(BIN_DIR) action_runner

### Rules to build final executable
action_runner: $(OBJECTS) $(libs) $(LIBS)
	@echo " Linking all FPGA action files to generate action_runner executable .."
	@$(CC) $(LDFLAGS) -I $(INCLUDE_DIR) $(OBJECTS) $(LDLIBS) -o $(BIN_DIR)/$@

$(BUILD_DIR)/%.o: $(FPGA_DIR)/%.c
	@echo " Creating FPGA action SW/HW object files .."
	@$(CC) -c $(CPPFLAGS) -I $(INCLUDE_DIR) $(CFLAGS) $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

$(BIN_DIR):
	@mkdir -p $@

clean distclean:
	$(RM) $(OBJECTS) $(BIN_DIR)/action_runner $(libs)

