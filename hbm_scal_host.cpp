#include "include/head.hpp"
#include "include/scal_helper.hpp"
#include "include/calc_functions.hpp"
#include "include/verifyHelpers.hpp"

int main(int argc, char**argv) {
    cl::Context context;
    cl::Program program;
    cl::Device device;
    cl::Kernel kernel;
    cl::CommandQueue queue;
	size_t vsize;
	vecType scale;
	
	std::string cpumode = "cpu", fpgamode = "fpga", hbmfpgamode = "hbmfpga";

	if (argc < 2 || (argv[1] != cpumode && argc != 3)) {
        std::cout << "Call: <action: cpu/fpga/hbmfpga> <xclbin>" << std::endl;
        return -1;
    }
	std::string mode = argv[1];

	if(mode != cpumode) {
		loadu280fpga(device, context);
		applyProgram(device, context, program, argv[2]);
	}

	vsize = DATA_SIZE;
	std::srand(1);

	//No reserving memory via new, because it seems xrt expects memory to be aligned to pages
	std::vector<vecType, aligned_allocator<vecType>> vec(vsize);
	auto genfun = [](){
		return std::rand() / (RAND_MAX / 1000.0);
	};
	std::generate(&vec[0], &vec[vsize], genfun);
	//posix_memalign((void **)&out, 4096, vsize * sizeof(vecType));
	std::vector<vecType, aligned_allocator<vecType>> out(vsize);
	scale = genfun();
	
	std::cout << "Successfully setup environment" << std::endl;

	double timeneeded = 1.0;

	if(mode == cpumode) {
		calc_cpu(vec, out, scale);

		std::cout << "Print results (y, n): ";
		std::string res;
		std::cin >> res;

		if(res == "y") {
			printResults(vec, out, scale);
		}
	}
	else if(mode == fpgamode) {
		timeneeded = calc_fpga(vec, out, scale, context, program, device);

		#ifdef DEBUG
		verifycorrect(vec, scale, out);
		#endif
	}
	else if(mode == hbmfpgamode) {
		timeneeded = calc_fpgahbm(vec, out, scale, context, program, device);
		
		#ifdef DEBUG
		verifycorrect(vec, scale, out);
		#endif
	}
	else {
		std::cout << "Mode not found" << std::endl;
		return -1;
	}

	double GBs = (vec.size() * sizeof(vecType)) / (timeneeded * 1024*1024*1024);
	std::cout << std::endl << "Kernel time without time to load/unload data: " << timeneeded << " s" << std::endl
			<< "GB/s computed: " << GBs << std::endl
			<< "Hence memory access speed is " << 2*GBs << " for this program" << std::endl
			<< "Note that this value is undefined if mode==cpu" << std::endl;
	
	return 0;
}
