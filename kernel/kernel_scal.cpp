#define UNROLL_FACTOR 4

typedef unsigned long long size_t;
typedef float vecType;

extern "C" {
    void scal(vecType *in, vecType *out, const size_t size, const float scale) {
        for(size_t i = 0; i < size; i += UNROLL_FACTOR) {
            for(unsigned int j = 0; j < UNROLL_FACTOR; j++) {
                #pragma HLS unroll skip_exit_check
                out[i+j] = scale * in[i+j];
            }
        }
    }
}