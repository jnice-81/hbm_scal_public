#pragma once

#include "head.hpp"
#include "scal_helper.hpp"

void calc_cpu(const std::vector<vecType, aligned_allocator<vecType>> &vec_1, std::vector<vecType, aligned_allocator<vecType>> &out_1, const vecType scale) {
	const vecType *vec = vec_1.data();
	vecType *out = out_1.data();
	const size_t vsize = vec_1.size();
	for(size_t i = 0; i < vsize; i++) {
		out[i] = scale * vec[i];
	}
}

/*
	Executes scal on the fpga. Expects vec.size() % KERNEL_PIPE = 0, and out.size() == vec.size()
*/
double calc_fpga(std::vector<vecType, aligned_allocator<vecType>> &vec,std::vector<vecType, aligned_allocator<vecType>> &out, 
	vecType scale, cl::Context &context, cl::Program &program, cl::Device &device) {

	cl_int err;

	assert(vec.size() % KERNEL_PIPE == 0);
	assert(vec.size() == out.size());

	OCL_CHECK(err, cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
	OCL_CHECK(err, cl::Kernel kernel = cl::Kernel(program, "scal", &err));

	OCL_CHECK(err, cl::Buffer vec_in(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 
			sizeof(vecType)*vec.size(), vec.data(), &err));
	OCL_CHECK(err, cl::Buffer vec_out(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, 
		sizeof(vecType)*vec.size(), out.data(), &err));

	OCL_CHECK(err, err= kernel.setArg(0, vec_in));
	OCL_CHECK(err, err= kernel.setArg(1, vec_out));
	OCL_CHECK(err, err= kernel.setArg(2, vec.size()));
	OCL_CHECK(err, err= kernel.setArg(3, scale));

	auto start = std::chrono::high_resolution_clock::now();

	OCL_CHECK(err, err = queue.enqueueMigrateMemObjects({vec_in}, 0));
	OCL_CHECK(err, err = queue.enqueueTask(kernel));
	OCL_CHECK(err, err = queue.finish());

	auto end = std::chrono::high_resolution_clock::now();
	auto timeneeded = std::chrono::duration<double>(end - start);

	OCL_CHECK(err, err = queue.enqueueMigrateMemObjects({vec_out}, CL_MIGRATE_MEM_OBJECT_HOST));
	OCL_CHECK(err, err = queue.finish());

	return timeneeded.count();
}

/*
	Executes scal on the fpga using hbm. Expects vec.size() % KERNEL_HBM_DATASIZE = 0, and out.size() == vec.size()
*/
double calc_fpgahbm(std::vector<vecType, aligned_allocator<vecType>> &vec,std::vector<vecType, aligned_allocator<vecType>> &out,
	vecType scale, cl::Context &context, cl::Program &program, cl::Device &device) {

	cl_int err;
	const size_t comp_dev = 16;

	assert(vec.size() % KERNEL_HBM_DATASIZE == 0);
	assert(vec.size() == out.size());

	MemorySplit<vecType> blocks_in(vec, comp_dev);
	MemorySplit<vecType> blocks_out(out, comp_dev);

	std::vector<cl::Buffer> vec_in(blocks_in.countBlocks());
	std::vector<cl::Buffer> vec_out(blocks_in.countBlocks());
	std::vector<cl::Kernel> kernels(blocks_in.countBlocks());
	std::vector<cl_mem_ext_ptr_t> inbanks(blocks_in.countBlocks());
	std::vector<cl_mem_ext_ptr_t> outbanks(blocks_in.countBlocks());

	#ifdef DEBUG
		OCL_CHECK(err, cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));
	#else
		OCL_CHECK(err, cl::CommandQueue queue(context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));
	#endif

	for(size_t i = 0; i < blocks_in.countBlocks(); i++) {
        cl_mem_ext_ptr_t bank_in, bank_out;
        bank_in.obj = blocks_in.data(i);
        bank_in.param = 0;
        bank_in.flags = (2*i) | XCL_MEM_TOPOLOGY;
        bank_out.obj = blocks_out.data(i);
        bank_out.param = 0;
        bank_out.flags = (2*i+1) | XCL_MEM_TOPOLOGY;
		inbanks[i] = bank_in;
		outbanks[i] = bank_out;

		size_t thisKernelDataSize = blocks_in.size(i) / SIZEDIV;

		OCL_CHECK(err, vec_in[i] = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX, 
			sizeof(vecType)*blocks_in.size(i), inbanks.data() + i, &err));
		OCL_CHECK(err, vec_out[i] = cl::Buffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX, 
			sizeof(vecType)*blocks_in.size(i), outbanks.data() + i, &err));

        std::string kernel_name = std::string("scal") + ":{" + "scal_" + std::to_string(i+1) + "}";
		OCL_CHECK(err, kernels[i] = cl::Kernel(program, kernel_name.c_str(), &err));

		OCL_CHECK(err, err= kernels[i].setArg(0, vec_in[i]));
		OCL_CHECK(err, err= kernels[i].setArg(1, vec_out[i]));
		OCL_CHECK(err, err= kernels[i].setArg(2, thisKernelDataSize));
		OCL_CHECK(err, err= kernels[i].setArg(3, scale));

		#ifdef DEBUGVERBOSE
		std::cout << "Kernel " << i << " ready" << std::endl 
		<< "with args thisKernelDataSize=" << thisKernelDataSize << " , scale=" << scale << std::endl << std::endl;
		#endif

		OCL_CHECK(err, err = queue.enqueueMigrateMemObjects({vec_in[i]}, 0));
	}
	OCL_CHECK(err, err = queue.finish());

	std::cout << "Starting actual computation" << std::endl;
	size_t countIter = blocks_in.countBlocks();
	auto start = std::chrono::high_resolution_clock::now();
	for(size_t i = 0; i < countIter; i++) {
		OCL_CHECK(err, err = queue.enqueueTask(kernels[i]));
	}
	OCL_CHECK(err, err = queue.finish());
	auto end = std::chrono::high_resolution_clock::now();
	auto timeneeded = std::chrono::duration<double>(end - start);
	std::cout << "Computation done" << std::endl;

	for(size_t i = 0; i < blocks_out.countBlocks(); i++) {
		OCL_CHECK(err, err = queue.enqueueMigrateMemObjects({vec_out[i]}, CL_MIGRATE_MEM_OBJECT_HOST));
	}
	OCL_CHECK(err, err = queue.finish());

	return timeneeded.count();
}