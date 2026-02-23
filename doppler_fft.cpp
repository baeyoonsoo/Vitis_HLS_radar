#include "doppler_fft.h"

void doppler_fft(
    hls::stream<axis_t> &in_stream,
    hls::stream<axis_t> &out_stream
) {
    #pragma HLS INTERFACE mode=axis port=in_stream
    #pragma HLS INTERFACE mode=axis port=out_stream
    #pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL

    cmpxData mem[DOPPLER_LEN][NUM_RANGE_BINS];
    #pragma HLS BIND_STORAGE variable=mem type=ram_2p impl=bram

    for (int c = 0; c < DOPPLER_LEN; c++) {
        for (int r = 0; r < NUM_RANGE_BINS; r++) {

            axis_t in_pkt = in_stream.read();
            unsigned int raw = in_pkt.data.to_uint();

            data_t real_val, imag_val;
            real_val.range(15, 0) = raw & 0xFFFF;
            imag_val.range(15, 0) = (raw >> 16) & 0xFFFF;
            
            mem[c][r] = cmpxData(real_val, imag_val);
        }
    }

    cmpxData xn[DOPPLER_LEN];
    cmpxData xk[DOPPLER_LEN];

    for (int r = 0; r < NUM_RANGE_BINS; r++) {
        
        hls::ip_fft::config_t<config_doppler> fft_config;
        hls::ip_fft::status_t<config_doppler> fft_status;
        fft_config.setSch(0x2A); 
        fft_config.setDir(1);    

        for (int c = 0; c < DOPPLER_LEN; c++) {
            xn[c] = mem[c][r]; 
        }

        hls::fft<config_doppler>(xn, xk, &fft_status, &fft_config);

        for (int c = 0; c < DOPPLER_LEN; c++) {
            #pragma HLS PIPELINE II=1
            
            axis_t out_pkt;
            cmpxData temp_out = xk[c];
            
            unsigned short out_real = (unsigned short)temp_out.real().range(15, 0);
            unsigned short out_imag = (unsigned short)temp_out.imag().range(15, 0);
            out_pkt.data = out_real | (out_imag << 16);
            
            out_pkt.keep = 0xF;
            out_pkt.strb = 0xF;

            if ((r == NUM_RANGE_BINS - 1) && (c == DOPPLER_LEN - 1)) {
                out_pkt.last = 1;
            } else {
                out_pkt.last = 0;
            }

            out_stream.write(out_pkt);
        }
    }
}