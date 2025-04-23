/**
 * @file protocol.hpp
 * @author wang.yunlong (wang.yunlong9@byd.com)
 * @brief 通信协议定义文件，包含：
 *        - 消息类型枚举定义
 *        - 数据包结构体(帧头、负载等)
 *        - 消息类及序列化/反序列化方法
 *        - 标准解析流程：
 *          1. 使用FrameParser解析原始数据
 *          2. 获取对应消息对象
 *          3. 调用process()处理消息
 *        - 标准消息构造流程：
 *          1. 构造具体消息对象并设置字段
 *          2. 使用PacketPacker打包为Packet
 *          3. 使用FramePacker打包为完整帧
 * @version 0.1
 * @date 2025-04-11
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef __PROTOCOL_HPP
#define __PROTOCOL_HPP
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "bsp_log.hpp"
#include "bsp_uid.hpp"

enum class PacketType : uint8_t {
    Master2Slave = 0x00,      // 对应协议中Master2Slave
    Slave2Master = 0x01,      // 对应协议中Slave2Master
    Backend2Master = 0x02,    // 对应协议中Backend2Master
    Master2Backend = 0x03,    // 对应协议中Master2Backend
    Slave2Backend = 0x04      // 对应协议中Slave2Backend
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
    COND_CFG_MSG = 0x10,    // 导通信息
    RES_CFG_MSG = 0x11,     // 阻值信息
    CLIP_CFG_MSG = 0x12,    // 卡钉信息
    RST_MSG = 0x30,
};

enum class Backend2MasterMessageID : uint8_t {
    SLAVE_CFG_MSG = 0x00,
    MODE_CFG_MSG = 0x01,
    RST_MSG = 0x02,
    CTRL_MSG = 0x03
};

enum class Master2BackendMessageID : uint8_t {
    SLAVE_CFG_MSG = 0x00,
    MODE_CFG_MSG = 0x01,
    RST_MSG = 0x02,
    CTRL_MSG = 0x03,
    CONDUCTION_DATA_MSG = 0x10,    // 导通数据
    RESISTANCE_DATA_MSG = 0x11,    // 阻值数据
    CLIPPING_DATA_MSG = 0x12       // 卡钉数据
};

enum class Slave2BackendMessageID : uint8_t {
    COND_DATA_MSG = 0x00,
    RES_DATA_MSG = 0x01,
    CLIP_DATA_MSG = 0x02,
};

namespace ProtocolUtils {

inline void serializeUint32(std::vector<uint8_t>& data, uint32_t value) {
    data.push_back(static_cast<uint8_t>(value));
    data.push_back(static_cast<uint8_t>(value >> 8));
    data.push_back(static_cast<uint8_t>(value >> 16));
    data.push_back(static_cast<uint8_t>(value >> 24));
}

inline uint32_t deserializeUint32(const std::vector<uint8_t>& data,
                                  size_t offset = 0) {
    return static_cast<uint32_t>(data[offset]) |
           (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) |
           (static_cast<uint32_t>(data[offset + 3]) << 24);
}
}    // namespace ProtocolUtils

class FrameBase {
   public:
    virtual ~FrameBase() = default;
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual void deserialize(const std::vector<uint8_t>& data) = 0;
    virtual bool validate() const { return true; }    // 可扩展校验逻辑
};

// 通用帧头定义
struct FrameHeader {
    static constexpr uint8_t FRAME_DELIMITER[2] = {0xAB, 0xCD};    // 帧分隔符
    static constexpr size_t HEADER_SIZE = 7;    // 2+1+1+1+2=7字节

    uint8_t packet_id;              // 数据包类型
    uint8_t fragment_sequence;      // 帧分片序号
    uint8_t more_fragments_flag;    // 更多分片标志(0:无,1:有)
    uint16_t data_length;           // 数据负载长度

    // 序列化为字节流
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data;
        data.reserve(HEADER_SIZE);
        data.push_back(FRAME_DELIMITER[0]);
        data.push_back(FRAME_DELIMITER[1]);
        data.push_back(packet_id);
        data.push_back(fragment_sequence);
        data.push_back(more_fragments_flag);
        data.push_back(static_cast<uint8_t>(data_length & 0xFF));    // 低位在前
        data.push_back(static_cast<uint8_t>(data_length >> 8));    // 高位在后
        return data;
    }

    bool deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < HEADER_SIZE) {
            Log.e("Invalid frame header data size");
            return false;
        }

        // 验证帧分隔符
        if (data[0] != FRAME_DELIMITER[0] || data[1] != FRAME_DELIMITER[1]) {
            Log.e("Invalid frame delimiter");
            return false;
        }

        packet_id = data[2];
        fragment_sequence = data[3];
        more_fragments_flag = data[4];
        data_length = data[5] | (data[6] << 8);    // 小端模式
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
        ProtocolUtils::serializeUint32(data, destination_id);
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
        destination_id = ProtocolUtils::deserializeUint32(data, 1);
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
        ProtocolUtils::serializeUint32(data, source_id);
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
        source_id = ProtocolUtils::deserializeUint32(data, 1);
        payload.assign(data.begin() + 5, data.end());
        return true;
    }
};

struct Backend2MasterPacket {
    uint8_t message_id;              // 消息类型标识
    std::vector<uint8_t> payload;    // 消息的序列化数据

    // 序列化 Backend2MasterPacket
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data;
        data.reserve(1 + payload.size());    // 1 + payload size
        data.push_back(message_id);
        data.insert(data.end(), payload.begin(), payload.end());
        return data;
    }

    // 反序列化 Backend2MasterPacket
    bool deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < 1) {
            Log.e("Backend2MasterPacket data too short");
            return false;
        }
        message_id = data[0];
        payload.assign(data.begin() + 1, data.end());
        return true;
    }
};

struct Master2BackendPacket {
    uint8_t message_id;              // 消息类型标识
    std::vector<uint8_t> payload;    // 消息的序列化数据

    // 序列化 Master2BackendPacket
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data;
        data.reserve(1 + payload.size());    // 1 + payload size
        // 序列化消息ID
        data.push_back(message_id);
        // 序列化payload
        data.insert(data.end(), payload.begin(), payload.end());
        return data;
    }

    // 反序列化 Master2BackendPacket
    bool deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < 1) {
            Log.e("Master2BackendPacket data too short");
            return false;
        }
        // 反序列化消息ID
        message_id = data[0];
        // 反序列化payload
        payload.assign(data.begin() + 1, data.end());
        return true;
    }
};
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
struct Slave2BackendPacket {
    uint8_t message_id;              // 消息类型标识
    uint32_t slave_id;               // 新增: 本机ID (4字节)
    DeviceStatus device_status;      // 设备状态(2字节)
    std::vector<uint8_t> payload;    // 消息的序列化数据

    // 序列化 Slave2BackendPacket
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data;
        data.reserve(7 + payload.size());    // 1 + 4 + 2 + payload size

        // 序列化消息ID
        data.push_back(message_id);

        // 序列化本机ID (4字节)
        ProtocolUtils::serializeUint32(data, slave_id);

        // 序列化设备状态
        uint16_t status = *reinterpret_cast<const uint16_t*>(&device_status);
        data.push_back(static_cast<uint8_t>(status));         // 低字节
        data.push_back(static_cast<uint8_t>(status >> 8));    // 高字节

        // 序列化payload
        data.insert(data.end(), payload.begin(), payload.end());
        return data;
    }

    // 反序列化 Slave2BackendPacket
    bool deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < 7) {
            Log.e("Slave2BackendPacket data too short");
            return false;
        }

        // 反序列化消息ID
        message_id = data[0];

        // 反序列化本机ID (4字节)
        slave_id = ProtocolUtils::deserializeUint32(data, 1);

        // 反序列化设备状态
        uint16_t status = data[5] | (data[6] << 8);
        device_status = *reinterpret_cast<DeviceStatus*>(&status);

        // 反序列化payload
        payload.assign(data.begin() + 7, data.end());
        return true;
    }
};

class PacketPacker {
   public:
    // 将消息打包为 Master2SlavePacket
    static Master2SlavePacket master2SlavePack(const Message& msg,
                                               uint32_t destination_id) {
        Master2SlavePacket packet;
        packet.message_id = msg.message_type();    // 获取消息类型
        packet.destination_id = destination_id;    // 设置目标 ID
        msg.serialize(packet.payload);
        return packet;
    }

    static Slave2MasterPacket slave2MasterPack(const Message& msg,
                                               uint32_t source_id) {
        Slave2MasterPacket packet;
        packet.message_id = msg.message_type();    // 获取消息类型
        packet.source_id = source_id;              // 设置目标 ID
        msg.serialize(packet.payload);
        return packet;
    }

    static Backend2MasterPacket backend2MasterPack(const Message& msg) {
        Backend2MasterPacket packet;
        packet.message_id = msg.message_type();    // 获取消息类型
        msg.serialize(packet.payload);
        return packet;
    }

    static Master2BackendPacket master2BackendPack(const Message& msg) {
        Master2BackendPacket packet;
        packet.message_id = msg.message_type();    // 获取消息类型
        msg.serialize(packet.payload);
        return packet;
    }

    static Slave2BackendPacket slave2BackendPack(const Message& msg,
                                                 uint32_t slave_id) {
        Slave2BackendPacket packet;
        packet.message_id = msg.message_type();    // 获取消息类型
        packet.slave_id = slave_id;                // 设置本机ID
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
        header.packet_id = static_cast<uint8_t>(PacketType::Slave2Master);
        header.fragment_sequence = fragment_seq;
        header.more_fragments_flag = more_fragments;
        header.data_length = static_cast<uint16_t>(packet_data.size());

        // 合并帧头和 Master2SlavePacket 数据
        std::vector<uint8_t> frame = header.serialize();
        frame.insert(frame.end(), packet_data.begin(), packet_data.end());
        return frame;
    }

    static std::vector<uint8_t> pack(const Backend2MasterPacket& packet,
                                     uint8_t slot = 0, uint8_t fragment_seq = 0,
                                     uint8_t more_fragments = 0) {
        std::vector<uint8_t> packet_data = packet.serialize();

        // 构建帧头
        FrameHeader header;
        header.packet_id = static_cast<uint8_t>(PacketType::Backend2Master);
        header.fragment_sequence = fragment_seq;
        header.more_fragments_flag = more_fragments;
        header.data_length = static_cast<uint16_t>(packet_data.size());

        // 合并帧头和 Master2SlavePacket 数据
        std::vector<uint8_t> frame = header.serialize();
        frame.insert(frame.end(), packet_data.begin(), packet_data.end());
        return frame;
    }

    static std::vector<uint8_t> pack(const Master2BackendPacket& packet,
                                     uint8_t slot = 0, uint8_t fragment_seq = 0,
                                     uint8_t more_fragments = 0) {
        std::vector<uint8_t> packet_data = packet.serialize();

        // 构建帧头
        FrameHeader header;
        header.packet_id = static_cast<uint8_t>(PacketType::Master2Backend);
        header.fragment_sequence = fragment_seq;
        header.more_fragments_flag = more_fragments;
        header.data_length = static_cast<uint16_t>(packet_data.size());

        // 合并帧头和 Master2SlavePacket 数据
        std::vector<uint8_t> frame = header.serialize();
        frame.insert(frame.end(), packet_data.begin(), packet_data.end());
        return frame;
    }

    static std::vector<uint8_t> pack(const Slave2BackendPacket& packet,
                                     uint8_t slot = 0, uint8_t fragment_seq = 0,
                                     uint8_t more_fragments = 0) {
        std::vector<uint8_t> packet_data = packet.serialize();

        // 构建帧头
        FrameHeader header;
        header.packet_id = static_cast<uint8_t>(PacketType::Slave2Backend);
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
    static uint8_t mode;
    static uint32_t timestamp;
    explicit SyncMsg(uint8_t m = 0, uint32_t ts = 0) {
        mode = m;
        timestamp = ts;
    }

    void serialize(std::vector<uint8_t>& data) const override {
        data.clear();    // 清空传入的vector
        data.push_back(mode);
        ProtocolUtils::serializeUint32(data, timestamp);
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 5) {
            Log.e("SyncMsg: Invalid SyncMsg data size");
        }
        mode = data[0];
        timestamp = ProtocolUtils::deserializeUint32(data, 1);
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
    static uint8_t timeSlot;               // 为从节点分配的时隙
    static uint8_t interval;               // 采集间隔，单位 ms
    static uint16_t totalResistanceNum;    // 系统中总阻值检测的数量
    static uint16_t startResistanceNum;    // 起始阻值数量
    static uint16_t resistanceNum;         // 阻值检测数量

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
    static uint8_t interval;    // 采集间隔，单位 ms
    static uint8_t mode;        // 0：非自锁，1：自锁
    static uint16_t
        clipPin;    // 16 个卡钉激活信息，激活的位置 1，未激活的位置 0

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

    void process() override;

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

    void process() override;

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
    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2SlaveMessageID::READ_CLIP_DATA_MSG);
    }
};

class RstMsg : public Message {
   public:
    static uint8_t lock;
    static uint16_t clipLed;    // 新增卡钉灯位初始化信息

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

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2SlaveMessageID::RST_MSG);
    }
};
}    // namespace Master2Slave

namespace Slave2Master {
// 导通数据消息（Slave -> Master）
class CondCfgMsg : public Message {
   public:
    static uint8_t status;                 // 新增状态码
    static uint8_t timeSlot;               // 为从节点分配的时隙
    static uint8_t interval;               // 采集间隔，单位 ms
    static uint16_t totalConductionNum;    // 系统中总导通检测的数量
    static uint16_t startConductionNum;    // 起始导通数量
    static uint16_t conductionNum;         // 导通检测数量

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
            Log.e("CondCfgMsg: Invalid CondCfgMsg data size");
            return;
        }
        status = data[0];    // 新增状态码反序列化
        timeSlot = data[1];
        interval = data[2];    // 调整字段索引(+1)
        totalConductionNum = (data[4] << 8) | data[3];
        startConductionNum = (data[6] << 8) | data[5];
        conductionNum = (data[8] << 8) | data[7];
        Log.d(
            "CondCfgMsg: status=0x%02X, timeSlot = 0x%02X, interval = 0x%02X, "
            "totalConductionNum = 0x%04X, "
            "startConductionNum = 0x%04X, conductionNum = 0x%04X",
            status, timeSlot, interval, totalConductionNum, startConductionNum,
            conductionNum);
    }
    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::COND_CFG_MSG);
    }
};

class ResCfgMsg : public Message {
   public:
    static uint8_t status;                 // 新增状态码
    static uint8_t timeSlot;               // 为从节点分配的时隙
    static uint8_t interval;               // 采集间隔，单位 ms
    static uint16_t totalResistanceNum;    // 系统中总阻值检测的数量
    static uint16_t startResistanceNum;    // 起始阻值数量
    static uint16_t resistanceNum;         // 阻值检测数量

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
            Log.e("ResCfgMsg: Invalid ResCfgMsg data size");
            return;
        }
        status = data[0];    // 新增状态码反序列化
        timeSlot = data[1];
        interval = data[2];    // 调整字段索引(+1)
        totalResistanceNum = (data[4] << 8) | data[3];
        startResistanceNum = (data[6] << 8) | data[5];
        resistanceNum = (data[8] << 8) | data[7];
        Log.d(
            "ResCfgMsg: status=0x%02X, timeSlot = 0x%02X, interval = 0x%02X, "
            "totalResistanceNum = 0x%04X, "
            "startResistanceNum = 0x%04X, resistanceNum = 0x%04X",
            status, timeSlot, interval, totalResistanceNum, startResistanceNum,
            resistanceNum);
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::RES_CFG_MSG);
    }
};

class ClipCfgMsg : public Message {
   public:
    static uint8_t status;      // 新增状态码
    static uint8_t interval;    // 采集间隔，单位 ms
    static uint8_t mode;        // 0：非自锁，1：自锁
    static uint16_t
        clipPin;    // 16 个卡钉激活信息，激活的位置 1，未激活的位置 0

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(status);      // 新增状态码序列化
        data.push_back(interval);    // 序列化采集间隔
        data.push_back(mode);        // 序列化 mode
        data.push_back(static_cast<uint8_t>(clipPin));    // 低字节在前
        data.push_back(static_cast<uint8_t>(clipPin >> 8));    // 高字节在后
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 5) {    // 修改为5字节(原4+新增1)
            Log.e("ClipCfgMsg: Invalid ClipCfgMsg data size");
            return;
        }
        status = data[0];                      // 新增状态码反序列化
        interval = data[1];                    // 调整字段索引(+1)
        mode = data[2];                        // 调整字段索引(+1)
        clipPin = data[3] | (data[4] << 8);    // 调整字段索引(+1)
        Log.d(
            "ClipCfgMsg: status=0x%02X, interval = 0x%02X, mode = 0x%02X, "
            "clipPin = 0x%04X",
            status, interval, mode, clipPin);
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::CLIP_CFG_MSG);
    }
};
class RstMsg : public Message {
   public:
    static uint8_t status;        // 新增状态码
    static uint8_t lockStatus;    // 锁状态
    static uint16_t clipLed;      // 卡钉灯位初始化信息

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

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2MasterMessageID::RST_MSG);
    }
};
}    // namespace Slave2Master

namespace Backend2Master {
class SlaveCfgMsg : public Message {
   public:
    struct SlaveConfig {
        uint32_t id;              // 从机ID (4字节)
        uint8_t conductionNum;    // 导通检测数量
        uint8_t resistanceNum;    // 阻值检测数量
        uint8_t clipMode;         // 卡钉检测模式
        uint16_t clipStatus;      // 卡钉初始化状态
    };

    static uint8_t slaveNum;                   // 从机数量
    static std::vector<SlaveConfig> slaves;    // 从机配置列表

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(slaveNum);    // 序列化从机数量
        // 序列化每个从机配置
        for (const auto& slave : slaves) {
            // 序列化从机ID (4字节)
            ProtocolUtils::serializeUint32(data, slave.id);
            // 序列化其他配置
            data.push_back(slave.conductionNum);
            data.push_back(slave.resistanceNum);
            data.push_back(slave.clipMode);
            // 序列化卡钉状态 (2字节)
            data.push_back(static_cast<uint8_t>(slave.clipStatus));
            data.push_back(static_cast<uint8_t>(slave.clipStatus >> 8));
        }
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() < 1 || (data.size() - 1) % 9 != 0) {    // 每个从机9字节
            Log.e("SlaveCfgMsg: Invalid data size");
            return;
        }

        slaveNum = data[0];
        slaves.clear();

        // 反序列化每个从机配置
        size_t offset = 1;
        for (uint8_t i = 0; i < slaveNum; i++) {
            if (offset + 8 >= data.size()) break;    // 确保有足够数据

            SlaveConfig slave;
            // 反序列化从机ID (4字节)
            slave.id = ProtocolUtils::deserializeUint32(data, offset);
            offset += 4;

            // 反序列化其他配置
            slave.conductionNum = data[offset++];
            slave.resistanceNum = data[offset++];
            slave.clipMode = data[offset++];

            // 反序列化卡钉状态 (2字节)
            slave.clipStatus = data[offset] | (data[offset + 1] << 8);
            offset += 2;

            slaves.push_back(slave);
        }

        // 日志输出
        Log.d("SlaveCfgMsg: slaveNum = %d", slaveNum);
        for (size_t i = 0; i < slaves.size(); i++) {
            const auto& s = slaves[i];
            Log.d("Slave %d: id=0x%08X, cond=%d, res=%d, mode=%d, clip=0x%04X",
                  i, s.id, s.conductionNum, s.resistanceNum, s.clipMode,
                  s.clipStatus);
        }
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(
            Backend2MasterMessageID::SLAVE_CFG_MSG);    // 需要替换为实际消息ID
    }
};

class ModeCfgMsg : public Message {
   public:
    static uint8_t mode;    // 模式配置

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(mode);    // 序列化模式
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 1) {
            Log.e("ModeCfgMsg: Invalid data size");
            return;
        }
        mode = data[0];
        Log.d("ModeCfgMsg: mode = 0x%02X", mode);
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Backend2MasterMessageID::MODE_CFG_MSG);
    }
};

class RstMsg : public Message {
   public:
    struct SlaveResetConfig {
        uint32_t id;            // 从机ID (4字节)
        uint8_t lock;           // 锁状态控制
        uint16_t clipStatus;    // 卡钉复位状态
    };

    static uint8_t slaveNum;                        // 从机数量
    static std::vector<SlaveResetConfig> slaves;    // 从机复位配置列表

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(slaveNum);    // 序列化从机数量
        // 序列化每个从机配置
        for (const auto& slave : slaves) {
            // 序列化从机ID (4字节)
            ProtocolUtils::serializeUint32(data, slave.id);
            // 序列化锁状态
            data.push_back(slave.lock);
            // 序列化卡钉状态 (2字节)
            data.push_back(static_cast<uint8_t>(slave.clipStatus));
            data.push_back(static_cast<uint8_t>(slave.clipStatus >> 8));
        }
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() < 1 || (data.size() - 1) % 7 != 0) {    // 每个从机7字节
            Log.e("RstMsg: Invalid data size");
            return;
        }

        slaveNum = data[0];
        slaves.clear();

        // 反序列化每个从机配置
        size_t offset = 1;
        for (uint8_t i = 0; i < slaveNum; i++) {
            if (offset + 6 >= data.size()) break;    // 确保有足够数据

            SlaveResetConfig slave;
            // 反序列化从机ID (4字节)
            slave.id = ProtocolUtils::deserializeUint32(data, offset);
            offset += 4;

            // 反序列化锁状态
            slave.lock = data[offset++];

            // 反序列化卡钉状态 (2字节)
            slave.clipStatus = data[offset] | (data[offset + 1] << 8);
            offset += 2;

            slaves.push_back(slave);
        }

        // 日志输出
        Log.d("RstMsg: slaveNum = %d", slaveNum);
        for (size_t i = 0; i < slaves.size(); i++) {
            const auto& s = slaves[i];
            Log.d("Slave %d: id=0x%08X, lock=%d, clipStatus=0x%04X", i, s.id,
                  s.lock, s.clipStatus);
        }
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Backend2MasterMessageID::RST_MSG);
    }
};

class CtrlMsg : public Message {
   public:
    static uint8_t runningStatus;    // 运行状态控制

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(runningStatus);    // 序列化运行状态
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 1) {
            Log.e("CtrlMsg: Invalid data size");
            return;
        }
        runningStatus = data[0];
        Log.d("CtrlMsg: runningStatus = 0x%02X", runningStatus);
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Backend2MasterMessageID::CTRL_MSG);
    }
};

}    // namespace Backend2Master

namespace Master2Backend {
class SlaveCfgMsg : public Message {
   public:
    struct SlaveConfig {
        uint32_t id;              // 从机ID (4字节)
        uint8_t conductionNum;    // 导通检测数量
        uint8_t resistanceNum;    // 阻值检测数量
        uint8_t clipMode;         // 卡钉检测模式
        uint16_t clipStatus;      // 卡钉初始化状态
    };

    static uint8_t status;                     // 响应状态
    static uint8_t slaveNum;                   // 从机数量
    static std::vector<SlaveConfig> slaves;    // 从机配置列表

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(status);      // 序列化响应状态
        data.push_back(slaveNum);    // 序列化从机数量
        // 序列化每个从机配置
        for (const auto& slave : slaves) {
            // 序列化从机ID (4字节)
            ProtocolUtils::serializeUint32(data, slave.id);
            // 序列化其他配置
            data.push_back(slave.conductionNum);
            data.push_back(slave.resistanceNum);
            data.push_back(slave.clipMode);
            // 序列化卡钉状态 (2字节)
            data.push_back(static_cast<uint8_t>(slave.clipStatus));
            data.push_back(static_cast<uint8_t>(slave.clipStatus >> 8));
        }
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() < 2 ||
            (data.size() - 2) % 9 != 0) {    // 2字节头部 + 每个从机9字节
            Log.e("SlaveCfgMsg: Invalid data size");
            return;
        }

        status = data[0];
        slaveNum = data[1];
        slaves.clear();

        // 反序列化每个从机配置
        size_t offset = 2;
        for (uint8_t i = 0; i < slaveNum; i++) {
            if (offset + 8 >= data.size()) break;

            SlaveConfig slave;
            // 反序列化从机ID (4字节)
            slave.id = ProtocolUtils::deserializeUint32(data, offset);
            offset += 4;

            // 反序列化其他配置
            slave.conductionNum = data[offset++];
            slave.resistanceNum = data[offset++];
            slave.clipMode = data[offset++];

            // 反序列化卡钉状态 (2字节)
            slave.clipStatus = data[offset] | (data[offset + 1] << 8);
            offset += 2;

            slaves.push_back(slave);
        }

        // 日志输出
        Log.d("SlaveCfgMsg: status=0x%02X, slaveNum=%d", status, slaveNum);
        for (size_t i = 0; i < slaves.size(); i++) {
            const auto& s = slaves[i];
            Log.d("Slave %d: id=0x%08X, cond=%d, res=%d, mode=%d, clip=0x%04X",
                  i, s.id, s.conductionNum, s.resistanceNum, s.clipMode,
                  s.clipStatus);
        }
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2BackendMessageID::SLAVE_CFG_MSG);
    }
};

class ModeCfgMsg : public Message {
   public:
    static uint8_t status;    // 响应状态
    static uint8_t mode;      // 模式配置

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(status);    // 序列化响应状态
        data.push_back(mode);      // 序列化模式
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 2) {
            Log.e("ModeCfgMsg: Invalid data size");
            return;
        }
        status = data[0];    // 反序列化响应状态
        mode = data[1];      // 反序列化模式
        Log.d("ModeCfgMsg: status=0x%02X, mode=0x%02X", status, mode);
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2BackendMessageID::MODE_CFG_MSG);
    }
};

class RstMsg : public Message {
   public:
    struct SlaveResetConfig {
        uint32_t id;            // 从机ID (4字节)
        uint16_t clipStatus;    // 卡钉复位状态
        uint8_t lock;           // 锁状态控制
    };

    static uint8_t status;                          // 响应状态
    static uint8_t slaveNum;                        // 从机数量
    static std::vector<SlaveResetConfig> slaves;    // 从机复位配置列表

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(status);      // 序列化响应状态
        data.push_back(slaveNum);    // 序列化从机数量
        for (const auto& slave : slaves) {
            // 序列化从机ID (4字节)
            ProtocolUtils::serializeUint32(data, slave.id);
            // 序列化锁状态
            data.push_back(slave.lock);
            // 序列化卡钉状态 (2字节)
            data.push_back(static_cast<uint8_t>(slave.clipStatus));
            data.push_back(static_cast<uint8_t>(slave.clipStatus >> 8));
        }
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() < 2 ||
            (data.size() - 2) % 7 != 0) {    // 2字节头部 + 每个从机7字节
            Log.e("RstMsg: Invalid data size");
            return;
        }

        status = data[0];
        slaveNum = data[1];
        slaves.clear();

        // 反序列化每个从机配置
        size_t offset = 2;
        for (uint8_t i = 0; i < slaveNum; i++) {
            if (offset + 6 >= data.size()) break;

            SlaveResetConfig slave;
            // 反序列化从机ID (4字节)
            slave.id = ProtocolUtils::deserializeUint32(data, offset);
            offset += 4;

            // 反序列化锁状态
            slave.lock = data[offset++];

            // 反序列化卡钉状态 (2字节)
            slave.clipStatus = data[offset] | (data[offset + 1] << 8);
            offset += 2;

            slaves.push_back(slave);
        }

        // 日志输出
        Log.d("RstMsg: status=0x%02X, slaveNum=%d", status, slaveNum);
        for (size_t i = 0; i < slaves.size(); i++) {
            const auto& s = slaves[i];
            Log.d("Slave %d: id=0x%08X, clipStatus=0x%04X, lock=%d", i, s.id,
                  s.clipStatus, s.lock);
        }
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2BackendMessageID::RST_MSG);
    }
};

class CtrlMsg : public Message {
   public:
    static uint8_t status;           // 响应状态
    static uint8_t runningStatus;    // 运行状态控制

    void serialize(std::vector<uint8_t>& data) const override {
        data.push_back(status);           // 序列化响应状态
        data.push_back(runningStatus);    // 序列化运行状态
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 2) {
            Log.e("CtrlMsg: Invalid data size");
            return;
        }
        status = data[0];           // 反序列化响应状态
        runningStatus = data[1];    // 反序列化运行状态
        Log.d("CtrlMsg: status=0x%02X, runningStatus=0x%02X", status,
              runningStatus);
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Master2BackendMessageID::CTRL_MSG);
    }
};
}    // namespace Master2Backend

namespace Slave2Backend {

class CondDataMsg : public Message {
   public:
    static uint16_t conductionLength;              // 导通数据字段长度
    static std::vector<uint8_t> conductionData;    // 导通数据

    void serialize(std::vector<uint8_t>& data) const override {
        // 序列化导通数据长度
        data.push_back(static_cast<uint8_t>(conductionLength));    // 低字节
        data.push_back(
            static_cast<uint8_t>(conductionLength >> 8));    // 高字节

        // 序列化导通数据
        data.insert(data.end(), conductionData.begin(), conductionData.end());
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() < 2) {
            Log.e("CondDataMsg: Invalid data size");
            return;
        }

        // 反序列化导通数据长度
        conductionLength = data[0] | (data[1] << 8);

        // 反序列化导通数据
        if (data.size() != 2 + conductionLength) {
            Log.e("CondDataMsg: Invalid conduction data size");
            return;
        }
        conductionData.assign(data.begin() + 2, data.end());

        Log.d("CondDataMsg: length=%d, dataSize=%d", conductionLength,
              conductionData.size());
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2BackendMessageID::COND_DATA_MSG);
    }
};

class ResDataMsg : public Message {
   public:
    static uint16_t resistanceLength;              // 阻值数据长度
    static std::vector<uint8_t> resistanceData;    // 阻值数据

    void serialize(std::vector<uint8_t>& data) const override {
        // 序列化阻值数据长度
        data.push_back(static_cast<uint8_t>(resistanceLength));    // 低字节
        data.push_back(
            static_cast<uint8_t>(resistanceLength >> 8));    // 高字节

        // 序列化阻值数据
        data.insert(data.end(), resistanceData.begin(), resistanceData.end());
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() < 2) {
            Log.e("ResDataMsg: Invalid data size");
            return;
        }

        // 反序列化阻值数据长度
        resistanceLength = data[0] | (data[1] << 8);

        // 反序列化阻值数据
        if (data.size() != 2 + resistanceLength) {
            Log.e("ResDataMsg: Invalid resistance data size");
            return;
        }
        resistanceData.assign(data.begin() + 2, data.end());

        Log.d("ResDataMsg: length=%d, dataSize=%d", resistanceLength,
              resistanceData.size());
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2BackendMessageID::RES_DATA_MSG);
    }
};

class ClipDataMsg : public Message {
   public:
    static uint16_t clipData;    // 卡钉板数据

    void serialize(std::vector<uint8_t>& data) const override {
        // 序列化卡钉板数据
        data.push_back(static_cast<uint8_t>(clipData));         // 低字节
        data.push_back(static_cast<uint8_t>(clipData >> 8));    // 高字节
    }

    void deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() != 2) {
            Log.e("ClipDataMsg: Invalid data size");
            return;
        }

        // 反序列化卡钉板数据
        clipData = data[0] | (data[1] << 8);

        Log.d("ClipDataMsg: clipData=0x%04X", clipData);
    }

    void process() override;

    uint8_t message_type() const override {
        return static_cast<uint8_t>(Slave2BackendMessageID::CLIP_DATA_MSG);
    }
};

}    // namespace Slave2Backend

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

        Log.d("FrameParser: header parsed, packet type=0x%02X, len=%d",
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

            // Compare packet.destination_id with UIDReader::get()
            if (packet.destination_id != UIDReader::get()) {
                Log.e("FrameParser: id compare fail");
                return nullptr;
            } else {
                Log.d("FrameParser: id compare success");
            }

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
                case Slave2MasterMessageID::COND_CFG_MSG:
                    msgTypeStr = "COND_CFG_MSG";
                    break;
                case Slave2MasterMessageID::RES_CFG_MSG:
                    msgTypeStr = "RES_CFG_MSG";
                    break;
                case Slave2MasterMessageID::CLIP_CFG_MSG:
                    msgTypeStr = "CLIP_CFG_MSG";
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
                case Slave2MasterMessageID::COND_CFG_MSG: {
                    Log.d("FrameParser: processing COND_CFG_MSG message");
                    auto msg = std::make_unique<Slave2Master::CondCfgMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Slave2MasterMessageID::RES_CFG_MSG: {
                    Log.d("FrameParser: processing RES_CFG_MSG message");
                    auto msg = std::make_unique<Slave2Master::ResCfgMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Slave2MasterMessageID::CLIP_CFG_MSG: {
                    Log.d("FrameParser: processing CLIP_CFG_MSG message");
                    auto msg = std::make_unique<Slave2Master::ClipCfgMsg>();
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

        } else if (header.packet_id ==
                   static_cast<uint8_t>(PacketType::Backend2Master)) {
            Backend2MasterPacket packet;
            // 3. 反序列化 Slave2MasterPacket
            if (!packet.deserialize(packet_data)) {
                Log.e("Failed to deserialize packet");
                return nullptr;
            }

            const char* msgTypeStr = "Unknown";
            switch (static_cast<Backend2MasterMessageID>(packet.message_id)) {
                case Backend2MasterMessageID::SLAVE_CFG_MSG:
                    msgTypeStr = "SLAVE_CFG_MSG";
                    break;
                case Backend2MasterMessageID::MODE_CFG_MSG:
                    msgTypeStr = "MODE_CFG_MSG";
                    break;
                case Backend2MasterMessageID::RST_MSG:
                    msgTypeStr = "RST_MSG";
                    break;
                case Backend2MasterMessageID::CTRL_MSG:
                    msgTypeStr = "CTRL_MSG";
                    break;
                default:
                    break;
            }

            Log.d("FrameParser: packet parsed, msg type=%s (0x%02X) ",
                  msgTypeStr, packet.message_id);

            switch (static_cast<Backend2MasterMessageID>(packet.message_id)) {
                case Backend2MasterMessageID::SLAVE_CFG_MSG: {
                    Log.d("FrameParser: processing SLAVE_CFG_MSG message");
                    auto msg = std::make_unique<Backend2Master::SlaveCfgMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Backend2MasterMessageID::MODE_CFG_MSG: {
                    Log.d("FrameParser: processing MODE_CFG_MSG message");
                    auto msg = std::make_unique<Backend2Master::ModeCfgMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Backend2MasterMessageID::RST_MSG: {
                    Log.d("FrameParser: processing RST_MSG message");
                    auto msg = std::make_unique<Backend2Master::RstMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Backend2MasterMessageID::CTRL_MSG: {
                    Log.d("FrameParser: processing CTRL_MSG message");
                    auto msg = std::make_unique<Backend2Master::CtrlMsg>();
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
        } else if (header.packet_id ==
                   static_cast<uint8_t>(PacketType::Master2Backend)) {
            Master2BackendPacket packet;
            if (!packet.deserialize(packet_data)) {
                Log.e("Failed to deserialize packet");
                return nullptr;
            }

            const char* msgTypeStr = "Unknown";
            switch (static_cast<Master2BackendMessageID>(packet.message_id)) {
                case Master2BackendMessageID::SLAVE_CFG_MSG:
                    msgTypeStr = "SLAVE_CFG_MSG";
                    break;
                case Master2BackendMessageID::MODE_CFG_MSG:
                    msgTypeStr = "MODE_CFG_MSG";
                    break;
                case Master2BackendMessageID::RST_MSG:
                    msgTypeStr = "RST_MSG";
                    break;
                case Master2BackendMessageID::CTRL_MSG:
                    msgTypeStr = "CTRL_MSG";
                    break;
                default:
                    break;
            }

            Log.d("FrameParser: packet parsed, msg type=%s (0x%02X) ",
                  msgTypeStr, packet.message_id);
            switch (static_cast<Master2BackendMessageID>(packet.message_id)) {
                case Master2BackendMessageID::SLAVE_CFG_MSG: {
                    Log.d("FrameParser: processing SLAVE_CFG_MSG message");
                    auto msg = std::make_unique<Master2Backend::SlaveCfgMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Master2BackendMessageID::MODE_CFG_MSG: {
                    Log.d("FrameParser: processing MODE_CFG_MSG message");
                    auto msg = std::make_unique<Master2Backend::ModeCfgMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Master2BackendMessageID::RST_MSG: {
                    Log.d("FrameParser: processing RST_MSG message");
                    auto msg = std::make_unique<Master2Backend::RstMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Master2BackendMessageID::CTRL_MSG: {
                    Log.d("FrameParser: processing CTRL_MSG message");
                    auto msg = std::make_unique<Master2Backend::CtrlMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                default:
                    Log.e(
                        "FrameParser: unsupported Master2Backend message "
                        "type=0x%02X",
                        static_cast<uint8_t>(packet.message_id));
                    return nullptr;
            }
        } else if (header.packet_id ==
                   static_cast<uint8_t>(PacketType::Slave2Backend)) {
            Slave2BackendPacket packet;
            if (!packet.deserialize(packet_data)) {
                Log.e("Failed to deserialize packet");
                return nullptr;
            }

            const char* msgTypeStr = "Unknown";
            switch (static_cast<Slave2BackendMessageID>(packet.message_id)) {
                case Slave2BackendMessageID::COND_DATA_MSG:
                    msgTypeStr = "COND_DATA_MSG";
                    break;
                case Slave2BackendMessageID::RES_DATA_MSG:
                    msgTypeStr = "RES_DATA_MSG";
                    break;
                case Slave2BackendMessageID::CLIP_DATA_MSG:
                    msgTypeStr = "CLIP_DATA_MSG";
                    break;
                default:
                    break;
            }

            Log.d("FrameParser: packet parsed, msg type=%s (0x%02X) ",
                  msgTypeStr, packet.message_id);
            switch (static_cast<Slave2BackendMessageID>(packet.message_id)) {
                case Slave2BackendMessageID::COND_DATA_MSG: {
                    Log.d("FrameParser: processing COND_DATA_MSG message");
                    auto msg = std::make_unique<Slave2Backend::CondDataMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Slave2BackendMessageID::RES_DATA_MSG: {
                    Log.d("FrameParser: processing RES_DATA_MSG message");
                    auto msg = std::make_unique<Slave2Backend::ResDataMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                case Slave2BackendMessageID::CLIP_DATA_MSG: {
                    Log.d("FrameParser: processing CLIP_DATA_MSG message");
                    auto msg = std::make_unique<Slave2Backend::ClipDataMsg>();
                    msg->deserialize(packet.payload);
                    return msg;
                }
                default:
                    Log.e(
                        "FrameParser: unsupported Slave2Backend message "
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
#endif