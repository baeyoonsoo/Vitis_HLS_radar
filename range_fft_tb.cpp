#include <iostream>
#include <fstream>
#include <vector>
#include "range_fft.h"
#include <unistd.h>

int main() {
    // 파일 열기
    std::ifstream inFile("lfm_pulse_bs_raw.bin", std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open lfm_pulse_bs_raw.bin" << std::endl;
        return 1;
    }

    // 스트림 타입 변경 (cmpxData -> axis_t)
    hls::stream<axis_t> in_stream("in_stream");
    hls::stream<axis_t> out_stream("out_stream");

    short i_val, q_val;
    int count = 0;
    
    std::cout << "Loading input data..." << std::endl;

    // 파일에서 읽어서 Stream에 쓰기
    // 실제로는 NUM_CHIRPS * FFT_LEN 만큼만 읽는 것이 안전함
    while (inFile.read(reinterpret_cast<char*>(&i_val), sizeof(short))) {
        if (!inFile.read(reinterpret_cast<char*>(&q_val), sizeof(short))) break;
        
        axis_t packet;
        // 데이터 패킹: 하위 16비트 I, 상위 16비트 Q
        packet.data.range(15, 0) = (ap_int<16>)i_val;
        packet.data.range(31, 16) = (ap_int<16>)q_val;
        
        // 입력 측 TLAST는 보통 필수는 아니지만, 마지막에 1을 넣어줌 (Optional)
        packet.last = 0; 
        packet.keep = 0xF;
        
        in_stream.write(packet);
        count++;
    }
    inFile.close();
    std::cout << "Successfully loaded " << count << " samples." << std::endl;

    // IP 실행 (64 Chirps 수행)
    // 주의: 입력 데이터 개수가 (NUM_CHIRPS * FFT_LEN)보다 적으면 행(Hang)이 걸릴 수 있음
    std::cout << "Running Range FFT IP..." << std::endl;
    range_fft(in_stream, out_stream, NUM_CHIRPS);

    // 결과 읽기
    std::cout << "Reading results..." << std::endl;
    int out_count = 0;
    
    while(!out_stream.empty()) {
        axis_t packet = out_stream.read();
        out_count++;

        // 데이터 언패킹 (검증용)
        ap_int<16> r_bits = packet.data.range(15, 0);
        ap_int<16> i_bits = packet.data.range(31, 16);
        
        // ap_fixed로 변환하여 값 확인
        data_t out_r, out_i;
        out_r.range(15, 0) = r_bits;
        out_i.range(15, 0) = i_bits;
        
        if (out_count % 128 == 0) {
            std::cout << "Sample " << out_count << ": " << out_r << " + j" << out_i;
            if (packet.last) std::cout << " [TLAST Detected!]";
            std::cout << std::endl;
        }

        // TLAST 확인
        if (packet.last == 1) {
            std::cout << "Process finished. TLAST received at sample: " << out_count << std::endl;
            break; // 루프 종료
        }
    }

    return 0;
}