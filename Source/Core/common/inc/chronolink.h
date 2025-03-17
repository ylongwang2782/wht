#include <array>
#include <cstdint>
#include <variant>
#include <vector>

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
        uint16_t colorSensor : 1;    // 颜色传感器匹配状态
        uint16_t sleeveLimit : 1;    // 限位开关状态
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
        uint16_t harnessLength;               // 线束数据字段长度
        std::vector<uint8_t> harnessData;    // 线束数据
        uint8_t clipLength;                  // 卡钉数据字段长度
        std::vector<uint8_t> clipData;       // 卡钉数据
    };

    struct InstructionReply {
        uint8_t type;                 // 指令类型 (0x00, 0x01, 0x02)
        uint8_t ackStatus;            // 应答状态 (0x00: ACK, 0x01: ERROR)
        std::variant<DeviceConfig,    // 设备配置
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