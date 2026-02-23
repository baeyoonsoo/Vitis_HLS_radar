#ifndef DOPPLER_FFT_H
#define DOPPLER_FFT_H

#include <ap_fixed.h>
#include <complex>
#include <hls_stream.h>
#include <hls_fft.h>
#include <ap_axi_sdata.h> // [필수]

// 표준 템플릿 사용 (TLAST 포함)
typedef ap_axis<32, 0, 0, 0> axis_t;

typedef ap_fixed<16, 1> data_t; 
typedef std::complex<data_t> cmpxData;

const int DOPPLER_LEN = 64;      
const int NUM_RANGE_BINS = 256;  

struct config_doppler : hls::ip_fft::params_t {
    static const bool has_ovflo = true;
    static const bool has_blk_exp = false;
    static const unsigned max_nfft = 6;      // 2^6 = 64 Point
    
    // [수정] 에러 해결을 위해 8비트로 강제 지정
    static const unsigned config_width = 8;  
    
    static const unsigned input_width = 16;
    static const unsigned output_width = 16;
    static const unsigned scaling_opt = hls::ip_fft::scaled;
    static const unsigned arch_opt = hls::ip_fft::radix_4_burst_io;
    static const unsigned ordering_opt = hls::ip_fft::natural_order;
};

void doppler_fft(
    hls::stream<axis_t> &in_stream,
    hls::stream<axis_t> &out_stream
);

#endif