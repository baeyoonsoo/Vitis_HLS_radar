#include "range_fft.h"

void range_fft(
    hls::stream<axis_t> &in_stream,
    hls::stream<axis_t> &out_stream,
    int num_chirps
) {
    #pragma HLS INTERFACE mode=axis port=in_stream
    #pragma HLS INTERFACE mode=axis port=out_stream
    #pragma HLS INTERFACE mode=s_axilite port=num_chirps bundle=CTRL
    #pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL

    cmpxData xn[FFT_LEN];
    cmpxData xk[FFT_LEN];

    for (int c = 0; c < num_chirps; c++) {
        hls::ip_fft::config_t<config_fft> fft_config;
        hls::ip_fft::status_t<config_fft> fft_status;

        // Config 설정
        fft_config.setSch(0x2AA); 
        fft_config.setDir(1);     

        // 1. 입력 읽기
        for (int i = 0; i < FFT_LEN; i++) {
            axis_t in_pkt = in_stream.read();
            unsigned int raw_data = in_pkt.data.to_uint();
            
            // 임시 변수 사용 (지난번 해결책 유지)
            data_t temp_real, temp_imag;
            temp_real.range(15, 0) = raw_data & 0xFFFF;
            temp_imag.range(15, 0) = (raw_data >> 16) & 0xFFFF;
            
            cmpxData temp_cmpx(temp_real, temp_imag);
            xn[i] = temp_cmpx; 
        }

        hls::fft<config_fft>(xn, xk, &fft_status, &fft_config);

        for (int i = 0; i < FFT_LEN; i++) {
        axis_t out_pkt;
        cmpxData temp_out = xk[i];
        
        unsigned short out_real = (unsigned short)temp_out.real().range(15, 0);
        unsigned short out_imag = (unsigned short)temp_out.imag().range(15, 0);
        
        out_pkt.data = out_real | (out_imag << 16);
        out_pkt.keep = 0xF; 
        out_pkt.strb = 0xF;

        // TLAST 로직
        if ((c == num_chirps - 1) && (i == FFT_LEN - 1)) {
            out_pkt.last = 1;
        } else {
            out_pkt.last = 0;
        }

        out_stream.write(out_pkt);
        }
    }
}