.PHONY: help


help::
	@echo "TARGET=<sw_emu, hw_emu, hw> specifies the build target for the kernels"
	@echo "make all: Build host and kernels for x86, platform xilinx_u280_xdma_201920_3"
	@echo "make host: Build host"
	@echo "make simplekernel: Build scal kernel using 16 compute units on default memory"
	@echo "make hbmkernel: Build scal kernel(s) using 16 compute units on all hbm ports"
	@echo "make clean: remove some of the logs"
	@echo "make cleansimplekernel: remove simplekernel buils"
	@echo "make cleanhbmkernel: remove hbmkernel buils"
	@echo ""

HOST_ARCH := x86
PLATFORM := xilinx_u280_xdma_201920_3
PROJ_ROOT := $(PWD)/..
BUILD_DIR := $(PWD)/build
EXECUTABLE := host
TARGET := sw_emu

include $(PROJ_ROOT)/common/includes/opencl/opencl.mk
CXXFLAGS += $(opencl_CXXFLAGS) -Wall -std=c++11 -I$(PROJ_ROOT)/common/includes/xcl2

ifneq ($(TARGET), hw)
	CXXFLAGS += -O1 -g 
else
	CXXFLAGS += -O3
endif

LDFLAGS += $(opencl_LDFLAGS) -lrt -lstdc++

HOST_SRC += $(PROJ_ROOT)/common/includes/xcl2/xcl2.cpp ./hbm_scal_host.cpp
KERNEL_SIMPLE_SRC = $(PWD)/kernel/kernel_scal.cpp
KERNEL_HBM_SRC = $(PWD)/kernel/kernel_scal_hbm.cpp

CXX := g++
VXX := v++
VXXLDFLAGS += --config scal_hbm.cgf

VXXFLAGS += -t $(TARGET) --platform $(PLATFORM)
ifneq ($(TARGET), hw)
	VXXFLAGS += -g
endif

.PHONY: cleanhost
cleanhost:
	rm -f $(EXECUTABLE)

#Build host
#Note: here $@=$(EXECUTABLE), $^=$(HOST_SRC)
$(EXECUTABLE): $(HOST_SRC) cleanhost
	$(CXX) -o $@ $(HOST_SRC) $(CXXFLAGS) $(LDFLAGS)

#Build simple kernel

$(BUILD_DIR)/scal_simple.xo: $(KERNEL_SIMPLE_SRC)
	mkdir -p $(BUILD_DIR)
	$(VXX) $(VXXFLAGS) -c -k scal -o $@ $^

$(BUILD_DIR)/scal_simple.xclbin: $(BUILD_DIR)/scal_simple.xo
	$(VXX) $(VXXFLAGS) -l -o $@ $^

#Build hbm kernel

$(BUILD_DIR)/scal_hbm.xo: $(KERNEL_HBM_SRC)
	mkdir -p $(BUILD_DIR)
	$(VXX) $(VXXFLAGS) -c -k scal -o $@ $^

$(BUILD_DIR)/scal_hbm.xclbin: $(BUILD_DIR)/scal_hbm.xo
	$(VXX) $(VXXFLAGS) $(VXXLDFLAGS) -l -o $@ $^

.PHONY: hbmkernel
hbmkernel: $(BUILD_DIR)/scal_hbm.xclbin

.PHONY: simplekernel
simplekernel: $(BUILD_DIR)/scal_simple.xclbin

.PHONY: all
all: $(EXECUTABLE) simplekernel hbmkernel

.PHONY: clean
clean:
	rm $(PWD)/*.log -f
	rm $(PWD)/.run -r -f
	rm $(PWD)/.ipcache -r -f

.PHONY: cleansimplekernel
cleansimplekernel:
	rm $(BUILD_DIR)/scal_simple.*

.PHONY: cleanhbmkernel
cleanhbmkernel:
	rm $(BUILD_DIR)/scal_hbm.*

.PHONY: cleanall
cleanall: cleanhbmkernel cleanhost cleansimplekernel