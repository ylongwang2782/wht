#ifndef PC_PROTOCOL_HPP
#define PC_PROTOCOL_HPP

#include <cstdint>
#include <vector>

#include "protocol.hpp"

class FrameHeader2PC {
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
    FrameHeader2PC() = default;
    ~FrameHeader2PC() = default;
    static bool checkParity(const std::vector<uint8_t>& data, bool evenParity = true) {
        if(data.empty()) return false;
        
        uint8_t parity = 0;
        for(const auto& byte : data) {
            parity ^= byte;  // 计算异或校验
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
};

class CondData2PC: private FrameHeader2PC {
   public:
    CondData2PC() = default;
    ~CondData2PC() = default;
    void pack(std::vector<uint8_t>& output, const CondDataMsg& msg,uint8_t* devID) {
        memcpy(header.devID, devID, 4);
        header.type = FrameHeader2PC::DataType::COND_DATA;
        header.status = msg.deviceStatus;
        header.payload_size = msg.conductionLength;
        FrameHeader2PC::pack(output, msg.conductionData.data(), msg.conductionLength);
    }
};
#endif
