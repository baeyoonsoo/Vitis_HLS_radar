#include "rd_map.h"
#include <ap_int.h> // ap_int 사용을 위해 추가

void rd_map(
    hls::stream<axis_t> &in_stream,
    hls::stream<axis_t> &out_stream
) {
    // 포트 정의
    #pragma HLS INTERFACE axis port=in_stream
    #pragma HLS INTERFACE axis port=out_stream
    #pragma HLS INTERFACE s_axilite port=return bundle=CTRL

    cmpxData line_buff[DOPPLER_LEN];

    main_loop: for (int r = 0; r < NUM_RANGE_BINS; r++) {
        
        // 1. 읽기: Range FFT로부터 한 줄(64개)을 쫙 읽어옴
        read_loop: for (int i = 0; i < DOPPLER_LEN; i++) {
            #pragma HLS PIPELINE II=1
            axis_t in_pkt = in_stream.read();
            unsigned int raw = in_pkt.data.to_uint();
            
            data_t real_val, imag_val;
            real_val.range(15, 0) = raw & 0xFFFF;
            imag_val.range(15, 0) = (raw >> 16) & 0xFFFF;
            
            line_buff[i] = cmpxData(real_val, imag_val);
        }

        // 2. FFT Shift 및 Power 계산 후 바로 출력
        write_loop: for (int i = 0; i < DOPPLER_LEN; i++) {
    #pragma HLS PIPELINE II=1
    
    int shift_idx = (i + DOPPLER_LEN / 2) % DOPPLER_LEN;
    
    // 1. 소수점(fixed-point) 타입에서 정수 부분을 안전하게 추출
    // .to_int()를 사용하여 HLS가 알아서 정수 스케일로 변환하게 하거나
    // raw 비트 데이터를 다시 부호 있는 정수로 해석합니다.
    ap_int<16> r_val = (ap_int<16>)line_buff[shift_idx].real().range(15, 0);
    ap_int<16> i_val = (ap_int<16>)line_buff[shift_idx].imag().range(15, 0);
    
    // 2. 제곱 연산 시 64비트 공간을 임시로 사용하여 오버플로우 방지
    // r_val * r_val은 최대 32비트가 필요하므로 long long으로 계산 후 32비트로 담습니다.
    long long pwr_long = (long long)r_val * r_val + (long long)i_val * i_val;
    
    // 3. 최종적으로 32비트 unsigned int로 캐스팅 (Python에서 받을 포맷)
    unsigned int pwr = (unsigned int)(pwr_long & 0xFFFFFFFF);

    axis_t out_pkt;
    out_pkt.data = pwr; 
    out_pkt.keep = -1; 
    out_pkt.strb = -1;
    
    if ((r == NUM_RANGE_BINS - 1) && (i == DOPPLER_LEN - 1)) {
        out_pkt.last = 1;
    } else {
        out_pkt.last = 0;
    }
    out_stream.write(out_pkt);
}
    }
}