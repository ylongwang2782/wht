#include "json_interface.h"

#include "cJSON.h"


void FrameParser::FrameParse(std::vector<uint8_t> data) {
    static std::vector<uint8_t> serial_recv_buff;
    static size_t serial_recv_buff_len = 0;

    serial_recv_buff.insert(serial_recv_buff.end(), data.begin(), data.end());
    serial_recv_buff_len += data.size();

    // parse all frames in the buffer
    size_t offset = 0;
    while (offset + FrameParser::header_len <=
           serial_recv_buff_len) {  // At least one frame header
        auto *frame =
            reinterpret_cast<Frame *>(serial_recv_buff.data() + offset);

        if (frame->frame_delimiter[0] == 0xAB &&
            frame->frame_delimiter[1] == 0xCD) {
            uint8_t frame_len = frame->frame_len;
            if (offset + frame_len <= serial_recv_buff_len) {
                FrameParser::FrameProc(frame);
                offset += frame_len;
            } else {
                break;
            }
        } else {
            offset++;
        }
    }

    // move remaining data to the beginning of the buffer
    if (offset < serial_recv_buff_len) {
        std::memmove(serial_recv_buff.data(), serial_recv_buff.data() + offset,
                     serial_recv_buff_len - offset);
    }
    serial_recv_buff_len -= offset;
    serial_recv_buff.resize(serial_recv_buff_len);  // 调整缓冲区大小
}

void FrameParser::FrameProc(Frame *frame) {
    uint8_t frame_type = frame->frame_type;

    if (frame_type == TYPE_JSON) {
        // 处理JSON数据
        printf("receive json data\n");

        cJSON *result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, "config", "success");
        cJSON_AddStringToObject(result, "control", "enable");

        cJSON *root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "result", result);

        char *reply = cJSON_PrintUnformatted(root);
        printf("%s", reply);

        cJSON_Delete(root);
        free(reply);
    }
}