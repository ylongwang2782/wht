#include <array>
#include <cstdint>
#include <vector>

class ChronoLink {
   public:
    struct Frame {
        std::array<uint8_t, 2> delimiter;
        uint16_t len;
        uint8_t slot;
        uint8_t type;
        uint8_t fragment_sequence;
        uint8_t more_fragments_flag;

        std::vector<uint8_t> padding;
    };

    struct DeviceConfigInfo {
        std::array<uint8_t, 4> ID;
        uint8_t pin_num;
    };
    static std::vector<DeviceConfigInfo> sync_frame;

    struct DeviceStatusInfo {
        // 颜色传感器匹配状态：0，颜色不匹配或无传感器；1，颜色匹配
        uint16_t colorSensorStatus : 1;
        // 限位开关状态：0，探针断开；1，探针导通
        uint16_t sleeveLimitStatus : 1;
        // 按钮开关状态: 0，按钮未按下；1，按钮按下
        uint16_t electromagnetUnlockButtonStatus : 1;
        // 电池低电量: 0，电池正常；1，电池低电量
        uint16_t batteryLowPowerAlarm : 1;
        // 气密性线状态：0，气密性断开；1，气密性导通
        uint16_t pressureSensorStatus : 1;
        // 电磁锁1状态：0，电磁锁1未锁；1，电磁锁1上锁
        uint16_t electromagneticLock1Status : 1;
        // 电磁锁2状态：0，电磁锁2未锁；1，电磁锁2上锁
        uint16_t electromagneticLock2Status : 1;
        // 辅件状态1：0，附件1不存在；1，附件1存在
        uint16_t accessory1 : 1;
        // 辅件状态2：0，附件2不存在；1，附件2存在
        uint16_t accessory2 : 1;
        uint16_t res : 7;
    };
    struct ConductionFrame {
        DeviceStatusInfo deviceStatusInfo;
        std::vector<uint8_t> ConductionData;
    };

    static std::vector<std::vector<uint8_t>> castFrameBytes;

    static std::vector<std::array<uint8_t, 4>> instruction_list;

    static uint8_t pack(uint8_t slot, uint8_t type, uint8_t* data, uint16_t len,
                        std::vector<std::vector<uint8_t>>& output);

    void packSyncFrame(uint8_t slot, uint8_t type,
                       std::vector<std::vector<uint8_t>>& output);

   private:
    static constexpr size_t payload_size = 242;
    std::vector<uint8_t> serializeSyncFrame(
        const std::vector<DeviceConfigInfo>& sync_frame);
};