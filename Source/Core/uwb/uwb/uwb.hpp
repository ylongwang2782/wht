#ifndef _UWB_HPP
#define _UWB_HPP

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <queue>
#include <vector>

#include "cx_uci.hpp"

// #include "bsp_log.hpp"
// extern Logger Log;

// extern Uart log_com;
// #define DEBUG_IN                                  \
//     if (ds) {                                     \
//         ds = false;                               \
//         log_com.data_send((uint8_t*)"in\r\n", 5); \
//     }

#define UWB_GENERAL_TIMEOUT_MS 1000

class CxUwbInterface {
   public:
    CxUwbInterface() = default;

   public:
    /* 复位引脚初始化 */
    virtual void reset_pin_init() = 0;

    /* 产生复位信号 */
    virtual void generate_reset_signal() = 0;

    /* 关闭复位信号 */
    virtual void turn_of_reset_signal() = 0;

    /* 芯片使能引脚初始化 */
    virtual void chip_en_init() = 0;

    /* 使能芯片 */
    virtual void chip_enable() = 0;

    /* 禁用芯片 */
    virtual void chip_disable() = 0;

    /* 通信接口初始化 */
    virtual void commuication_peripheral_init() = 0;

    /**
     * @brief 发送数据
     * @param tx_data 发送数据
     * @return 发送成功返回true，失败返回false
     */
    virtual bool send(std::vector<uint8_t>& tx_data) = 0;

    /**
     * @brief 接收数据
     * @param rx_data 接收数据
     * @return 接收成功返回true，失败返回false
     */
    // virtual bool get_recv_data(std::vector<uint8_t>& rx_data) = 0;
    virtual bool get_recv_data(std::queue<uint8_t>& rx_data) = 0;

    /* 获取系统1ms时间戳 */
    virtual uint32_t get_system_1ms_ticks() = 0;

    /* 延迟1ms */
    virtual void delay_ms(uint32_t ms) = 0;

    /* 日志输出 */
    virtual void log(const char* format, ...) {}
};

template <class Interface>
class UWB {
   public:
    explicit UWB() : interface() { __init(); }
    explicit UWB(const Interface& i) : interface(i) { __init(); }

    ~UWB() {}

   private:
    enum UwbsSTA : uint8_t { BOOT = 0, READY, ACTIVE, ERROR };

    Interface interface;
    UwbsSTA uwbs_sta = BOOT;

    UciCtrlPacket recv_packet;
    UciCMD uci_cmd;
    UciNTF uci_ntf;

    std::vector<uint8_t> rx_raw_buffer_vec;
    std::vector<uint8_t> rx_payload;
    std::queue<uint8_t, std::deque<uint8_t>> rx_data_queue;
    std::queue<uint8_t, std::deque<uint8_t>> transparent_data;
    uint8_t _data;

    std::function<bool(const UciCtrlPacket&)> check_rsp = nullptr;
    std::function<bool()> cmd_packer = nullptr;
    // bool ds = false;

    /**
     * @brief 初始化
     */
   public:
    /**
     * @brief 初始化
     * @return 初始化成功返回true，失败返回false
     */
    bool reset(uint16_t timeout_ms = UWB_GENERAL_TIMEOUT_MS) {
        uwbs_sta = BOOT;
        uci_cmd.core_device_reset();
        check_rsp = [this](const UciCtrlPacket& rsp) {
            return uci_cmd.check_core_device_reset_rsp(rsp);
        };

        cmd_packer = [this]() { return uci_cmd.core_device_reset(); };

        if (__send_packet()) {
            uint32_t start_tick = interface.get_system_1ms_ticks();
            while (interface.get_system_1ms_ticks() - start_tick < timeout_ms) {
                update();
                if (uwbs_sta == READY) {
                    interface.log("[UWB]: UWBS software reset successfully");
                    return true;
                }
            }
        }
        interface.log("[UWB]: error: software reset fail");
        interface.log("[UWB]: error: hardware reset start");

        interface.generate_reset_signal();
        interface.delay_ms(100);
        interface.turn_of_reset_signal();
        uint32_t start_tick = interface.get_system_1ms_ticks();
        while (interface.get_system_1ms_ticks() - start_tick < timeout_ms) {
            update();
            if (uwbs_sta == READY) {
                interface.log("[UWB]: hardware reset successfully");
                return true;
            }
        }
        interface.log("[UWB]: error: UWBS hardware reset failed");
        return false;
    }

    /**
     * @brief 数据透传
     * @param data 发送数据
     * @return 发送成功返回true，失败返回false
     */
    bool data_transmit(const std::vector<uint8_t>& data) {
        if (uwbs_sta != READY) {
            interface.log("[UWB]: error: UWBS not ready");
            return false;
        }

        if (data.size() == 0) {
            return true;
        }

        cmd_packer = [this, &data]() { return uci_cmd.cx_app_data_tx(data); };
        check_rsp = [this](const UciCtrlPacket& rsp) {
            return uci_cmd.check_cx_app_data_tx_rsp(rsp);
        };

        if (__send_packet()) {
            // interface.log("[UWB]: data transmit");
            return true;
        }
        interface.log("[UWB]: error: data transmit fail");
        return false;
    }

    bool data_transmit_tx_test(uint16_t pack_size, uint16_t pack_num) {
        if (uwbs_sta != READY) {
            interface.log("[UWB]: error: UWBS not ready");
            return false;
        }
        std::vector<uint8_t> data(pack_size, 0xff);

        int delay_cnt = 20;
        for (uint16_t i = 0; i < pack_num; i++) {
            data[0] = i & 0xff;
            data[1] = i >> 8;
            data_transmit(data);

            while (delay_cnt--);
            delay_cnt = 20;
        }
        return true;
    }

    bool data_transmit_rx_test(uint16_t pack_size, uint16_t pack_num) {
        if (uwbs_sta != READY) {
            interface.log("[UWB]: error: UWBS not ready");
            return false;
        }
        std::vector<uint8_t> data(pack_size);
        uint16_t loss_pack = 0;
        uint32_t time_ms = 0;
        bool start_flag = false;
        uint16_t recv_cnt = 0;
        uint16_t pack_id = 0;
        uint32_t start_tick = 0;
        while (true) {
            if (get_recv_data(data)) {
                pack_id = data[0] | (data[1] << 8);
                if (!start_flag) {
                    if (pack_id == 0) {
                        start_flag = true;
                        recv_cnt = 1;
                        start_tick = interface.get_system_1ms_ticks();
                    }
                } else {
                    if (pack_id < recv_cnt) {
                        interface.log(
                            "[UWB]: error: pack id = %u, recv_cnt = %u",
                            pack_id, recv_cnt);
                        return false;
                    }
                    if (pack_id != recv_cnt) {
                        loss_pack += pack_id - recv_cnt;
                        recv_cnt = pack_id;
                    }
                    recv_cnt++;
                    // interface.log("[UWB]: recv recv_cnt = %d", recv_cnt);
                    if (recv_cnt == pack_num) {
                        time_ms = interface.get_system_1ms_ticks() - start_tick;
                        interface.log(
                            "[UWB]: data transmit rx test: transmit pack = "
                            "%u, loss pack = %u, "
                            "time = %ums, transmit data = %uB",
                            pack_num, loss_pack, time_ms, pack_num * pack_size);
                        return true;
                    }
                }
            }
        }
        return true;
    }

    /**
     * @brief 设置接收模式
     * @return 设置成功返回true，失败返回false
     */
    bool set_recv_mode() {
        if (uwbs_sta != READY) {
            interface.log("[UWB]: error: UWBS not ready");
            return false;
        }

        cmd_packer = [this]() { return uci_cmd.cx_app_data_rx(); };
        check_rsp = [this](const UciCtrlPacket& rsp) {
            return uci_cmd.check_cx_app_data_rx_rsp(rsp);
        };

        if (__send_packet()) {
            interface.log("[UWB]: set recv mode");
            return true;
        }
        interface.log("[UWB]: error: set recv mode fail");
        return false;
    }

    /**
     * @brief 获取透传数据
     * @param recv_data 接收数据
     * @return 获取成功返回true，失败返回false
     */
    bool get_recv_data(std::vector<uint8_t>& recv_data) {
        update();
        if (uwbs_sta != READY) {
            interface.log("[UWB]: error: UWBS not ready");
            return false;
        }
        if (transparent_data.empty()) {
            return false;
        }
        recv_data.clear();
        recv_data.reserve(transparent_data.size());
        while (transparent_data.empty() == false) {
            recv_data.push_back(transparent_data.front());
            transparent_data.pop();
        }
        return true;
    }

    /**
     * @brief 停止接收
     * @return 停止成功返回true，失败返回false
     */
    bool stop_recv() {
        if (uwbs_sta != READY) {
            interface.log("[UWB]: error: UWBS not 22");
            return false;
        }
        cmd_packer = [this]() { return uci_cmd.cx_app_data_stop_rx(); };
        check_rsp = [this](const UciCtrlPacket& rsp) {
            return uci_cmd.check_cx_app_data_stop_rx_rsp(rsp);
        };
        if (__send_packet()) {
            interface.log("[UWB]: stop recv");
            return true;
        }
        interface.log("[UWB]: error: stop recv fail");
        return false;
    }

    /**
     * @brief 更新，监听通知和更新状态机
     * @return 无
     */
    void update() {
        __listening_ntf();
        // printf("[UWB]: update\n");
        __uwbs_state_machine();
    }

   private:
    void __delay_ms(uint32_t ms) { interface.delay_ms(ms); }
    void __uwbs_state_machine() {
        switch (uwbs_sta) {
            case BOOT: {
                // interface.log("[UWB]: boot");
                break;
            }
            case READY: {
                // interface.log("[UWB]: ready");
                break;
            }
            case ACTIVE: {
                break;
            }
            case ERROR: {
                break;
            }
        }
    }

    void __load_recv_data() {
        interface.get_recv_data(rx_data_queue);
    }

    bool __rsp_process(uint32_t timeout_ms) {
        uint32_t start_tick = interface.get_system_1ms_ticks();
        while (interface.get_system_1ms_ticks() - start_tick < timeout_ms) {
            __load_recv_data();
            while (rx_data_queue.empty() == false) {
                _data = rx_data_queue.front();
                rx_data_queue.pop();

                if (recv_packet.flow_parse(_data, rx_payload)) {
                    if (recv_packet.mt == MT_RSP) {
                        // 接收到响应
                        return true;
                    } else if (recv_packet.mt == MT_NTF) {
                        // 接收到通知
                        __notify_process();
                    }
                }
            }
        }
        interface.log("[UWB]: error: wait rsp timeout");
        return false;
    }

    void __listening_ntf() {
        __load_recv_data();
        while (rx_data_queue.empty() == false) {
            _data = rx_data_queue.front();
            rx_data_queue.pop();

            if (recv_packet.flow_parse(_data, rx_payload)) {
                if (recv_packet.mt == MT_NTF) {
                    // 接收到通知
                    printf("[UWB]: receive notify");
                    __notify_process();
                } else {
                    interface.log("[UWB]: error: unexpected rsp packet");
                }
            }
        }
    }

    void __notify_process() {
        if (recv_packet.gid == GID0x00) {
            switch (recv_packet.oid) {
                case CORE_DEVICE_STATUS_NTF: {
                    uint8_t sta =
                        uci_ntf.parse_core_device_status_ntf(rx_payload);
                    if (sta == DEVICE_STATE_READY) {
                        if (uwbs_sta == BOOT) {
                            uwbs_sta = READY;
                            printf("[UWB]: ready\n");
                            interface.log("[UWB]: UWBS move to active state");
                        }
                    }
                    break;
                }
                default: {
                    interface.log(
                        "[UWB]: warning: undealed notify: gid=0x00, oid=0x%.2X",
                        recv_packet.oid);
                    break;
                }
            }
        }
        if (recv_packet.gid == GID0x03) {
            switch (recv_packet.oid) {
                case CX_APP_DATA_TX_NTF: {
                    if (uci_ntf.parse_cx_app_data_tx_ntf(rx_payload) !=
                        STATUS_OK) {
                        interface.log("[UWB]: error: parse data tx ntf fail");
                    } else {
                        // interface.log("[UWB]: data transmit");
                    }
                    break;
                }
                case CX_APP_DATA_RX_NTF: {
                    if (!uci_ntf.parse_cx_app_data_rx_ntf(rx_payload)) {
                        interface.log("[UWB]: error: parse data rx ntf fail");
                    }

                    if (rx_payload.size() < 2) {
                        interface.log(
                            "[UWB]: error: rx payload size is too small");
                        break;
                    }
                    for (auto it = rx_payload.begin() + 2;
                         it != rx_payload.end(); it++) {
                        transparent_data.push(*it);
                    }
                    // interface.log("[UWB]: data receive, size=%u",
                    //               rx_payload.size() - 2);
                    break;
                }
                default: {
                    interface.log(
                        "[UWB]: warning: undealed notify: gid=0x03, oid=0x%.2X",
                        recv_packet.oid);
                    break;
                }
            }
        }
    }

    void __init() {
        uint32_t start_tick;
        uwbs_sta = BOOT;

        // 芯片使能引脚初始化
        interface.chip_en_init();
        // 复位引脚初始化
        interface.reset_pin_init();
        // 使能芯片
        interface.chip_disable();
        __delay_ms(100);

        // 初始化通信端口
        interface.commuication_peripheral_init();

        interface.chip_enable();
        __delay_ms(200);

        // 产生复位信号
        // interface.generate_reset_signal();
        // __delay_ms(100);

        // // 关闭复位信号
        // interface.turn_of_reset_signal();
        // __delay_ms(2000);

        // 更新通知与状态机
        start_tick = interface.get_system_1ms_ticks();
        while (uwbs_sta != READY) {    // 检查是否就绪
            update();
            if (interface.get_system_1ms_ticks() - start_tick > 5000) {
                // 超时
                interface.log("[UWB]: error: init wait ready timeout");
                break;
            }
        }
        interface.log("[UWB]: init success");
        // 复位指令
        // __delay_ms(500);
        reset(3000);
    }
    bool __send_packet() {
        if ((cmd_packer == nullptr) || (check_rsp == nullptr)) {
            return false;
        }
        bool send_flag = true;
        bool pack_all_payload = false;
        bool ret = false;
        uint8_t index = 0;
        while (1) {
            if (send_flag) {
                send_flag = false;
                pack_all_payload = cmd_packer();
                interface.send(uci_cmd.packet);
                // Log.r(uci_cmd.packet.data(), uci_cmd.packet.size());
            }
            if (__rsp_process(UWB_GENERAL_TIMEOUT_MS)) {
                if (check_rsp(recv_packet)) {
                    send_flag = true;
                    if (pack_all_payload) {
                        ret = true;
                        break;
                    }
                } else {
                    interface.log("[UWB]: error: rsp check fail");
                    ret = false;
                    break;
                }
            } else {
                ret = false;
                break;
            }
        }
        uci_cmd.reset_packer();
        cmd_packer = nullptr;
        check_rsp = nullptr;
        return ret;
    }
};
#endif