#include "rd_map.h"
#include <iostream>
#include <ap_int.h>

using namespace std;

int main() {
    hls::stream<axis_t> in_stream("in_stream");
    hls::stream<axis_t> out_stream("out_stream");

    cout << ">> 1. Generating Input Data..." << endl;

    // 파이썬 환경(np.int16)과 동일하게 16비트 정수를 쏴줍니다.
    // 예: 실수부 10, 허수부 10 (고정 소수점이 아닌 생략된 정수 관점)
    ap_int<16> test_real = 10;
    ap_int<16> test_imag = 10;
    
    // Packing
    unsigned int packed_data = 0;
    packed_data |= (test_real & 0xFFFF);
    packed_data |= ((test_imag & 0xFFFF) << 16);

    // 예상 출력 Power = 10^2 + 10^2 = 200 (정수)
    unsigned int expected_pwr = 200;

    for (int r = 0; r < NUM_RANGE_BINS; r++) {
        for (int d = 0; d < DOPPLER_LEN; d++) {
            axis_t pkt;
            pkt.data = packed_data;
            pkt.keep = 0xF;
            pkt.strb = 0xF;
            pkt.last = (r == NUM_RANGE_BINS-1 && d == DOPPLER_LEN-1) ? 1 : 0;
            in_stream.write(pkt);
        }
    }

    cout << ">> 2. Running RD Map IP..." << endl;
    rd_map(in_stream, out_stream);

    cout << ">> 3. Verifying Output..." << endl;
    
    int sample_cnt = 0;
    bool pass = true;
    
    while(!out_stream.empty()) {
        axis_t out_pkt = out_stream.read();
        sample_cnt++;

        // Unpacking (HLS가 정수를 출력하므로 unsigned int로 바로 받음)
        unsigned int result_pwr = out_pkt.data;

        // 값 검증 (200 이어야 함)
        if (result_pwr != expected_pwr) {
            cout << "ERROR at sample " << sample_cnt << ": Val = " << result_pwr 
                 << " (Expected: " << expected_pwr << ")" << endl;
            pass = false;
        }

        // TLAST 검증
        if (sample_cnt == NUM_RANGE_BINS * DOPPLER_LEN) {
            if (out_pkt.last != 1) {
                cout << "ERROR: TLAST not asserted at the last sample!" << endl;
                pass = false;
            } else {
                cout << "SUCCESS: TLAST detected correctly." << endl;
            }
        } else {
            if (out_pkt.last == 1) {
                cout << "ERROR: TLAST asserted prematurely at sample " << sample_cnt << endl;
                pass = false;
            }
        }
    }

    if (pass) cout << ">> Test Passed! 🎉" << endl;
    else cout << ">> Test Failed!" << endl;

    return pass ? 0 : 1;
}