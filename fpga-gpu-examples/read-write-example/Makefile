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

BIN_DIR = bin
BUILD_DIR = build
GPU_DIR = src/gpu
FPGA_DIR = src/fpga
HOST_DIR = src/host
INCLUDE_DIR = include


all: fpga gpu host

fpga:
	@if [ -d src/$@ -a -f src/$@/Makefile ]; then			\
		$(MAKE) -C src/$@ || exit 1;			\
	else							\
		echo "INFO: No Makefile available in $@ ...";	\
	fi

gpu:
	@if [ -d src/$@ -a -f src/$@/Makefile ]; then			\
		$(MAKE) -C src/$@ || exit 1;			\
	else							\
		echo "INFO: No Makefile available in $@ ...";	\
	fi

host:
	@if [ -d src/$@ -a -f src/$@/Makefile ]; then			\
		$(MAKE) -C src/$@ || exit 1;			\
	else							\
		echo "INFO: No Makefile available in $@ ...";	\
	fi

clean distclean:
	$(RM) $(BUILD_DIR)/* $(BIN_DIR)/* $(libs)

