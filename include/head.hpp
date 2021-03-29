#pragma once

typedef float vecType;
#define KERNEL_PIPE 4
#define KERNEL_HBM_DATASIZE 8
#define CHUNK_SIZE 4096
#define DATA_SIZE (1024*1024*1024)  //in vecType, ie here it's 4 GB. Note that DATA_SIZE % 256 == 0 should hold, if computing using the hbm kernel
//#define DATA_SIZE (1024*32)
//#define DEBUGVERBOSE
//#define DEBUG
#define SIZEDIV 1

#include <xcl2.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>
#include <chrono>
