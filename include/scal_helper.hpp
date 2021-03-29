#pragma once

#include "head.hpp"

/*
Divides a contigous block of memory into comp_dev blocks,
where each block is a multiple of CHUNK_SIZE (in bytes) except the last one.
If the block is smaller than comp_dev * CHUNK_SIZE, it will create less blocks.
Note that it is necessary that CHUNK_SIZE is dividable by sizeof(T)

Obviously vector must not be modified after creating this. (Or split must be called again)
*/
template<typename T>
class MemorySplit {
public:
    MemorySplit() {
        assert(CHUNK_SIZE % sizeof(T) == 0);
    }

	MemorySplit(std::vector<T, aligned_allocator<T>> &vec, const size_t &comp_dev) {
        MemorySplit();
		split(vec, comp_dev);
    }

    void split(std::vector<T, aligned_allocator<T>> &vec, const size_t &comp_dev) {
        std::vector<size_t> perDevice(comp_dev);
        begin.reserve(comp_dev);
        blocksize.reserve(comp_dev);
        begin.resize(0);
        blocksize.resize(0);

        const size_t totalsize = vec.size();
        const size_t localChunkSize = CHUNK_SIZE / sizeof(T);
        const size_t chunks_down = totalsize / localChunkSize;
        const size_t partsize = chunks_down / comp_dev;
	    const size_t numadd1 = chunks_down % comp_dev;
        T *ptr = vec.data();
        for(size_t i = 0; i < comp_dev; i++) {
            size_t current = partsize;
            if(i < numadd1) {
			    current++;
		    }
            current = current * localChunkSize;
            if(current > 0 || (i == comp_dev - 1 && totalsize % localChunkSize > 0)) {
                begin.push_back(ptr);
                blocksize.push_back(current);
                ptr += current;
			}
        }
        blocksize.back() += totalsize % localChunkSize;

		#ifdef DEBUGVERBOSE

		std::cout << "Created, size: "  << countBlocks() << " start at " << vec.data() << " len " << vec.size() * sizeof(T) << std::endl;
		for(size_t i = 0; i < countBlocks(); i++) {
			std::cout << "Block " << i << ": (" << begin[i] << " , " << blocksize[i] << ")" << std::endl;
		}
		#endif
    }

    size_t countBlocks() {
        return begin.size();
    }

    T* data(size_t index) {
        return begin[index];
    }

    size_t size(size_t index) {
        return this->blocksize[index];
    }

private:
    std::vector<T*> begin;
    std::vector<size_t> blocksize;
};

/*
never tested btw

void splitMemory(std::vector<size_t> &result, size_t totalsize, size_t comp_dev) {
	std::vector<size_t> perDevice(comp_dev);
	const unsigned int kernelpipeline = 4;
	result.resize(comp_dev);

	const size_t partsize = totalsize / comp_dev;
	const size_t numadd1 = totalsize % comp_dev;
	for(size_t i = 0; i < comp_dev; i++) {
		size_t current = partsize;
		if(i < numadd1) {
			current++;
		}
		perDevice[i] = current;
	}
}

void concatenateMemory(std::vector<vecType, aligned_allocator<vecType>> &target, const std::vector<vecType*> &parts, 
	const std::vector<size_t> chunksize, const size_t totalsize) {

	const size_t comp_dev = chunksize.size();
	target.resize(totalsize);
	size_t used = 0;
	for(size_t i = 0; i < comp_dev; i++) {
		memcpy(target.data() + used, parts[i], chunksize[i]);
		used += chunksize[i];
	}
}
*/

void loadu280fpga(cl::Device &device, cl::Context &context) {
	std::vector<cl::Device> xildevice = xcl::get_xil_devices();
	bool success = false;
	for(size_t i = 0; i < xildevice.size(); i++) {
		if(xildevice[i].getInfo<CL_DEVICE_NAME>() == "xilinx_u280_xdma_201920_3") {
			 device = xildevice[i];
			 success = true;
			 break;
		}
	}
	if(!success) {		
		std::cerr << "Failed to load device" << std::endl;
		std::exit(-1);
	}
	cl_int error;
	OCL_CHECK(error, context = cl::Context(device, NULL, NULL, NULL, &error));
}

void applyProgram(cl::Device &device, cl::Context &context, cl::Program &program, std::string program_path) {
	auto buf = xcl::read_binary_file(program_path);
	cl_int err;
	OCL_CHECK(err, program = cl::Program(context, {device}, {{buf.data(), buf.size()}}, NULL, &err));
}

size_t roundUpToMultiplesOf(const size_t divisor, const size_t currentvalue) {
	size_t newsize = currentvalue;
	if(newsize % divisor != 0) {
		newsize = (newsize + divisor) - (newsize % divisor);
	}
	return newsize;
}
