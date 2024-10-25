#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

enum wht_frame_type { TYPE_JSON, TYPE_DATA };

class FrameParser {
   public:
    static constexpr size_t header_len = 6;
    static constexpr size_t packet_max_size = 1024;
    static constexpr size_t frame_delimiter_len = 2;
    static constexpr size_t payload_size = packet_max_size - header_len;
    
    struct Frame {
        std::array<uint8_t, frame_delimiter_len> frame_delimiter;
        uint16_t frame_len;
        uint8_t frame_type;
        std::array<uint8_t, payload_size> data_pad;
    };
    void FrameParse(std::vector<uint8_t> data);

   private:
    void FrameProc(Frame *frame);
};