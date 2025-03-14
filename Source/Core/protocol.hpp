#include <cstdint>
#include <memory>
#include <vector>

#include "bsp_log.hpp"

extern Logger Log;

enum class PacketType : uint8_t {
    MasterToSlave = 0x00,    // 对应协议中Master2Slave
    SlaveToMaster = 0x01     // 对应协议中Slave2Master
};

// 添加 Master 到 Slave 的消息类型枚举
enum class Master2SlaveMessageID : uint8_t {
    SYNC_MSG = 0x00,               // 同步消息
    WRITE_COND_INFO_MSG = 0x01,    // 配置消息
    WRITE_RES_INFO_MSG = 0x02,     // 控制命令
    WRITE_CLIP_INFO_MSG = 0x03,    // 数据请求
    READ_DATA_MSG = 0x04,          // 数据回复
    LOCK_MSG = 0x05,               // 设备解锁
    READ_CLIP_NUM_MSG = 0x06,      // 设备状态
};

enum class Slave2MasterMessageID : uint8_t {
    COND_INFO_MSG = 0x00,      // 配置信息
    RES_INFO_MSG = 0x01,       // 控制命令
    CLIP_INFO_MSG = 0x02,      // 数据请求
    COND_DATA_MSG = 0x03,      // 数据回复
    RES_DATA_MSG = 0x04,       // 设备解锁
    CLIP_DATA_MSG = 0x05,      // 设备状态
    CLIP_NUM_MSG = 0x06,       // 设备状态
    LOCK_STATUS_MSG = 0x07,    // 设备状态
};

class FrameBase {
   public:
    virtual ~FrameBase() = default;
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual void deserialize(const std::vector<uint8_t>& data) = 0;
    virtual bool validate() const { return true; }    // 可扩展校验逻辑
};

// 通用帧头定义
struct FrameHeader {
    uint8_t delimiter[2] = {0xAB, 0xCD};
    uint8_t slot;
    uint8_t packet_id;
    uint8_t fragment_sequence;
    uint8_t more_fragments;
    uint16_t data_length;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data;
        data.push_back(delimiter[0]);
        data.push_back(delimiter[1]);
        data.push_back(slot);
        data.push_back(packet_id);
        data.push_back(fragment_sequence);
        data.push_back(more_fragments);
        data.push_back(static_cast<uint8_t>(data_length >> 8));
        data.push_back(static_cast<uint8_t>(data_length));
        return data;
    }

    void deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < 8) {
            Log.e("Invalid frame header data size");
        };

        delimiter[0] = data[0];
        delimiter[1] = data[1];
        slot = data[2];
        packet_id = data[3];
        fragment_sequence = data[4];
        more_fragments = data[5];
        data_length = (data[7] << 8) | data[6];
    }
};

// 设备状态结构体
struct DeviceStatus {
    uint16_t colorSensor : 1;
    uint16_t sleeveLimit : 1;
    uint16_t electromagnetUnlockButton : 1;
    uint16_t batteryLowPowerAlarm : 1;
    uint16_t pressureSensor : 1;
    uint16_t electromagneticLock1 : 1;
    uint16_t electromagneticLock2 : 1;
    uint16_t accessory1 : 1;
    uint16_t accessory2 : 1;
    uint16_t res : 7;
};

// 消息基类
class Message {
   public:
    virtual ~Message() = default;
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual void deserialize(const std::vector<uint8_t>& data) = 0;
};

// 同步消息（Master -> Slave）
class SyncMessage : public Message {
   public:
    uint32_t timestamp;
    explicit SyncMessage(uint32_t ts = 0) : timestamp(ts) {}

    std::vector<uint8_t> serialize() const override {
        std::vector<uint8_t> data;
        data.push_back(static_cast<uint8_t>(timestamp >> 24));
        data.push_back(static_cast<uint8_t>(timestamp >> 16));
        data.push_back(static_cast<uint8_t>(timestamp >> 8));
        data.push_back(static_cast<uint8_t>(timestamp));
        return data;
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 4) {
            Log.e("SyncMessage: Invalid SyncMessage data size");
        }
        timestamp =
            (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        Log.d("SyncMessage: timestamp =  0x%08X", timestamp);
    }
};

// 导通数据消息（Slave -> Master）
class ConductionDataMessage : public Message {
    DeviceStatus status;
    std::vector<uint8_t> conduction_data;

   public:
    std::vector<uint8_t> serialize() const override {
        std::vector<uint8_t> data;
        // 序列化status和data...
        return data;
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        // 反序列化逻辑...
    }
};

// 其他消息类型的类似实现...

// class WirelessFrame : public FrameBase {
//     FrameHeader header;
//     std::unique_ptr<Message> payload;

//    public:
//     explicit WirelessFrame(PacketType type) {
//         header.packet_id = static_cast<uint8_t>(type);
//     }

//     std::vector<uint8_t> serialize() const override {
//         auto header_data = header.serialize();
//         auto payload_data = payload->serialize();

//         header.data_length = static_cast<uint16_t>(payload_data.size());
//         header_data = header.serialize();    // 更新长度

//         header_data.insert(header_data.end(), payload_data.begin(),
//                            payload_data.end());
//         return header_data;
//     }

//     void deserialize(const std::vector<uint8_t>& data) override {
//         header.deserialize(data);

//         auto payload_start = data.begin() + 8;
//         auto payload_end = payload_start + header.data_length;

//         switch (static_cast<PacketType>(header.packet_id)) {
//             case PacketType::MasterToSlave:
//                 // payload = std::make_unique<MasterMessage>();
//                 break;
//             case PacketType::SlaveToMaster:
//                 // payload = std::make_unique<SlaveMessage>();
//                 break;
//             default:
//                 // printf("Unknown packet type!\n");
//         }

//         payload->deserialize(std::vector<uint8_t>(payload_start,
//         payload_end));
//     }

//     template <typename T>
//     T& getPayload() {
//         return dynamic_cast<T&>(*payload);
//     }
// };

// class MessageFactory {
//    public:
//     static std::unique_ptr<Message> create(Master2SlaveMessageID type) {
//         switch (type) {
//             case Master2SlaveMessageID::SYNC_MSG:
//                 return std::make_unique<SyncMessage>();
//             // case PacketType::ConductionData:
//             //     return std::make_unique<ConductionDataMessage>();
//             // 其他消息类型...
//             default:
//                 // throw std::invalid_argument("Unsupported message type");
//                 Log.e("Unsupported message type");
//         }
//     }
// };

class FrameParser {
   public:
    std::unique_ptr<Message> parse(const std::vector<uint8_t>& raw) {
        // 第一阶段：帧头解析
        FrameHeader header;
        header.deserialize(raw);
        Log.d("FrameParser: header parsed, type=0x%02X, len=%d",
              header.packet_id, header.data_length);

        // 第二阶段：数据完整性验证
        if (raw.size() < 8 + header.data_length) {
            Log.e("FrameParser: invalid frame, expected=%d, actual=%d",
                  8 + header.data_length, raw.size());
            return nullptr;
        }

        // 第三阶段：负载提取
        auto payload_start = raw.begin() + 8;
        auto payload_end = payload_start + header.data_length;
        std::vector<uint8_t> payload(payload_start, payload_end);
        Log.d("FrameParser: payload extracted, len=%d", payload.size());

        // 第四阶段：动态消息解析
        switch (static_cast<PacketType>(header.packet_id)) {
            case PacketType::MasterToSlave:
                Log.d("FrameParser: parsing MasterToSlave message");
                return parseMasterMessage(payload);
            case PacketType::SlaveToMaster:
                Log.d("FrameParser: parsing SlaveToMaster message");
                // return parseSlaveMessage(payload);
            default:
                Log.e("FrameParser: unknown packet type=0x%02X",
                      header.packet_id);
                return nullptr;
        }
    }

   private:
    std::unique_ptr<Message> parseMasterMessage(
        const std::vector<uint8_t>& data) {
        if (data.empty()) {
            Log.e("FrameParser: empty Master message");
            return nullptr;
        }

        auto msgType = static_cast<Master2SlaveMessageID>(data[0]);
        Log.d("FrameParser: parsing Master message, type=0x%02X",
              static_cast<uint8_t>(msgType));

        uint32_t targetID =
            (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
        Log.d("FrameParser: targetID extracted: 0x%08X", targetID);

        auto payload = std::vector<uint8_t>(data.begin() + 5, data.end());
        Log.d("FrameParser: payload extracted, len=%d", payload.size());

        switch (msgType) {
            case Master2SlaveMessageID::SYNC_MSG: {
                Log.d("FrameParser: processing SYNC_MSG message");
                auto msg = std::make_unique<SyncMessage>();
                msg->deserialize(payload);
                Log.d("FrameParser: SYNC_MSG message deserialized");
                return msg;
            }
            default:
                Log.e("FrameParser: unsupported Master message type=0x%02X",
                      static_cast<uint8_t>(msgType));
                return nullptr;
        }
    }

    std::unique_ptr<Message> parseSlaveMessage(
        const std::vector<uint8_t>& data) {
        if (data.empty()) {
            Log.e("FrameParser: empty Slave message");
            return nullptr;
        }

        auto msgType = static_cast<Slave2MasterMessageID>(data[0]);
        Log.d("FrameParser: parsing Slave message, type=0x%02X",
              static_cast<uint8_t>(msgType));

        auto payload = std::vector<uint8_t>(data.begin() + 1, data.end());
        Log.d("FrameParser: payload extracted, len=%d", payload.size());

        switch (msgType) {
            case Slave2MasterMessageID::COND_INFO_MSG: {
                auto msg = std::make_unique<ConductionDataMessage>();
                msg->deserialize(payload);
                return msg;
            }
            default:
                return nullptr;
        }
    }
};