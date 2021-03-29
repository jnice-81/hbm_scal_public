#define NUM_32B 16  //One channel has 256 bits (or 512???)
#define NUM_BURST 256

/*
struct floatarrayarg
{
    float infloats[NUM_32B];
};

If programming the exact same procedure using floatarrayarg, as parameter accessing all values
in parallel, it complains that one cannot access the same parameter several times in the same clock
cycle. However it says even in the manual that it is recommended to use all the width available,
usually 512 bits. ???? Why is this happening
*/

extern "C" {
    void scal(float *in, float *out, const unsigned long long size, const float scale) {
        /*
            For future reference: Each m_axi port has a seperate read and write port, so
            one can be used for one read arg and one write arg without lost performance.
            If several read/writes are needed consider adding more via bundle argument.

            The scalar variables get mapped to s_axelite ports.
        */
        #pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem max_read_burst_length = 256 max_read_outstanding=1
        #pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem max_write_burst_length = 256 max_write_outstanding=1

        burst_loop:
        for(unsigned long long b = 0; b < size; b += NUM_BURST) {
            #pragma HLS pipeline rewind II=16

            pipeline_loop: 
            for(unsigned long long i = 0; i < NUM_BURST; i += NUM_32B) {
                //#pragma HLS pipeline II=1 rewind

                parallel_loop:
                for(unsigned int j = 0; j < NUM_32B; j++) {
                    #pragma HLS unroll skip_exit_check
                    out[b+i+j] = scale * in[b+i+j];
                }
            }
        }
    }
}
