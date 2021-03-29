#pragma once

#include "head.hpp"
#include "calc_functions.hpp"

bool sameFloating(std::vector<vecType, aligned_allocator<vecType>> &v1, std::vector<vecType, aligned_allocator<vecType>> &v2, vecType tol) {
	assert(v1.size() == v2.size());
	for(size_t i = 0; i < v1.size(); i++) {
		if(std::abs(v1[i] - v2[i]) > tol) {
			return false;
		}
	}
	return true;
}

void verifycorrect(std::vector<vecType, aligned_allocator<vecType>> &input, vecType scale, std::vector<vecType, aligned_allocator<vecType>> &check) {
	std::vector<vecType, aligned_allocator<vecType>> checkAgainst(input.size());
	calc_cpu(input, checkAgainst, scale);
	bool ok = sameFloating(checkAgainst, check, 1e-10);
	if(!ok) {
		std::cerr << "Equivalence check failed" << std::endl;
        #ifdef DEBUGVERBOSE
        for(size_t i = 0; i < input.size(); i++) {
            std::cout << "(" << checkAgainst[i] << " , " << check[i] << ")" << std::endl;
        }
        #endif

		std::exit(-1);
	}
}

void printResults(std::vector<vecType, aligned_allocator<vecType>> &vec, std::vector<vecType, aligned_allocator<vecType>> &out, vecType scale) {
	for(size_t i = 0; i < out.size(); i++) {
		std::cout << "( " << out[i] << " , " << scale << " * " << vec[i] << " )" << std::endl;
	}
}