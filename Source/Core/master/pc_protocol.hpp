#ifndef PC_PROTOCOL_HPP
#define PC_PROTOCOL_HPP

#include <cstdint>
#include <vector>

#include "protocol.hpp"

class UploadFrameHeader {
   public:
    enum DataType : uint8_t {
        COND_DATA,
        RESISTANCE_DATA,
        CLIP_DATA,
    };
#pragma pack(push, 1)
    struct FrameHeader {
        uint8_t delimiter[3];
        uint8_t devID[4];        // 设备ID
        DeviceStatus status;     // 设备状态
        DataType type;           // 数据类型
        uint16_t length;         // 数据长度
        uint8_t payload_size;    // 负载大小
    };
#pragma pack(pop)
    FrameHeader header;
    UploadFrameHeader() = default;
    ~UploadFrameHeader() = default;
    static bool checkParity(const std::vector<uint8_t>& data,
                            bool evenParity = true) {
        if (data.empty()) return false;

        uint8_t parity = 0;
        for (const auto& byte : data) {
            parity ^= byte;    // 计算异或校验
        }
        // 计算奇校验位
        bool result = (__builtin_popcount(parity) & 1) == 1;
        // 如果是偶校验则取反
        return evenParity ? !result : result;
    }

    static bool checkParity(uint8_t* data, uint16_t length,
                            bool evenParity = true) {
        if (length == 0) return false;

        uint8_t parity = 0;
        for (uint16_t i = 0; i < length; ++i) {
            parity ^= data[i];    // 计算异或校验
        }
        // 计算奇校验位
        bool result = (__builtin_popcount(parity) & 1) == 1;
        // 如果是偶校验则取反
        return evenParity ? !result : result;
    }

    void pack(std::vector<uint8_t>& output, uint8_t* payload,
              uint16_t payload_size) {
        output.clear();
        header.delimiter[0] = 0xA5;
        header.delimiter[1] = 0xFF;
        header.delimiter[2] = 0xCC;
        header.length = payload_size + sizeof(header) + 1;

        output.assign(reinterpret_cast<uint8_t*>(&header),
                      reinterpret_cast<uint8_t*>(&header) + sizeof(header));
        output.insert(output.end(), payload, payload + payload_size);
        output.push_back(checkParity(output, true));
    }

    void pack(uint8_t* output, uint8_t* payload, uint16_t payload_size) {
        header.delimiter[0] = 0xA5;
        header.delimiter[1] = 0xFF;
        header.delimiter[2] = 0xCC;
        header.length = payload_size + sizeof(header) + 1;

        memcpy(output, &header, sizeof(header));
        memcpy(output + sizeof(header), payload, payload_size);
        output[sizeof(header) + payload_size] =
            checkParity(output, sizeof(header) + payload_size, true);
    }
};

class UploadCondDataFrame : private UploadFrameHeader {
   public:
    UploadCondDataFrame() = default;
    ~UploadCondDataFrame() = default;
    void pack(std::vector<uint8_t>& output, const CondDataMsg& msg,
              uint8_t* devID) {
        memcpy(header.devID, devID, 4);
        header.type = UploadFrameHeader::DataType::COND_DATA;
        header.status = msg.deviceStatus;
        header.payload_size = msg.conductionLength;
        UploadFrameHeader::pack(output, msg.conductionData.data(),
                                msg.conductionLength);
    }

    void pack(uint8_t* output, const CondDataMsg& msg, uint8_t* devID) {
        memcpy(header.devID, devID, 4);
        header.type = UploadFrameHeader::DataType::COND_DATA;
        header.status = msg.deviceStatus;
        header.payload_size = msg.conductionLength;
        UploadFrameHeader::pack(output, msg.conductionData.data(),
                                msg.conductionLength);
    }
};
#endif
