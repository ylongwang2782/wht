#include <cstdint>
#include <cstdio>
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
    WRITE_RES_INFO_MSG = 0x02,     // 写入阻值信息
    WRITE_CLIP_INFO_MSG = 0x03,    // 写入卡钉信息
    READ_DATA_MSG = 0x04,          // 读取数据
    LOCK_MSG = 0x05,               // 锁
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
    static constexpr uint8_t FRAME_DELIMITER[2] = {
        0xAB, 0xCD};    // 使用常量表示帧分隔符
    uint8_t slot;
    uint8_t packet_id;
    uint8_t fragment_sequence;
    uint8_t more_fragments_flag;
    uint16_t data_length;

    void serialize(std::vector<uint8_t>& data) const {
        data.clear();  // 清空传入的vector
        data.push_back(FRAME_DELIMITER[0]);
        data.push_back(FRAME_DELIMITER[1]);
        data.push_back(slot);
        data.push_back(packet_id);
        data.push_back(fragment_sequence);
        data.push_back(more_fragments_flag);
        data.push_back(static_cast<uint8_t>(data_length >> 8));  // 高字节在前
        data.push_back(static_cast<uint8_t>(data_length));       // 低字节在后
    }

    bool deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < 8) {
            Log.e("Invalid frame header data size");
            return false;
        }

        // 验证帧分隔符
        if (data[0] != FRAME_DELIMITER[0] || data[1] != FRAME_DELIMITER[1]) {
            Log.e("Invalid frame delimiter");
            return false;
        }

        slot = data[2];
        packet_id = data[3];
        fragment_sequence = data[4];
        more_fragments_flag = data[5];
        data_length = (data[7] << 8) | data[6];
        return true;
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
    virtual void serialize(std::vector<uint8_t>& data) const = 0;
    virtual void deserialize(const std::vector<uint8_t>& data) = 0;
    virtual void process() = 0;
};

// 同步消息（Master -> Slave）
class SyncMsg : public Message {
   public:
    uint8_t mode;
    uint32_t timestamp;
    explicit SyncMsg(uint8_t m = 0, uint32_t ts = 0) : mode(m), timestamp(ts) {}

    void serialize(std::vector<uint8_t>& data) const override {
        data.clear();    // 清空传入的vector
        data.push_back(mode);
        data.push_back(static_cast<uint8_t>(timestamp >> 24));
        data.push_back(static_cast<uint8_t>(timestamp >> 16));
        data.push_back(static_cast<uint8_t>(timestamp >> 8));
        data.push_back(static_cast<uint8_t>(timestamp));
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 5) {
            Log.e("SyncMsg: Invalid SyncMsg data size");
        }
        mode = data[0];
        timestamp =
            (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
        Log.d("SyncMsg: mode = 0x%02X, timestamp = 0x%08X", mode, timestamp);
    }

    void process() override { Log.d("SyncMsg process"); };
};

class WriteCondInfoMsg : public Message {
   public:
    uint8_t timeSlot;
    uint16_t totalConductionNum;
    uint16_t startConductionNum;
    uint16_t conductionNum;

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(timeSlot);
        data.push_back(static_cast<uint8_t>(totalConductionNum >> 8));
        data.push_back(static_cast<uint8_t>(totalConductionNum));
        data.push_back(static_cast<uint8_t>(startConductionNum >> 8));
        data.push_back(static_cast<uint8_t>(startConductionNum));
        data.push_back(static_cast<uint8_t>(conductionNum >> 8));
        data.push_back(static_cast<uint8_t>(conductionNum));
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 7) {
            Log.e("WriteCondInfoMsg: Invalid WriteCondInfoMsg data size");
        }
        timeSlot = data[0];
        totalConductionNum = (data[2] << 8) | data[1];
        startConductionNum = (data[4] << 8) | data[3];
        conductionNum = (data[6] << 8) | data[5];
        Log.d(
            "WriteCondInfoMsg: timeSlot = 0x%02X, totalConductionNum "
            "= 0x%04X, startConductionNum = 0x%04X, conductionNum = 0x%04X",
            timeSlot, totalConductionNum, startConductionNum, conductionNum);
    }
    void process() override { Log.d("WriteCondInfoMsg process"); };
};

class WriteResInfoMsg : public Message {
   public:
    uint8_t timeSlot;
    uint16_t totalResistanceNum;
    uint16_t startResistanceNum;
    uint16_t resistanceNum;

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(timeSlot);
        data.push_back(static_cast<uint8_t>(totalResistanceNum >> 8));
        data.push_back(static_cast<uint8_t>(totalResistanceNum));
        data.push_back(static_cast<uint8_t>(startResistanceNum >> 8));
        data.push_back(static_cast<uint8_t>(startResistanceNum));
        data.push_back(static_cast<uint8_t>(resistanceNum >> 8));
        data.push_back(static_cast<uint8_t>(resistanceNum));
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 7) {
            Log.e("WriteResInfoMsg: Invalid WriteResInfoMsg data size");
        }
        timeSlot = data[0];
        totalResistanceNum = (data[2] << 8) | data[1];
        startResistanceNum = (data[4] << 8) | data[3];
        resistanceNum = (data[6] << 8) | data[5];
        Log.d(
            "WriteResInfoMsg: timeSlot = 0x%02X, totalResistanceNum = 0x%04X, "
            "startResistanceNum = 0x%04X, resistanceNum = 0x%04X",
            timeSlot, totalResistanceNum, startResistanceNum, resistanceNum);
    }
    void process() override { Log.d("WriteResInfoMsg process"); };
};

class WriteClipInfoMsg : public Message {
   public:
    uint16_t clipPin;    // 16 个卡钉激活信息，激活的位置 1，未激活的位置 0

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(static_cast<uint8_t>(clipPin));    // 低字节在前
        data.push_back(static_cast<uint8_t>(clipPin >> 8));    // 高字节在后
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 2) {
            Log.e("WriteClipInfoMsg: Invalid WriteClipInfoMsg data size");
        }
        clipPin = data[0] | (data[1] << 8);    // 低字节在前，高字节在后
        Log.d("WriteClipInfoMsg: clipPin = 0x%04X", clipPin);
    }

    void process() override { Log.d("WriteClipInfoMsg process"); };
};

class ReadDataMsg : public Message {
   public:
    enum class ReadType : uint8_t {
        CONDUCTION_DATA = 0,    // 读取导通数据
        RESISTANCE_DATA = 1,    // 读取阻值数据
        CLIP_DATA = 2,          // 读取卡钉数据
        CLIP_INFO = 3           // 读取卡钉信息
    };

    ReadType type;

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(static_cast<uint8_t>(type));
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 1) {
            Log.e("ReadDataMsg: Invalid ReadDataMsg data size");
        }
        type = static_cast<ReadType>(data[0]);
        const char* msgTypeStr = "Unknown";
        switch (type) {
            case ReadType::CONDUCTION_DATA:
                msgTypeStr = "CONDUCTION_DATA";
                break;
            case ReadType::RESISTANCE_DATA:
                msgTypeStr = "RESISTANCE_DATA";
                break;
            case ReadType::CLIP_DATA:
                msgTypeStr = "CLIP_DATA";
                break;
            case ReadType::CLIP_INFO:
                msgTypeStr = "CLIP_INFO";
                break;
            default:
                break;
        }
        Log.d("ReadDataMsg: type=%s (0x%02X)", msgTypeStr, type);
    }
    void process() override { Log.d("ReadDataMsg process"); };
};

class InitMsg : public Message {
   public:
    uint8_t lock;
    uint16_t clipLed;    // 新增卡钉灯位初始化信息

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(lock);
        // 新增 clipLed 序列化
        data.push_back(static_cast<uint8_t>(clipLed));         // 低字节
        data.push_back(static_cast<uint8_t>(clipLed >> 8));    // 高字节
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 3) {    // 修改为3字节
            Log.e("InitMsg: Invalid InitMsg data size");
        }
        lock = data[0];
        // 新增 clipLed 反序列化
        clipLed = data[1] | (data[2] << 8);    // 低字节在前，高字节在后
        Log.d("InitMsg: lock = 0x%02X, clipLed = 0x%04X", lock, clipLed);
    }
    void process() override { Log.d("InitMsg process"); };
};

// 导通数据消息（Slave -> Master）
class ConductionDataMessage : public Message {
    DeviceStatus status;
    std::vector<uint8_t> conduction_data;

   public:
    void serialize(std::vector<uint8_t>& data) const override {
        // 序列化status和data...
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        // 反序列化逻辑...
    }
    void process() override;
};

// 其他消息类型的类似实现...

// class WirelessFrame : public FrameBase {
//     FrameHeader header;
//     std::unique_ptr<Message> payload;

//    public:
//     explicit WirelessFrame(PacketType type) {
//         header.packet_id = static_cast<uint8_t>(type);
//     }

//     void serialize(std::vector<uint8_t>& data) const override {
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
//                 return std::make_unique<SyncMsg>();
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
        const char* msgTypeStr = "Unknown";
        switch (msgType) {
            case Master2SlaveMessageID::SYNC_MSG:
                msgTypeStr = "SYNC_MSG";
                break;
            case Master2SlaveMessageID::WRITE_COND_INFO_MSG:
                msgTypeStr = "WRITE_COND_INFO_MSG";
                break;
            case Master2SlaveMessageID::WRITE_RES_INFO_MSG:
                msgTypeStr = "WRITE_RES_INFO_MSG";
                break;
            case Master2SlaveMessageID::WRITE_CLIP_INFO_MSG:
                msgTypeStr = "WRITE_CLIP_INFO_MSG";
                break;
            case Master2SlaveMessageID::READ_DATA_MSG:
                msgTypeStr = "READ_DATA_MSG";
                break;
            case Master2SlaveMessageID::LOCK_MSG:
                msgTypeStr = "LOCK_MSG";
                break;
            default:
                break;
        }
        Log.d("FrameParser: parsing Master message, type=%s (0x%02X)",
              msgTypeStr, static_cast<uint8_t>(msgType));

        uint32_t targetID =
            (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
        if (targetID != 0xFFFFFFFF) {
            if (targetID != 0x3732485B) {
                Log.e("FrameParser: invalid targetID: 0x%08X", targetID);
                return nullptr;
            }
        }
        Log.d("FrameParser: targetID extracted: 0x%08X", targetID);

        auto payload = std::vector<uint8_t>(data.begin() + 5, data.end());
        Log.d("FrameParser: payload extracted, len=%d", payload.size());

        switch (msgType) {
            case Master2SlaveMessageID::SYNC_MSG: {
                Log.d("FrameParser: processing SYNC_MSG message");
                auto msg = std::make_unique<SyncMsg>();
                msg->deserialize(payload);
                Log.d("FrameParser: SYNC_MSG message deserialized");
                return msg;
            }
            case Master2SlaveMessageID::WRITE_COND_INFO_MSG: {
                Log.d("FrameParser: processing WRITE_COND_INFO_MSG message");
                auto msg = std::make_unique<WriteCondInfoMsg>();
                msg->deserialize(payload);
                Log.d("FrameParser: WRITE_COND_INFO_MSG message deserialized");
                return msg;
            }
            case Master2SlaveMessageID::WRITE_RES_INFO_MSG: {
                Log.d("FrameParser: processing WRITE_RES_INFO_MSG message");
                auto msg = std::make_unique<WriteResInfoMsg>();
                msg->deserialize(payload);
                Log.d("FrameParser: WRITE_RES_INFO_MSG message deserialized");
                return msg;
            }
            case Master2SlaveMessageID::WRITE_CLIP_INFO_MSG: {
                Log.d("FrameParser: processing WRITE_CLIP_INFO_MSG message");
                auto msg = std::make_unique<WriteClipInfoMsg>();
                msg->deserialize(payload);
                Log.d("FrameParser: WRITE_CLIP_INFO_MSG message deserialized");
                return msg;
            }
            case Master2SlaveMessageID::READ_DATA_MSG: {
                Log.d("FrameParser: processing READ_DATA_MSG message");
                auto msg = std::make_unique<ReadDataMsg>();
                msg->deserialize(payload);
                Log.d("FrameParser: READ_DATA_MSG message deserialized");
                return msg;
            }
            case Master2SlaveMessageID::LOCK_MSG: {
                Log.d("FrameParser: processing LOCK_MSG message");
                auto msg = std::make_unique<InitMsg>();
                msg->deserialize(payload);
                Log.d("FrameParser: LOCK_MSG message deserialized");
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