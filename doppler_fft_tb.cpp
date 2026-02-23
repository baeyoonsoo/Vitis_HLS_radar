#include "doppler_fft.h"
#include <iostream>
#include <cmath>

using namespace std;

int main() {
    // 1. 스트림 선언 (이제 axis_t 타입 사용)
    hls::stream<axis_t> in_stream("in_stream");
    hls::stream<axis_t> out_stream("out_stream");

    // 2. 테스트 데이터 생성 및 입력 (Packing)
    cout << ">> Generating Input Data (Chirp-major order)..." << endl;

    // Range FFT 출력 순서: [Chirp 0 전체], [Chirp 1 전체] ... 순으로 들어옴
    for (int c = 0; c < DOPPLER_LEN; c++) {
        for (int r = 0; r < NUM_RANGE_BINS; r++) {
            
            // 테스트용 값 생성 (예: 실수부=1.0, 허수부=0.0 인 DC 신호)
            // 실제로는 사인파 등을 넣어서 주파수 분석이 되는지 봐야 하지만,
            // 여기서는 흐름(Flow) 확인용으로 간단한 값을 넣습니다.
            data_t real_val = 0.5; // 임의의 값
            data_t imag_val = 0.0;
            
            // [중요] axis_t로 변환 (Packing)
            axis_t in_pkt;
            
            // 비트 추출
            unsigned short real_bits = real_val.range(15, 0);
            unsigned short imag_bits = imag_val.range(15, 0);
            
            // 32비트 데이터 조립
            in_pkt.data = real_bits | (imag_bits << 16);
            
            // 사이드 채널 설정
            in_pkt.keep = 0xF;
            in_pkt.strb = 0xF;
            // 입력 쪽 TLAST는 보통 IP 내부 로직이 카운터 기반이면 무시되지만,
            // 규격상 마지막에 1을 줍니다.
            if (c == DOPPLER_LEN - 1 && r == NUM_RANGE_BINS - 1)
                in_pkt.last = 1;
            else
                in_pkt.last = 0;

            in_stream.write(in_pkt);
        }
    }

    // 3. DUT(Device Under Test) 실행
    cout << ">> Running Doppler FFT..." << endl;
    doppler_fft(in_stream, out_stream);

    // 4. 결과 확인 (Unpacking)
    cout << ">> Verifying Output Data (Range-major order)..." << endl;
    
    // Doppler FFT 출력 순서: [Range 0 전체], [Range 1 전체] ... (Corner Turned)
    bool error_found = false;
    
    for (int r = 0; r < NUM_RANGE_BINS; r++) {
        for (int c = 0; c < DOPPLER_LEN; c++) {
            
            if (out_stream.empty()) {
                cout << "ERROR: Output stream empty prematurely!" << endl;
                error_found = true;
                break;
            }

            axis_t out_pkt = out_stream.read();
            
            // [중요] 32비트 -> 복소수 변환 (Unpacking)
            unsigned int raw = out_pkt.data.to_uint();
            data_t out_real, out_imag;
            
            out_real.range(15, 0) = raw & 0xFFFF;
            out_imag.range(15, 0) = (raw >> 16) & 0xFFFF;
            
            // 데이터 출력 (일부만 출력해서 확인)
            if (r == 0 && c < 5) {
                cout << "Range[" << r << "] Doppler[" << c << "] = " 
                     << out_real.to_float() << " + j" << out_imag.to_float();
                
                if (out_pkt.last) cout << " [TLAST]";
                cout << endl;
            }
        }
        if (error_found) break;
    }

    if (!out_stream.empty()) {
        cout << "WARNING: Leftover data in output stream." << endl;
    }

    cout << ">> Testbench Finished." << endl;
    return 0;
}