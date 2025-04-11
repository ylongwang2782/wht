#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "bsp_log.hpp"

enum class PacketType : uint8_t {
    Master2Slave = 0x00,      // 对应协议中Master2Slave
    Slave2Master = 0x01,      // 对应协议中Slave2Master
    Backend2Master = 0x02,    // 对应协议中Backend2Master
    Master2Backend = 0x03     // 对应协议中Master2Backend
};

enum class Master2SlaveMessageID : uint8_t {
    SYNC_MSG = 0x00,              // 同步消息
    COND_CFG_MSG = 0x10,          // 写入导通信息
    RES_CFG_MSG = 0x11,           // 写入阻值信息
    CLIP_CFG_MSG = 0x12,          // 写入卡钉信息
    READ_COND_DATA_MSG = 0x20,    // 读取
    READ_RES_DATA_MSG = 0x21,     // 读取
    READ_CLIP_DATA_MSG = 0x22,    // 读取
    RST_MSG = 0x30
};

enum class Slave2MasterMessageID : uint8_t {
    COND_INFO_MSG = 0x10,      // 导通信息
    RES_INFO_MSG = 0x11,       // 阻值信息
    CLIP_INFO_MSG = 0x12,      // 卡钉信息
    COND_DATA_MSG = 0x20,      // 导通数据
    RES_DATA_MSG = 0x21,       // 阻值数据
    CLIP_DATA_MSG = 0x22,      // 卡钉数据
    RST_MSG = 0x30,    // 卡钉数量
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
    static constexpr size_t HEADER_SIZE = 8;
    uint8_t slot;
    uint8_t packet_id;
    uint8_t fragment_sequence;
    uint8_t more_fragments_flag;
    uint16_t data_length;

    // 序列化为字节流
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data;
        data.reserve(HEADER_SIZE);
        data.push_back(FRAME_DELIMITER[0]);
        data.push_back(FRAME_DELIMITER[1]);
        data.push_back(slot);
        data.push_back(packet_id);
        data.push_back(fragment_sequence);
        data.push_back(more_fragments_flag);
        data.push_back(static_cast<uint8_t>(data_length & 0xFF));    // 低位在前
        data.push_back(static_cast<uint8_t>(data_length >> 8));    // 高位在后
        return data;
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

// 消息基类
class Message {
   public:
    virtual ~Message() = default;
    virtual void serialize(std::vector<uint8_t>& data) const = 0;
    virtual void deserialize(const std::vector<uint8_t>& data) = 0;
    virtual void process() = 0;
    // 消息类型标识
    virtual uint8_t message_type() const = 0;
};

struct Master2SlavePacket {
    uint8_t message_id;              // 消息类型标识
    uint32_t destination_id;         // 目标设备 ID
    std::vector<uint8_t> payload;    // 消息的序列化数据

    // 序列化 Master2SlavePacket
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data;
        data.reserve(5 + payload.size());    // 1 + 4 + payload size
        data.push_back(message_id);
        data.push_back(static_cast<uint8_t>(destination_id >> 24));
        data.push_back(static_cast<uint8_t>(destination_id >> 16));
        data.push_back(static_cast<uint8_t>(destination_id >> 8));
        data.push_back(static_cast<uint8_t>(destination_id));
        data.insert(data.end(), payload.begin(), payload.end());
        return data;
    }

    // 反序列化 Master2SlavePacket
    bool deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < 5) {
            Log.e("Master2SlavePacket data too short");
            return false;
        }
        message_id = data[0];
        destination_id = (static_cast<uint32_t>(data[1]) << 24) |
                         (static_cast<uint32_t>(data[2]) << 16) |
                         (static_cast<uint32_t>(data[3]) << 8) |
                         static_cast<uint32_t>(data[4]);
        payload.assign(data.begin() + 5, data.end());
        return true;
    }
};

struct Slave2MasterPacket {
    uint8_t message_id;              // 消息类型标识
    uint32_t source_id;              // 目标设备 ID
    std::vector<uint8_t> payload;    // 消息的序列化数据

    // 序列化 Master2SlavePacket
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data;
        data.reserve(5 + payload.size());    // 1 + 4 + payload size
        data.push_back(message_id);
        data.push_back(static_cast<uint8_t>(source_id >> 24));
        data.push_back(static_cast<uint8_t>(source_id >> 16));
        data.push_back(static_cast<uint8_t>(source_id >> 8));
        data.push_back(static_cast<uint8_t>(source_id));
        data.insert(data.end(), payload.begin(), payload.end());
        return data;
    }

    // 反序列化 Master2SlavePacket
    bool deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < 5) {
            Log.e("Slave2MasterPacket data too short");
            return false;
        }
        message_id = data[0];
        source_id = (static_cast<uint32_t>(data[1]) << 24) |
                    (static_cast<uint32_t>(data[2]) << 16) |
                    (static_cast<uint32_t>(data[3]) << 8) |
                    static_cast<uint32_t>(data[4]);
        payload.assign(data.begin() + 5, data.end());
        return true;
    }
};

class PacketPacker {
   public:
    // 将消息打包为 Master2SlavePacket
    static Master2SlavePacket masterPack(const Message& msg,
                                         uint32_t destination_id) {
        Master2SlavePacket packet;
        packet.message_id = msg.message_type();    // 获取消息类型
        packet.destination_id = destination_id;    // 设置目标 ID
        msg.serialize(packet.payload);
        return packet;
    }

    static Slave2MasterPacket slavePack(const Message& msg,
                                        uint32_t source_id) {
        Slave2MasterPacket packet;
        packet.message_id = msg.message_type();    // 获取消息类型
        packet.source_id = source_id;              // 设置目标 ID
        msg.serialize(packet.payload);
        return packet;
    }
};

class FramePacker {
   public:
    // 将 Master2SlavePacket 打包为帧
    static std::vector<uint8_t> pack(const Master2SlavePacket& packet,
                                     uint8_t slot = 0, uint8_t fragment_seq = 0,
                                     uint8_t more_fragments = 0) {
        // 序列化 Master2SlavePacket
        std::vector<uint8_t> packet_data = packet.serialize();

        // 构建帧头
        FrameHeader header;
        header.slot = slot;
        header.packet_id = static_cast<uint8_t>(PacketType::Master2Slave);
        header.fragment_sequence = fragment_seq;
        header.more_fragments_flag = more_fragments;
        header.data_length = static_cast<uint16_t>(packet_data.size());

        // 合并帧头和 Master2SlavePacket 数据
        std::vector<uint8_t> frame = header.serialize();
        frame.insert(frame.end(), packet_data.begin(), packet_data.end());
        return frame;
    }

    // 将 Slave2MasterPacket 打包为帧
    static std::vector<uint8_t> pack(const Slave2MasterPacket& packet,
                                     uint8_t slot = 0, uint8_t fragment_seq = 0,
                                     uint8_t more_fragments = 0) {
        // 序列化 Slave2MasterPacket
        std::vector<uint8_t> packet_data = packet.serialize();

        // 构建帧头
        FrameHeader header;
        header.slot = slot;
        header.packet_id = static_cast<uint8_t>(PacketType::Slave2Master);
        header.fragment_sequence = fragment_seq;
        header.more_fragments_flag = more_fragments;
        header.data_length = static_cast<uint16_t>(packet_data.size());

        // 合并帧头和 Master2SlavePacket 数据
        std::vector<uint8_t> frame = header.serialize();
        frame.insert(frame.end(), packet_data.begin(), packet_data.end());
        return frame;
    }
};

namespace Master2Slave {
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

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2SlaveMessageID::SYNC_MSG);
    }
};

class CondCfgMsg : public Message {
   public:
    static uint8_t timeSlot;               // 为从节点分配的时隙
    static uint8_t interval;               // 采集间隔，单位 ms
    static uint16_t totalConductionNum;    // 系统中总导通检测的数量
    static uint16_t startConductionNum;    // 起始导通数量
    static uint16_t conductionNum;         // 导通检测数量

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(timeSlot);
        data.push_back(interval);    // 序列化采集间隔
        data.push_back(static_cast<uint8_t>(totalConductionNum >> 8));
        data.push_back(static_cast<uint8_t>(totalConductionNum));
        data.push_back(static_cast<uint8_t>(startConductionNum >> 8));
        data.push_back(static_cast<uint8_t>(startConductionNum));
        data.push_back(static_cast<uint8_t>(conductionNum >> 8));
        data.push_back(static_cast<uint8_t>(conductionNum));
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 8) {    // 修改为8字节
            Log.e("CondCfgMsg: Invalid CondCfgMsg data size");
            return;
        }
        timeSlot = data[0];
        interval = data[1];    // 反序列化采集间隔
        totalConductionNum = (data[3] << 8) | data[2];
        startConductionNum = (data[5] << 8) | data[4];
        conductionNum = (data[7] << 8) | data[6];
        Log.d(
            "CondCfgMsg: timeSlot = 0x%02X, interval = 0x%02X, "
            "totalConductionNum "
            "= 0x%04X, startConductionNum = 0x%04X, conductionNum = 0x%04X",
            timeSlot, interval, totalConductionNum, startConductionNum,
            conductionNum);
    }
    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2SlaveMessageID::COND_CFG_MSG);
    }
};

class ResCfgMsg : public Message {
   public:
    uint8_t timeSlot;               // 为从节点分配的时隙
    uint8_t interval;               // 采集间隔，单位 ms
    uint16_t totalResistanceNum;    // 系统中总阻值检测的数量
    uint16_t startResistanceNum;    // 起始阻值数量
    uint16_t resistanceNum;         // 阻值检测数量

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(timeSlot);
        data.push_back(interval);    // 序列化采集间隔
        data.push_back(static_cast<uint8_t>(totalResistanceNum >> 8));
        data.push_back(static_cast<uint8_t>(totalResistanceNum));
        data.push_back(static_cast<uint8_t>(startResistanceNum >> 8));
        data.push_back(static_cast<uint8_t>(startResistanceNum));
        data.push_back(static_cast<uint8_t>(resistanceNum >> 8));
        data.push_back(static_cast<uint8_t>(resistanceNum));
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 8) {    // 修改为8字节
            Log.e("ResCfgMsg: Invalid ResCfgMsg data size");
            return;
        }
        timeSlot = data[0];
        interval = data[1];    // 反序列化采集间隔
        totalResistanceNum = (data[3] << 8) | data[2];
        startResistanceNum = (data[5] << 8) | data[4];
        resistanceNum = (data[7] << 8) | data[6];
        Log.d(
            "ResCfgMsg: timeSlot = 0x%02X, interval = 0x%02X, "
            "totalResistanceNum = 0x%04X, "
            "startResistanceNum = 0x%04X, resistanceNum = 0x%04X",
            timeSlot, interval, totalResistanceNum, startResistanceNum,
            resistanceNum);
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2SlaveMessageID::RES_CFG_MSG);
    }
};

class ClipCfgMsg : public Message {
   public:
    uint8_t interval;    // 采集间隔，单位 ms
    uint8_t mode;        // 0：非自锁，1：自锁
    uint16_t clipPin;    // 16 个卡钉激活信息，激活的位置 1，未激活的位置 0

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(interval);    // 序列化采集间隔
        data.push_back(mode);        // 序列化 mode
        data.push_back(static_cast<uint8_t>(clipPin));    // 低字节在前
        data.push_back(static_cast<uint8_t>(clipPin >> 8));    // 高字节在后
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 4) {    // 修改为4字节
            Log.e("ClipCfgMsg: Invalid ClipCfgMsg data size");
            return;
        }
        interval = data[0];                    // 反序列化采集间隔
        mode = data[1];                        // 反序列化 mode
        clipPin = data[2] | (data[3] << 8);    // 低字节在前，高字节在后
        Log.d(
            "ClipCfgMsg: interval = 0x%02X, mode = 0x%02X, clipPin = "
            "0x%04X",
            interval, mode, clipPin);
    }

    void process() override { Log.d("ClipCfgMsg process"); };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2SlaveMessageID::CLIP_CFG_MSG);
    }
};

class ReadCondDataMsg : public Message {
   public:
    uint8_t reserve;    // 保留字段，固定为0

    ReadCondDataMsg() : reserve(0) {}

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(reserve);    // 序列化保留字段
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 1) {
            Log.e("ReadCondDataMsg: Invalid ReadCondDataMsg data size");
            return;
        }
        reserve = data[0];    // 反序列化保留字段
        Log.d("ReadCondDataMsg: reserve = 0x%02X", reserve);
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2SlaveMessageID::READ_COND_DATA_MSG);
    }
};

class ReadResDataMsg : public Message {
   public:
    uint8_t reserve;    // 保留字段，固定为0

    ReadResDataMsg() : reserve(0) {}

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(reserve);    // 序列化保留字段
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 1) {
            Log.e("ReadResDataMsg: Invalid ReadResDataMsg data size");
            return;
        }
        reserve = data[0];    // 反序列化保留字段
        Log.d("ReadResDataMsg: reserve = 0x%02X", reserve);
    }

    void process() override { Log.d("ReadResDataMsg process"); };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2SlaveMessageID::READ_RES_DATA_MSG);
    }
};

class ReadClipDataMsg : public Message {
   public:
    uint8_t reserve;    // 保留字段，固定为0

    ReadClipDataMsg() : reserve(0) {}

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(reserve);    // 序列化保留字段
    }
    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 1) {
            Log.e("ReadClipDataMsg: Invalid ReadClipDataMsg data size");
            return;
        }
        reserve = data[0];    // 反序列化保留字段
        Log.d("ReadClipDataMsg: reserve = 0x%02X", reserve);
    }
    void process() override { "ReadClipDataMsg process"; };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2SlaveMessageID::READ_CLIP_DATA_MSG);
    }
};

class RstMsg : public Message {
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
            Log.e("RstMsg: Invalid RstMsg data size");
        }
        lock = data[0];
        // 新增 clipLed 反序列化
        clipLed = data[1] | (data[2] << 8);    // 低字节在前，高字节在后
        Log.d("RstMsg: lock = 0x%02X, clipLed = 0x%04X", lock, clipLed);
    }

    void process() override { Log.d("RstMsg process"); };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2SlaveMessageID::RST_MSG);
    }
};
}    // namespace Master2Slave

namespace Slave2Master {
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
// 导通数据消息（Slave -> Master）
class CondInfoMsg : public Message {
   public:
    uint8_t status;                 // 新增状态码
    uint8_t timeSlot;               // 为从节点分配的时隙
    uint8_t interval;               // 采集间隔，单位 ms
    uint16_t totalConductionNum;    // 系统中总导通检测的数量
    uint16_t startConductionNum;    // 起始导通数量
    uint16_t conductionNum;         // 导通检测数量

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(status);    // 新增状态码序列化
        data.push_back(timeSlot);
        data.push_back(interval);    // 序列化采集间隔
        data.push_back(static_cast<uint8_t>(totalConductionNum));
        data.push_back(static_cast<uint8_t>(totalConductionNum >> 8));
        data.push_back(static_cast<uint8_t>(startConductionNum));
        data.push_back(static_cast<uint8_t>(startConductionNum >> 8));
        data.push_back(static_cast<uint8_t>(conductionNum));
        data.push_back(static_cast<uint8_t>(conductionNum >> 8));
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 9) {    // 修改为9字节(原8+新增1)
            Log.e("CondInfoMsg: Invalid CondInfoMsg data size");
            return;
        }
        status = data[0];    // 新增状态码反序列化
        timeSlot = data[1];
        interval = data[2];    // 调整字段索引(+1)
        totalConductionNum = (data[4] << 8) | data[3];
        startConductionNum = (data[6] << 8) | data[5];
        conductionNum = (data[8] << 8) | data[7];
        Log.d(
            "CondInfoMsg: status=0x%02X, timeSlot = 0x%02X, interval = 0x%02X, "
            "totalConductionNum = 0x%04X, "
            "startConductionNum = 0x%04X, conductionNum = 0x%04X",
            status, timeSlot, interval, totalConductionNum, startConductionNum,
            conductionNum);
    }
    void process() override { Log.d("CondInfoMsg process"); };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::COND_INFO_MSG);
    }
};

class ResInfoMsg : public Message {
   public:
    uint8_t status;                 // 新增状态码
    uint8_t timeSlot;               // 为从节点分配的时隙
    uint8_t interval;               // 采集间隔，单位 ms
    uint16_t totalResistanceNum;    // 系统中总阻值检测的数量
    uint16_t startResistanceNum;    // 起始阻值数量
    uint16_t resistanceNum;         // 阻值检测数量

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(status);    // 新增状态码序列化
        data.push_back(timeSlot);
        data.push_back(interval);    // 序列化采集间隔
        data.push_back(static_cast<uint8_t>(totalResistanceNum >> 8));
        data.push_back(static_cast<uint8_t>(totalResistanceNum));
        data.push_back(static_cast<uint8_t>(startResistanceNum >> 8));
        data.push_back(static_cast<uint8_t>(startResistanceNum));
        data.push_back(static_cast<uint8_t>(resistanceNum >> 8));
        data.push_back(static_cast<uint8_t>(resistanceNum));
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 9) {    // 修改为9字节(原8+新增1)
            Log.e("ResInfoMsg: Invalid ResInfoMsg data size");
            return;
        }
        status = data[0];    // 新增状态码反序列化
        timeSlot = data[1];
        interval = data[2];    // 调整字段索引(+1)
        totalResistanceNum = (data[4] << 8) | data[3];
        startResistanceNum = (data[6] << 8) | data[5];
        resistanceNum = (data[8] << 8) | data[7];
        Log.d(
            "ResInfoMsg: status=0x%02X, timeSlot = 0x%02X, interval = 0x%02X, "
            "totalResistanceNum = 0x%04X, "
            "startResistanceNum = 0x%04X, resistanceNum = 0x%04X",
            status, timeSlot, interval, totalResistanceNum, startResistanceNum,
            resistanceNum);
    }

    void process() override { Log.d("ResInfoMsg process"); };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::RES_INFO_MSG);
    }
};

class ClipInfoMsg : public Message {
   public:
    uint8_t status;      // 新增状态码
    uint8_t interval;    // 采集间隔，单位 ms
    uint8_t mode;        // 0：非自锁，1：自锁
    uint16_t clipPin;    // 16 个卡钉激活信息，激活的位置 1，未激活的位置 0

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(status);      // 新增状态码序列化
        data.push_back(interval);    // 序列化采集间隔
        data.push_back(mode);        // 序列化 mode
        data.push_back(static_cast<uint8_t>(clipPin));    // 低字节在前
        data.push_back(static_cast<uint8_t>(clipPin >> 8));    // 高字节在后
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 5) {    // 修改为5字节(原4+新增1)
            Log.e("ClipInfoMsg: Invalid ClipInfoMsg data size");
            return;
        }
        status = data[0];                      // 新增状态码反序列化
        interval = data[1];                    // 调整字段索引(+1)
        mode = data[2];                        // 调整字段索引(+1)
        clipPin = data[3] | (data[4] << 8);    // 调整字段索引(+1)
        Log.d(
            "ClipInfoMsg: status=0x%02X, interval = 0x%02X, mode = 0x%02X, "
            "clipPin = 0x%04X",
            status, interval, mode, clipPin);
    }

    void process() override { Log.d("ClipInfoMsg process"); };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::CLIP_INFO_MSG);
    }
};

class CondDataMsg : public Message {
   public:
    DeviceStatus deviceStatus;              // 设备状态
    uint16_t conductionLength;              // 导通数据字段长度
    std::vector<uint8_t> conductionData;    // 导通数据

    void serialize(std::vector<uint8_t>& data) const override {
        // 序列化设备状态
        uint16_t status = *reinterpret_cast<const uint16_t*>(&deviceStatus);
        data.push_back(static_cast<uint8_t>(status));         // 低字节
        data.push_back(static_cast<uint8_t>(status >> 8));    // 高字节

        // 序列化导通数据长度
        data.push_back(static_cast<uint8_t>(conductionLength));    // 低字节
        data.push_back(
            static_cast<uint8_t>(conductionLength >> 8));    // 高字节

        // 序列化导通数据
        data.insert(data.end(), conductionData.begin(), conductionData.end());
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() < 4) {
            Log.e("CondDataMsg: Invalid CondDataMsg data size");
            return;
        }

        // 反序列化设备状态
        uint16_t status = data[0] | (data[1] << 8);
        deviceStatus = *reinterpret_cast<DeviceStatus*>(&status);

        // 反序列化导通数据长度
        conductionLength = data[2] | (data[3] << 8);

        // 反序列化导通数据
        if (data.size() != 4 + conductionLength) {
            Log.e("CondDataMsg: Invalid conduction data size");
            return;
        }
        conductionData.assign(data.begin() + 4, data.end());
    }

    void process() override { Log.d("CondDataMsg process"); };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::COND_DATA_MSG);
    }
};

class ResistanceDataMsg : public Message {
   public:
    DeviceStatus deviceStatus;              // 设备状态
    uint16_t resistanceLength;              // 阻值数据长度
    std::vector<uint8_t> resistanceData;    // 阻值数据

    void serialize(std::vector<uint8_t>& data) const override {
        // 序列化设备状态
        uint16_t status = *reinterpret_cast<const uint16_t*>(&deviceStatus);
        data.push_back(static_cast<uint8_t>(status));         // 低字节
        data.push_back(static_cast<uint8_t>(status >> 8));    // 高字节

        // 序列化阻值数据长度
        data.push_back(static_cast<uint8_t>(resistanceLength));    // 低字节
        data.push_back(
            static_cast<uint8_t>(resistanceLength >> 8));    // 高字节

        // 序列化阻值数据
        data.insert(data.end(), resistanceData.begin(), resistanceData.end());
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() < 4) {
            Log.e("ResistanceDataMsg: Invalid ResistanceDataMsg data size");
            return;
        }

        // 反序列化设备状态
        uint16_t status = data[0] | (data[1] << 8);
        deviceStatus = *reinterpret_cast<DeviceStatus*>(&status);

        // 反序列化阻值数据长度
        resistanceLength = data[2] | (data[3] << 8);

        // 反序列化阻值数据
        if (data.size() != 4 + resistanceLength) {
            Log.e("ResistanceDataMsg: Invalid resistance data size");
            return;
        }
        resistanceData.assign(data.begin() + 4, data.end());
    }

    void process() override { Log.d("ResistanceDataMsg process"); };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::RES_DATA_MSG);
    }
};

class ClipDataMsg : public Message {
   public:
    DeviceStatus deviceStatus;    // 设备状态
    uint16_t clipData;            // 卡钉板数据

    void serialize(std::vector<uint8_t>& data) const override {
        // 序列化设备状态
        uint16_t status = *reinterpret_cast<const uint16_t*>(&deviceStatus);
        data.push_back(static_cast<uint8_t>(status));         // 低字节
        data.push_back(static_cast<uint8_t>(status >> 8));    // 高字节

        // 序列化卡钉板数据
        data.push_back(static_cast<uint8_t>(clipData));         // 低字节
        data.push_back(static_cast<uint8_t>(clipData >> 8));    // 高字节
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 4) {
            Log.e("ClipDataMsg: Invalid ClipDataMsg data size");
            return;
        }

        // 反序列化设备状态
        uint16_t status = data[0] | (data[1] << 8);
        deviceStatus = *reinterpret_cast<DeviceStatus*>(&status);

        // 反序列化卡钉板数据
        clipData = data[2] | (data[3] << 8);
        Log.d("ClipDataMsg: deviceStatus = 0x%04X, clipData = 0x%04X", status,
              clipData);
    }

    void process() override { Log.d("ClipDataMsg process"); };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::CLIP_DATA_MSG);
    }
};

class RstMsg : public Message {
   public:
    uint8_t status;        // 新增状态码
    uint8_t lockStatus;    // 锁状态
    uint16_t clipLed;      // 卡钉灯位初始化信息

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(status);    // 新增状态码序列化
        data.push_back(lockStatus);
        // 序列化卡钉灯位信息
        data.push_back(static_cast<uint8_t>(clipLed));         // 低字节
        data.push_back(static_cast<uint8_t>(clipLed >> 8));    // 高字节
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 4) {    // 修改为4字节(原3+新增1)
            Log.e("RstMsg: Invalid RstMsg data size");
            return;
        }

        status = data[0];                      // 新增状态码反序列化
        lockStatus = data[1];                  // 调整字段索引(+1)
        clipLed = data[2] | (data[3] << 8);    // 调整字段索引(+1)
        Log.d("RstMsg: status=0x%02X, lockStatus = 0x%02X, clipLed = 0x%04X",
              status, lockStatus, clipLed);
    }

    void process() override { Log.d("RstMsg process"); };

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::RST_MSG);
    }
};
}    // namespace Slave2Master

class FrameParser {
   public:
    std::unique_ptr<Message> parse(const std::vector<uint8_t>& raw_data) {
        // 1. 解析帧头
        FrameHeader header;
        Log.d("FrameParser: raw_data size=%d", raw_data.size());
        if (!header.deserialize(raw_data)) {
            Log.e("FrameParser: Frame header parse failed");
            return nullptr;
        }

        Log.d("FrameParser: header parsed, type=0x%02X, len=%d",
              header.packet_id, header.data_length);

        // 数据完整性验证
        if (raw_data.size() != FrameHeader::HEADER_SIZE + header.data_length) {
            Log.e("FrameParser: invalid frame, expected=%d, actual=%d",
                  8 + header.data_length, raw_data.size());
            return nullptr;
        }

        // 2. 提取 Master2SlavePacket 数据
        auto packet_start = raw_data.begin() + FrameHeader::HEADER_SIZE;
        auto packet_end = packet_start + header.data_length;
        std::vector<uint8_t> packet_data(packet_start, packet_end);
        Log.d("FrameParser: payload extracted, len=%d", packet_data.size());

        if (header.packet_id ==
            static_cast<uint8_t>(PacketType::Master2Slave)) {
            // 3. 反序列化 Master2SlavePacket
            Master2SlavePacket packet;
            if (!packet.deserialize(packet_data)) {
                Log.e("Failed to deserialize packet");
                return nullptr;
            }

            const char* msgTypeStr = "Unknown";
            switch (static_cast<Master2SlaveMessageID>(packet.message_id)) {
                case Master2SlaveMessageID::SYNC_MSG:
                    msgTypeStr = "SYNC_MSG";
                    break;
                case Master2SlaveMessageID::COND_CFG_MSG:
                    msgTypeStr = "COND_CFG_MSG";
                    break;
                case Master2SlaveMessageID::RES_CFG_MSG:
                    msgTypeStr = "RES_CFG_MSG";
                    break;
                case Master2SlaveMessageID::CLIP_CFG_MSG:
                    msgTypeStr = "CLIP_CFG_MSG";
                    break;
                case Master2SlaveMessageID::READ_COND_DATA_MSG:
                    msgTypeStr = "READ_COND_DATA_MSG";
                    break;
                case Master2SlaveMessageID::RST_MSG:
                    msgTypeStr = "RST_MSG";
                    break;
                default:
                    break;
            }

            Log.d(
                "FrameParser: packet parsed, type=%s (0x%02X), "
                "destination_id=0x%08X",
                msgTypeStr, packet.message_id, packet.destination_id);

            switch (static_cast<Master2SlaveMessageID>(packet.message_id)) {
                case Master2SlaveMessageID::SYNC_MSG: {
                    Log.d("FrameParser: processing SYNC_MSG message");
                    auto msg = std::make_unique<Master2Slave::SyncMsg>();
                    msg->deserialize(packet.payload);
                    Log.d("FrameParser: SYNC_MSG message deserialized");
                    return msg;
                }
                case Master2SlaveMessageID::COND_CFG_MSG: {
                    Log.d(
                        "FrameParser: processing COND_CFG_MSG "
                        "message");
                    auto msg = std::make_unique<Master2Slave::CondCfgMsg>();
                    msg->deserialize(packet.payload);
                    Log.d(
                        "FrameParser: COND_CFG_MSG message "
                        "deserialized");
                    return msg;
                }
                case Master2SlaveMessageID::RES_CFG_MSG: {
                    Log.d(
                        "FrameParser: processing RES_CFG_MSG "
                        "message");
                    auto msg = std::make_unique<Master2Slave::ResCfgMsg>();
                    msg->deserialize(packet.payload);
                    Log.d(
                        "FrameParser: RES_CFG_MSG message "
                        "deserialized");
                    return msg;
                }
                case Master2SlaveMessageID::CLIP_CFG_MSG: {
                    Log.d(
                        "FrameParser: processing CLIP_CFG_MSG "
                        "message");
                    auto msg = std::make_unique<Master2Slave::ClipCfgMsg>();
                    msg->deserialize(packet.payload);
                    Log.d(
                        "FrameParser: CLIP_CFG_MSG message "
                        "deserialized");
                    return msg;
                }
                case Master2SlaveMessageID::READ_COND_DATA_MSG: {
                    Log.d("FrameParser: processing READ_COND_DATA_MSG message");
                    auto msg =
                        std::make_unique<Master2Slave::ReadCondDataMsg>();
                    msg->deserialize(packet.payload);
                    Log.d(
                        "FrameParser: READ_COND_DATA_MSG message deserialized");
                    return msg;
                }
                case Master2SlaveMessageID::READ_RES_DATA_MSG: {
                    Log.d("FrameParser: processing READ_RES_DATA_MSG message");
                    auto msg = std::make_unique<Master2Slave::ReadResDataMsg>();
                    msg->deserialize(packet.payload);
                    Log.d(
                        "FrameParser: READ_RES_DATA_MSG message deserialized");
                    return msg;
                }
                case Master2SlaveMessageID::READ_CLIP_DATA_MSG: {
                    Log.d("FrameParser: processing READ_CLIP_DATA_MSG message");
                    auto msg =
                        std::make_unique<Master2Slave::ReadClipDataMsg>();
                    msg->deserialize(packet.payload);
                    Log.d(
                        "FrameParser: READ_CLIP_DATA_MSG message deserialized");
                    return msg;
                }
                case Master2SlaveMessageID::RST_MSG: {
                    Log.d("FrameParser: processing RST_MSG message");
                    auto msg = std::make_unique<Master2Slave::RstMsg>();
                    msg->deserialize(packet.payload);
                    Log.d("FrameParser: RST_MSG message deserialized");
                    return msg;
                }
                default:
                    Log.e(
                        "FrameParser: unsupported Master message "
                        "type=0x%02X",
                        static_cast<uint8_t>(packet.message_id));
                    return nullptr;
            }
        } else if (header.packet_id ==
                   static_cast<uint8_t>(PacketType::Slave2Master)) {
            Slave2MasterPacket packet;

            // 3. 反序列化 Slave2MasterPacket
            if (!packet.deserialize(packet_data)) {
                Log.e("Failed to deserialize packet");
                return nullptr;
            }

            const char* msgTypeStr = "Unknown";
            switch (static_cast<Slave2MasterMessageID>(packet.message_id)) {
                case Slave2MasterMessageID::COND_INFO_MSG:
                    msgTypeStr = "COND_INFO_MSG";
                    break;
                case Slave2MasterMessageID::RES_INFO_MSG:
                    msgTypeStr = "RES_INFO_MSG";
                    break;
                case Slave2MasterMessageID::CLIP_INFO_MSG:
                    msgTypeStr = "CLIP_INFO_MSG";
                    break;
                case Slave2MasterMessageID::COND_DATA_MSG:
                    msgTypeStr = "COND_DATA_MSG";
                    break;
                case Slave2MasterMessageID::RES_DATA_MSG:
                    msgTypeStr = "RES_DATA_MSG";
                    break;
                case Slave2MasterMessageID::CLIP_DATA_MSG:
                    msgTypeStr = "CLIP_DATA_MSG";
                    break;
                case Slave2MasterMessageID::RST_MSG:
                    msgTypeStr = "RST_MSG";
                    break;
                default:
                    break;
            }

            Log.d(
                "FrameParser: packet parsed, type=%s (0x%02X), "
                "source_id=0x%08X",
                msgTypeStr, packet.message_id, packet.source_id);

            switch (static_cast<Slave2MasterMessageID>(packet.message_id)) {
                case Slave2MasterMessageID::COND_INFO_MSG: {
                    Log.d("FrameParser: processing COND_INFO_MSG message");
                    auto msg = std::make_unique<Slave2Master::CondInfoMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Slave2MasterMessageID::RES_INFO_MSG: {
                    Log.d("FrameParser: processing RES_INFO_MSG message");
                    auto msg = std::make_unique<Slave2Master::ResInfoMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Slave2MasterMessageID::CLIP_INFO_MSG: {
                    Log.d("FrameParser: processing CLIP_INFO_MSG message");
                    auto msg = std::make_unique<Slave2Master::ClipInfoMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Slave2MasterMessageID::COND_DATA_MSG: {
                    Log.d("FrameParser: processing COND_DATA_MSG message");
                    auto msg = std::make_unique<Slave2Master::CondDataMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Slave2MasterMessageID::RES_DATA_MSG: {
                    Log.d("FrameParser: processing RES_DATA_MSG message");
                    auto msg =
                        std::make_unique<Slave2Master::ResistanceDataMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Slave2MasterMessageID::CLIP_DATA_MSG: {
                    Log.d("FrameParser: processing CLIP_DATA_MSG message");
                    auto msg = std::make_unique<Slave2Master::ClipDataMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Slave2MasterMessageID::RST_MSG: {
                    Log.d("FrameParser: processing RST_MSG message");
                    auto msg = std::make_unique<Slave2Master::RstMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                default:
                    Log.e(
                        "FrameParser: unsupported Slave message "
                        "type=0x%02X",
                        static_cast<uint8_t>(packet.message_id));
                    return nullptr;
            }

        } else {
            Log.e("FrameParser: unsupported Packet type=0x%02X",
                  header.packet_id);
            return nullptr;
        }
    }
};