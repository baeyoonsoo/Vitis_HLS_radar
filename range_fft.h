#ifndef RANGE_FFT_H
#define RANGE_FFT_H

#include <ap_fixed.h>
#include <complex>
#include <hls_stream.h>
#include <hls_fft.h>
#include <ap_axi_sdata.h> // [추가] AXI Stream 헤더

typedef ap_fixed<16, 1> data_t; 
typedef std::complex<data_t> cmpxData;

// [추가] AXI Stream 패킷 타입 정의 (Data 32bit + User 0 + ID 0 + Dest 0)
// 데이터 32bit = 16bit Real + 16bit Imag
typedef ap_axis<32, 0, 0, 0> axis_t;

const int FFT_LEN = 256;
const int NUM_CHIRPS = 64; // Default 값 (실제로는 포트로 입력받음)

struct config_fft : hls::ip_fft::params_t {
    static const bool has_ovflo = true;
    static const bool has_blk_exp = false;
    static const unsigned max_nfft = 8; 
    static const unsigned input_width = 16;
    static const unsigned output_width = 16;
    static const unsigned scaling_opt = hls::ip_fft::scaled;
    static const unsigned arch_opt = hls::ip_fft::pipelined_streaming_io;
    static const unsigned ordering_opt = hls::ip_fft::natural_order;
};

void range_fft(
    hls::stream<axis_t> &in_stream,  // [변경] 입력 타입
    hls::stream<axis_t> &out_stream, // [변경] 출력 타입
    int num_chirps
);

#endif