#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <variant>
#include <vector>

#include "bsp_allocate.hpp"
#include "bsp_log.hpp"
struct DeviceStatusInfo {
    // Color sensor matching status: 0 - color not matching or no sensor; 1 -
    // color matching
    uint16_t colorSensorStatus : 1;
    // Limit switch status: 0 - probe disconnected; 1 - probe connected
    uint16_t sleeveLimitStatus : 1;
    // Button switch status: 0 - button not pressed; 1 - button pressed
    uint16_t electromagnetUnlockButtonStatus : 1;
    // Battery low power: 0 - battery normal; 1 - battery low power
    uint16_t batteryLowPowerAlarm : 1;
    // Air-tightness line status: 0 - air-tightness disconnected; 1 -
    // air-tightness connected
    uint16_t pressureSensorStatus : 1;
    // Electromagnetic lock 1 status: 0 - electromagnetic lock 1 unlocked; 1 -
    // electromagnetic lock 1 locked
    uint16_t electromagneticLock1Status : 1;
    // Electromagnetic lock 2 status: 0 - electromagnetic lock 2 unlocked; 1 -
    // electromagnetic lock 2 locked
    uint16_t electromagneticLock2Status : 1;
    // Accessory status 1: 0 - accessory 1 does not exist; 1 - accessory 1
    // exists
    uint16_t accessory1 : 1;
    // Accessory status 2: 0 - accessory 2 does not exist; 1 - accessory 2
    // exists
    uint16_t accessory2 : 1;
    uint16_t res : 7;    // Reserved bits
};

struct ConductionFrame {
    DeviceStatusInfo deviceStatusInfo;
    std::vector<uint8_t> ConductionData;
};

struct CommandFrame {
    uint8_t type;
    uint32_t slot_index;
};

class ChronoLink {
   public:
    enum type : uint8_t { SYNC, COMMAND, REPLY };

    enum cmdType : uint8_t { DEV_CONF, DATA_REQ, DEV_UNLOCK };

    enum status : uint8_t { OK, ERROR };

    struct Fragment {
        std::array<uint8_t, 2> delimiter;
        uint8_t slot;
        uint8_t type;
        uint8_t fragment_sequence;
        uint8_t more_fragments_flag;
        uint16_t len;
        std::vector<uint8_t> padding;
    };

    struct DeviceConfig {
        uint8_t timeslot;               // 时隙
        uint16_t totalHarnessNum;       // 总线束数量
        uint16_t startHarnessNum;       // 总线束数量
        uint8_t harnessNum;             // 线束检测数量
        uint8_t clipNum;                // 卡钉检测数量
        std::vector<uint8_t> resNum;    // 阻值检测索引列表
    };

    struct DeviceUnlock {
        uint8_t lockStatus;    // 锁状态
    };

    struct Instruction {
        uint8_t type;                                        // 指令类型
        std::vector<uint8_t> targetID;                       // 目标 ID 列表
        std::variant<DeviceConfig, DeviceUnlock> context;    // 指令内容
    };

    struct DeviceStatus {
        uint16_t colorSensor : 1;                  // 颜色传感器匹配状态
        uint16_t sleeveLimit : 1;                  // 限位开关状态
        uint16_t electromagnetUnlockButton : 1;    // 电磁锁解锁按钮状态
        uint16_t batteryLowPowerAlarm : 1;         // 电池低电量报警
        uint16_t pressureSensor : 1;               // 气压传感器状态
        uint16_t electromagneticLock1 : 1;         // 电磁锁1状态
        uint16_t electromagneticLock2 : 1;         // 电磁锁2状态
        uint16_t accessory1 : 1;                   // 辅件1状态
        uint16_t accessory2 : 1;                   // 辅件2状态
        uint16_t reserved : 7;                     // 保留字段
    };

    struct DataReplyContext {
        DeviceStatus deviceStatus;           // 状态字
        uint8_t harnessLength;               // 线束数据字段长度
        std::vector<uint8_t> harnessData;    // 线束数据
        uint8_t clipLength;                  // 卡钉数据字段长度
        std::vector<uint8_t> clipData;       // 卡钉数据
    };

    struct InstructionReply {
        uint8_t type;                     // 指令类型 (0x00, 0x01, 0x02)
        uint8_t ackStatus;                // 应答状态 (0x00: ACK, 0x01: ERROR)
        std::variant<DeviceConfig,        // 设备配置
                     DataReplyContext,    // 数据回复
                     DeviceUnlock>
            context;
    };

    struct DevConf {
        std::array<uint8_t, 4> ID;
        uint8_t harnessNum;
    };

    // Structure to hold the complete frame data
    struct CompleteFrame {
        uint8_t slot;
        uint8_t type;
        std::vector<uint8_t> data;
    };

    void push_back(const uint8_t* data, size_t length);
    bool parseFrameFragment(Fragment& fragment);
    bool is_data_upload = false;
    static std::vector<DevConf> sync_frame;
    static std::vector<uint8_t> ID_paired_list;
    static CommandFrame command_frame;
    static std::vector<std::array<uint8_t, 4>> instruction_list;

    static uint8_t pack(uint8_t slot, uint8_t type, uint8_t* data, uint16_t len,
                        std::vector<std::vector<uint8_t>>& output);
    void packSyncFrame(uint8_t slot, uint8_t type,
                       std::vector<std::vector<uint8_t>>& output);
    void packCommandFrame(std::vector<std::vector<uint8_t>>& output);

    void setBit(uint32_t& num, int n);

    void receiveAndAssembleFrame(
        const Fragment& fragment,
        void (*frameSorting)(ChronoLink::CompleteFrame complete_frame));
    void frameSorting(CompleteFrame complete_frame);

    Instruction parseInstruction(const std::vector<uint8_t>& rawData);

    std::vector<uint8_t> generateReplyFrame(
        uint8_t type, uint8_t ackStatus,
        const std::variant<DeviceConfig, DataReplyContext, DeviceUnlock>&
            context);

    void sendReply(uint8_t slot, uint8_t type, uint8_t instructionType,
                   uint8_t ackStatus,
                   const std::variant<DeviceConfig, DataReplyContext,
                                      DeviceUnlock>& context);

    class DeviceConfigType {
       public:
#pragma pack(push, 1)    // 设置按 1 字节对齐
        typedef struct {
            uint8_t timeslot;            // 时隙
            uint16_t totalHarnessNum;    // 总线束数量
            uint16_t startHarnessNum;    // 总线束数量
            uint8_t harnessNum;          // 线束检测数量
            uint8_t clipNum;             // 卡钉检测数量
        } __NormalCfg;
        typedef struct {
            __NormalCfg normalCfg;
            std::vector<uint8_t> resNum;    // 阻值检测索引列表
        } __DeviceConfig;
#pragma pack(pop)    // 恢复原来的对齐方式
        __DeviceConfig cfg;
        DeviceConfigType() {}
        ~DeviceConfigType() {}

        void pack(std::vector<uint8_t>& output) {
            uint8_t* p = reinterpret_cast<uint8_t*>(&cfg.normalCfg);
            output.insert(output.end(), p, p + sizeof(__NormalCfg));
            output.insert(output.end(), cfg.resNum.begin(), cfg.resNum.end());
        }
        void prase(std::vector<uint8_t>& input) {
            uint8_t* p = reinterpret_cast<uint8_t*>(&cfg.normalCfg);
            memcpy(p, input.data(), sizeof(__NormalCfg));
            cfg.resNum.assign(input.begin() + sizeof(__NormalCfg), input.end());
        }
        void prase(uint8_t* input) {
            uint8_t* p = reinterpret_cast<uint8_t*>(&cfg.normalCfg);
            memcpy(p, input, sizeof(__NormalCfg));
            cfg.resNum.assign(input + sizeof(__NormalCfg),
                              input + sizeof(__NormalCfg) + cfg.resNum.size());
        }

       private:
    };

    template <type frameType>
    class FrameBase {
       public:
        FrameBase() {
            fragment.header.delimiter[0] = 0xAB;
            fragment.header.delimiter[1] = 0xCD;
            fragment.header.slot = 0;
            fragment.header.type = frameType;
            fragment.header.fragment_sequence = 0;
        }
        void pack(uint8_t slot, uint8_t* payload, uint16_t payload_len,
                  std::vector<uint8_t>& output) {
            size_t offset = 0;
            uint8_t fragments_num =
                (payload_len + payload_size - 1) / payload_size;

            // 预留空间
            if (output.capacity() <
                sizeof(__FragmentHeader) * fragments_num + payload_len) {
                output.resize(sizeof(__FragmentHeader) * fragments_num +
                              payload_len);
            }

            output.clear();

            for (uint8_t i = 0; i < fragments_num; i++) {
                size_t current_payload_size = (i == fragments_num - 1)
                                                  ? (payload_len % payload_size)
                                                  : payload_size;
                if (current_payload_size == 0) {
                    current_payload_size = payload_size;
                }
                fragment.header.len = current_payload_size;
                fragment.header.fragment_sequence = i;
                fragment.header.more_fragments_flag = 1;
                if (i == fragments_num - 1) {
                    fragment.header.more_fragments_flag = 0;
                }
                fragment.header.slot = slot;

                memcpy(output.data() + offset, &fragment.header,
                       sizeof(__FragmentHeader));
                offset += sizeof(__FragmentHeader);

                memcpy(output.data() + offset, payload + i * payload_size,
                       current_payload_size);
                offset += current_payload_size;
            }
        }

        bool parse(std::vector<uint8_t>& recv_buf,
                   CompleteFrame& complete_frame) {
            // Fragment fragment;
            std::vector<uint8_t> payload;
            payload.reserve(payload_size);
            // fragment.padding.reserve(payload_size);

            size_t min_packet_size = 8;
            size_t index = 0;

            // Clear buffer befor assembly
            complete_frame.data.clear();

            while (index + min_packet_size <= recv_buf.size()) {
                // 检查起始标志 0xAB 0xCD
                if (recv_buf[index] != 0xAB || recv_buf[index + 1] != 0xCD) {
                    Log.w("Invalid header delimiter at index %d\n", index);
                    ++index;    // 跳过无效字节，继续检查下一个可能的帧
                    continue;
                }

                // Parse header
                fragment.header.delimiter[0] = recv_buf[index++];
                fragment.header.delimiter[1] = recv_buf[index++];

                // Parse slot, type, fragment_sequence, and more_fragments_flag
                fragment.header.slot = recv_buf[index++];
                fragment.header.type = recv_buf[index++];
                fragment.header.fragment_sequence = recv_buf[index++];
                fragment.header.more_fragments_flag = recv_buf[index++];

                // Parse length (little-endian)
                fragment.header.len = static_cast<uint16_t>(
                    recv_buf[index] | (recv_buf[index + 1] << 8));
                index += 2;

                // Parse padding (data field)
                fragment.payload.assign(
                    recv_buf.begin() + index,
                    recv_buf.begin() + index + fragment.header.len);

                complete_frame.data.insert(complete_frame.data.end(),
                                           fragment.payload.begin(),
                                           fragment.payload.end());

                // Check if this is the last fragment
                if (fragment.header.more_fragments_flag == 0) {
                    complete_frame.slot = fragment.header.slot;
                    complete_frame.type = fragment.header.type;
                    return true;
                }
                index += fragment.header.len;
            }
            return false;
        }

       private:
#pragma pack(push, 1)    // 设置按 1 字节对齐
        typedef struct {
            uint8_t delimiter[2];
            uint8_t slot;
            uint8_t type;
            uint8_t fragment_sequence;
            uint8_t more_fragments_flag;
            uint16_t len;
        } __FragmentHeader;

        typedef struct {
            __FragmentHeader header;
            std::vector<uint8_t> payload;
        } __Fragment;
#pragma pack(pop)    // 恢复原来的对齐方式
        __Fragment fragment;
    };

    class CommandFrameType : private FrameBase<COMMAND>, DeviceConfigType {
       public:
#pragma pack(push, 1)    // 设置按 1 字节对齐

        typedef struct {
            uint8_t type;           // 指令类型
            uint8_t targetID[4];    // 目标 ID 列表
        } InstructionHeader;
#pragma pack(pop)    // 恢复原来的对齐方式
        using DeviceConfig = __DeviceConfig;
        CommandFrameType() {}
        ~CommandFrameType() {}

       private:
    };

   private:
    std::vector<uint8_t> receive_buffer;
    static constexpr size_t payload_size = 242;
    std::vector<uint8_t> serializeSyncFrame(
        const std::vector<DevConf>& sync_frame);
    std::vector<uint8_t> serializeCommandFrame(
        const CommandFrame& command_frame);
    status parseDeviceConfigInfo(const std::vector<uint8_t>& data,
                                 std::vector<DevConf>& device_configs);
};