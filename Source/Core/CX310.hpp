#include <array>
#include <cstdint>
#include <vector>

#include "bsp_log.hpp"
#include "bsp_uart.hpp"

#define UCI_OK  0x00 /* 成功 */
#define UCI_ERR 0x01 /* 失败 */

typedef uint8_t byte;

class CX310Class {
   private:
    std::vector<uint8_t> current_packet_;

   public:
    /**
     * @brief MT Values
     * 消息类型值
     * 消息类型值用于标识消息的类型。
     * 消息类型值的范围为0x00-0x03。
     */
    static constexpr byte CONTROL_PACKET_RFU = 0x00;    // 0b000 预留
    static constexpr byte CONTROL_PACKET_CMD = 0x01;    // 0b001 命令消息
    static constexpr byte CONTROL_PACKET_RSP = 0x02;    // 0b010 响应消息
    static constexpr byte CONTROL_PACKET_NTF = 0x03;    // 0b011 通知消息

    /**
     * @brief PBF Values
     * 分组边界标志(PBF)常量
     */
    static constexpr byte PBF_COMPLETE_MESSAGE = 0x00;
    static constexpr byte PBF_SEGMENTED_MESSAGE = 0x01;

    /**
     * @brief GID
     *
     */
    static constexpr byte UWB_DATA_CONTROL_GROUP = 0x03;    // 0b0011

    /**
     * @brief OID
     * 应用数据控制组
     * 应用数据控制组用于控制应用数据的发送、接收和配置。
     * 应用数据控制组的命令ID范围为0x00-0x03。
     */
    // 应用数据发送命令
    static constexpr byte APP_DATA_TX_CMD = 0x00;    // 0b000000
    static constexpr byte APP_DATA_TX_RSP = 0x00;    // 响应
    static constexpr byte APP_DATA_TX_NTF = 0x00;    // 通知
    // 应用数据接收命令
    static constexpr byte APP_DATA_RX_CMD = 0x01;    // 0b000001
    static constexpr byte APP_DATA_RX_RSP = 0x01;    // 响应
    static constexpr byte APP_DATA_RX_NTF = 0x01;    // 通知
    // 应用数据停止接收命令
    static constexpr byte APP_DATA_STOP_RX_CMD = 0x02;    // 0b000010
    static constexpr byte APP_DATA_STOP_RX_RSP = 0x02;    // 响应
    // 应用数据配置设置命令
    static constexpr byte APP_DATA_CFG_SET_CMD = 0x03;    // 0b000011
    static constexpr byte APP_DATA_CFG_SET_RSP = 0x03;    // 响应
    // 应用数据配置获取命令
    static constexpr byte APP_DATA_CFG_GET_CMD = 0x04;    // 0b000100
    static constexpr byte APP_DATA_CFG_GET_RSP = 0x04;    // 响应

    /**
     * @brief 应用数据配置ID常量
     * 应用数据配置ID用于标识应用数据的配置项。
     * 应用数据配置ID的范围为0x00-0x0F
     */
    static constexpr byte STATUS_OK = 0x00;          // 成功
    static constexpr byte STATUS_REJECTED = 0x01;    // 操作当前不支持
    static constexpr byte STATUS_FAILED = 0x02;      // 操作失败
    static constexpr byte STATUS_SYNTAX_ERROR = 0x03;    // 数据格式不符合规范
    static constexpr byte STATUS_INVALID_PARAM = 0x04;    // 配置ID不正确
    static constexpr byte STATUS_INVALID_RANGE = 0x05;
    static constexpr byte STATUS_INVALID_MESSAGE_SIZE = 0x06;
    static constexpr byte STATUS_UNKNOWN_GID = 0x07;    // UCI的GID错误
    static constexpr byte STATUS_UNKNOWN_OID = 0x08;    // UCI的OID错误
    static constexpr byte STATUS_READ_ONLY = 0x09;    // 配置ID是只可读的
    static constexpr byte STATUS_COMMAND_RETRY = 0x0A;    // UWBS请求从主机重传
    static constexpr byte STATUS_RFU_B = 0x0B;            // 预留值
    static constexpr byte STATUS_RFU_C = 0x0C;            // 预留值
    static constexpr byte STATUS_RFU_D = 0x0D;            // 预留值
    static constexpr byte STATUS_RFU_E = 0x0E;            // 预留值
    static constexpr byte STATUS_RFU_F = 0x0F;            // 预留值

    CX310Class(Uart& uart) : uart(uart) {}
    Uart& uart;    // 串口对象的引用

    void startTransmit(const std::vector<uint8_t>& data) {
        byte header = (CONTROL_PACKET_CMD << 5) | (PBF_COMPLETE_MESSAGE << 4) |
                      UWB_DATA_CONTROL_GROUP;
        byte oid = APP_DATA_TX_CMD;

        // 发送UCI
        send(header, oid, data);
    }

    void setChannel(byte channel) {
        byte header = (CONTROL_PACKET_CMD << 5) | (PBF_COMPLETE_MESSAGE << 4)
        |
                      UWB_DATA_CONTROL_GROUP;
        byte oid = APP_DATA_CFG_SET_CMD;
        byte paramNum = 1;
        byte paramID = 4;
        byte paramLen = 1;
        byte paramValue = channel;
        const std::vector<byte> payload = {paramNum, paramID, paramLen,
                                           paramValue};
        send(header, oid, payload);

        // const std::array<byte, 8> rxModeData = {0x23, 0x03, 0x00, 0x04,
        //                                   0x01, 0x04, 0x01, 0x09};
        // uart.send(rxModeData.data(), rxModeData.size());
        // uart.data_send(rxModeData.data(), rxModeData.size());
    }

    void newReceive() {
        byte header = (CONTROL_PACKET_CMD << 5) | (PBF_COMPLETE_MESSAGE << 4) |
                      UWB_DATA_CONTROL_GROUP;
        byte oid = APP_DATA_RX_CMD;

        // send(header, oid, data);
    }

    void setRxMode() {
        std::vector<byte> rxModeData = {0x23, 0x01, 0x00, 0x00};
        uart.send(rxModeData.data(), rxModeData.size());
    }

    void setStandbyMode() {
        std::vector<byte> standByModeData = {0x23, 0x02, 0x00, 0x00};
        uart.send(standByModeData.data(), standByModeData.size());
    }

    // 接收数据解析函数
    std::vector<uint8_t> parse_received_data(
        const std::vector<uint8_t>& rx_data) {
        std::vector<uint8_t> payload;

        // 检查最小长度和包头(0x63)
        if (rx_data.size() < 6 || rx_data[0] != 0x63) {
            Log.e("UCI: Invalid packet header or length");
            return payload;    // 返回空vector表示错误
        }

        // Parse data length (little-endian format)
        uint16_t data_len = (rx_data[5] << 8) | rx_data[4];

        // 检查数据长度是否匹配
        if (rx_data.size() != 6 + data_len) {
            Log.e("UCI: Packet length mismatch");
            return payload;
        }

        // 提取有效载荷数据(跳过前6字节头部)
        payload.insert(payload.end(), rx_data.begin() + 6, rx_data.end());

        return payload;
    }

   private:
    void send(byte header, byte oid, const std::vector<byte>& payload) {
        // 计算数据长度
        uint16_t data_len = payload.size();
        // 构建UCI
        std::vector<byte> uci;
        uci.push_back(header);
        uci.push_back(oid);
        uci.push_back((data_len >> 8) & 0xFF);    // 高字节
        uci.push_back(data_len & 0xFF);           // 低字节
        uci.insert(uci.end(), payload.begin(), payload.end());
        uart.data_send(uci.data(), uci.size());
    }
};
