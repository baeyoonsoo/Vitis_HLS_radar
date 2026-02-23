#ifndef RD_MAP_H
#define RD_MAP_H

#include <ap_fixed.h>
#include <complex>
#include <hls_stream.h>
// [필수] Xilinx AXI Stream 표준 라이브러리
#include <ap_axi_sdata.h> 

// [핵심] 수동으로 구조체를 만들지 말고, 표준 템플릿을 사용합니다.
// <DataWidth, UserWidth, IDWidth, DestWidth>
// 32비트 데이터, 나머지는 0 (사용 안 함)
typedef ap_axis<32, 0, 0, 0> axis_t;

// 데이터 타입 (기존 유지)
typedef ap_fixed<16, 1> data_t; 
typedef std::complex<data_t> cmpxData;
typedef ap_fixed<32, 16> power_t;

const int DOPPLER_LEN = 64;      
const int NUM_RANGE_BINS = 256;  

void rd_map(
    hls::stream<axis_t> &in_stream, 
    hls::stream<axis_t> &out_stream
);

#endif