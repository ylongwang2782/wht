#include <array>
#include <cstdint>
#include <vector>

#include "bsp_log.hpp"
#include "bsp_uart.hpp"

#define RX_MODE      0x01 /* 接收模式 */
#define STANDBY_MODE 0x02 /* 待机模式 */

#define UCI_OK  0x00 /* 成功 */
#define UCI_ERR 0x01 /* 失败 */

class Uci {
   private:
    std::vector<uint8_t> current_packet_;
    uint32_t packet_count_;
    uint32_t lost_count_;

   public:
    Uci(Uart& uart) : uart(uart) {}
    Uart& uart;    // 串口对象的引用

    void data_send(const std::vector<uint8_t>& data) {
        uint8_t header = 0x23;    // 0b0010 (Control Packet) << 4 | 0b0011 (UWB
                                  // Data Session Control Group)
        uint8_t oid = 0x00;       // TX enable OID
        uint16_t size = data.size();
        // Serialize UCI
        current_packet_.clear();
        current_packet_.push_back(header);
        current_packet_.push_back(oid);
        // 修改为大端模式：先存高位字节，再存低位字节
        current_packet_.push_back((size >> 8) & 0xFF);
        current_packet_.push_back(size & 0xFF);
        current_packet_.insert(current_packet_.end(), data.begin(), data.end());

        // 发送UCI
        send(current_packet_);
    }

    // after send data to uci uart, check reply
    bool send_reply_check(const std::vector<uint8_t>& data, uint8_t mode) {
        std::vector<uint8_t> sendReply;

        switch (mode) {
            case RX_MODE:
                sendReply = {0x43, 0x01, 0x00, 0x01, 0x00};
                break;
            case STANDBY_MODE:
                sendReply = {0x43, 0x02, 0x00, 0x01, 0x00};
                break;
            default:
                sendReply = {0x43, 0x00, 0x00, 0x01, 0x00,
                             0x63, 0x00, 0x00, 0x01, 0x00};
        }

        if (data.size() != sendReply.size()) {
            return false;
        }
        return true;
    }

    int mode_set(uint8_t mode) {
        std::vector<uint8_t> rxModeData = {0x23, 0x01, 0x00, 0x00};
        std::vector<uint8_t> standByModeData = {0x23, 0x02, 0x00, 0x00};
        std::vector<uint8_t> response;

        switch (mode) {
            case RX_MODE:
                send(rxModeData);
                response = uart.getReceivedData();
                if (!send_reply_check(response, RX_MODE)) {
                    Log.e("Uci: RX_MODE config failed.");
                    return UCI_ERR;
                }
                return UCI_OK;

            case STANDBY_MODE:
                send(standByModeData);
                response = uart.getReceivedData();
                if (!send_reply_check(response, STANDBY_MODE)) {
                    Log.e("Uci: STANDBY_MODE config failed.");
                    return UCI_ERR;
                }
                return UCI_OK;

            default:
                return UCI_ERR;
        }
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
    void send(std::vector<uint8_t>& data) {
        uart.data_send(const_cast<uint8_t*>(data.data()), data.size());
    }

    // 初始化测试参数
    void init_packet_test() {
        packet_count_ = 0;
        lost_count_ = 0;
    }

    // 发送测试数据包
    bool send_test_packet(uint16_t packet_id) {
        std::vector<uint8_t> test_data = {
            0xAA,                                     // 测试包标识
            static_cast<uint8_t>(packet_id >> 8),     // 包ID高字节
            static_cast<uint8_t>(packet_id & 0xFF)    // 包ID低字节
        };
        data_send(test_data);
        packet_count_++;
        return true;
    }

    // 接收并验证测试包
    bool receive_test_packet(const std::vector<uint8_t>& data) {
        if (data.size() < 3 || data[0] != 0xAA) {
            return false;    // 不是测试包
        }

        uint16_t received_id = (data[1] << 8) | data[2];
        if (received_id != packet_count_ - lost_count_ - 1) {
            lost_count_++;
        }
        return true;
    }

    // 获取丢包率
    float get_packet_loss_rate() const {
        if (packet_count_ == 0) return 0.0f;
        return (static_cast<float>(lost_count_) / packet_count_) * 100.0f;
    }
};