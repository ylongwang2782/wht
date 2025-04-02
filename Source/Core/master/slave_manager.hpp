#ifndef SLAVE_MANAGER_HPP
#define SLAVE_MANAGER_HPP

#include <cstdint>
#include <cstdio>
#include <vector>

#include "TaskCPP.h"
#include "bsp_log.hpp"
#include "bsp_uart.hpp"
#include "com_list.hpp"
#include "master_cfg.hpp"
#include "master_def.hpp"
#include "protocol.hpp"

using namespace ComList;
extern Logger Log;
extern Uart usart1;
extern Uart usart2;

class __ProcessBase {
   public:
    __ProcessBase(ManagerDataTransferMsg& __transfer_msg)
        : transfer_msg(__transfer_msg) {}

   public:
    ManagerDataTransferMsg& transfer_msg;
    static bool rsp_parsed;
    static Slave2MasterMessageID expected_rsp_msg_id;
   private:
    uint8_t send_cnd = 0;
    std::vector<uint8_t> rsp_data;
    FrameParser frame_parser;

   public:
    // virtual bool slave_response_process() = 0;

    uint32_t get_id(uint8_t id[4]) {
        uint32_t target_id;
        memcpy(&target_id, id, 4);
        return target_id;
    }

    bool send_frame(std::vector<uint8_t>& frame, bool rsp = true) {
        send_cnd = 0;
        while (send_cnd < SlaveManager_TX_RETRY_TIMES + 1) {
            if (__send(frame)) {
                if (!rsp) {
                    return true;    // 直接返回，不等待从机回复
                }

                // 发送成功，等待从机回复
                if (transfer_msg.rx_done_sem.take(SlaveManager_RSP_TIMEOUT) ==
                    false) {
                    Log.i(
                        "SlaveManager: rx_done_sem.take failed, slave no "
                        "response");

                } else if (__rsp_process()) {
                    Log.i("SlaveManager: slave response process success");
                    return true;
                }

                send_cnd++;
                if (send_cnd < SlaveManager_TX_RETRY_TIMES + 1) {
                    Log.i("SlaveManager: send retry %d", send_cnd);
                }

            } else {
                // 发送失败
                break;
            }
        }

        return false;
    }

   private:
    bool __send(std::vector<uint8_t>& frame) {
        // 将发送数据写入队列
        for (auto it = frame.begin(); it != frame.end(); it++) {
            if (transfer_msg.tx_data_queue.add(
                    *it, SlaveManager_TX_QUEUE_TIMEOUT) == false) {
                Log.e("SlaveManager: tx_data_queue.add failed");
                return false;
            }
        }

        // 请求数据发送
        transfer_msg.tx_request_sem.give();

        // 等待数据发送完成
        if (transfer_msg.tx_done_sem.take(SlaveManager_TX_TIMEOUT) == false) {
            Log.e("SlaveManager: tx_done_sem.take failed, timeout");
            return false;
        }
        return true;
    }

    bool __rsp_process() {
        uint16_t rsp_len = transfer_msg.rx_data_queue.waiting();
        if (rsp_data.capacity() < rsp_len) {
            rsp_data.resize(rsp_len);
        }
        // 解析从机返回的数据
        rsp_data.clear();
        uint8_t data;
        while (transfer_msg.rx_data_queue.pop(data, 0)) {
            // 处理接收到的数据
            rsp_data.push_back(data);
        }
        // 解析数据
        auto msg = frame_parser.parse(rsp_data);
        if (msg != nullptr) {
            // 处理解析后的数据
            msg->process();
        } else {
            Log.e("SlaveManager: parse failed");
        }
        return rsp_parsed;
    }
};

class DeviceConfigProcessor : private __ProcessBase {
   public:
    DeviceConfigProcessor(ManagerDataTransferMsg& __transfer_msg)
        : __ProcessBase(__transfer_msg),
          wirte_cond_info_msg(),
          write_clip_info_msg(),
          write_res_info_msg() {}

   private:
    WriteCondInfoMsg wirte_cond_info_msg;
    WriteClipInfoMsg write_clip_info_msg;
    WriteResInfoMsg write_res_info_msg;

   public:
    // bool slave_response_process() override { return true; }
    bool process(CfgCmd& cfg_cmd, uint8_t timeSlot) {
        bool ret = true;
        if (!__cond_config(cfg_cmd, timeSlot)) {
            Log.e("SlaveManager: cond config failed");
            ret = false;
        } else {
            Log.i("SlaveManager: cond config success");
        }

        if (cfg_cmd.clip_exist) {
            if (!__clip_config(cfg_cmd)) {
                Log.e("SlaveManager: clip config failed");
                ret = false;
            }
        } else {
            Log.i("SlaveManager: clip config success");
        }
        return ret;
    }

   private:
    bool __cond_config(CfgCmd& cfg_cmd, uint8_t timeSlot) {
        Log.i("SlaveManager: cond config start");
        wirte_cond_info_msg.timeSlot = timeSlot;

        // 配置设备检测线数
        wirte_cond_info_msg.conductionNum = cfg_cmd.cond;

        // 配置起始检测线数
        wirte_cond_info_msg.startConductionNum = cfg_cmd.startHarnessNum;

        // 配置总检测线数
        wirte_cond_info_msg.totalConductionNum = cfg_cmd.totalHarnessNum;

        // 配置检测间隔
        wirte_cond_info_msg.interval = CONDUCTION_TEST_INTERVAL;

        // 打包数据
        uint32_t target_id = get_id(cfg_cmd.id);
        auto cond_packet =
            PacketPacker::masterPack(wirte_cond_info_msg, target_id);
        auto cond_frame = FramePacker::pack(cond_packet);

        // 设置预期回复消息ID
        expected_rsp_msg_id = Slave2MasterMessageID::COND_INFO_MSG;

        // 发送数据
        return send_frame(cond_frame);
    }

    bool __clip_config(CfgCmd& cfg_cmd) {
        Log.i("SlaveManager: clip config start");

        write_clip_info_msg.clipPin = cfg_cmd.clip_pin;
        write_clip_info_msg.mode = cfg_cmd.clip_mode;
        write_clip_info_msg.interval = CLIP_TEST_INTERVAL;
        // 打包数据
        uint32_t target_id = get_id(cfg_cmd.id);
        auto clip_packet =
            PacketPacker::masterPack(write_clip_info_msg, target_id);
        auto clip_frame = FramePacker::pack(clip_packet);

        // 设置预期回复消息ID
        expected_rsp_msg_id = Slave2MasterMessageID::CLIP_INFO_MSG;

        // 发送数据
        return send_frame(clip_frame);
    }

    bool __res_config(CfgCmd& cfg_cmd) { return true; }
};

class DeviceModeProcessor : private __ProcessBase {
   public:
    DeviceModeProcessor(ManagerDataTransferMsg& __transfer_msg)
        : __ProcessBase(__transfer_msg), mode(CONDUCTION_TEST) {}

   private:
   public:
    SysMode mode;

   public:
    bool process(ModeCmd mode_cmd) {
        mode = mode_cmd.mode;
        Log.i("SlaveManager: mode config : %u", mode);
        return true;
    }
};

class DeviceCtrlProcessor : private __ProcessBase {
   public:
    DeviceCtrlProcessor(ManagerDataTransferMsg& __transfer_msg)
        : __ProcessBase(__transfer_msg) {}

   public:
   private:
    SyncMsg sync_msg;

   public:
    bool process(CtrlCmd& ctrl_cmd, SysMode mode) {
        Log.i("SlaveManager: ctrl config start");
        sync_msg.mode = mode;
        sync_msg.timestamp = 0;
        
        // 打包数据
        auto sync_packet = PacketPacker::masterPack(sync_msg, 0);
        auto sync_frame = FramePacker::pack(sync_packet);

        // 发送数据
        return send_frame(sync_frame, false);
    }
};

class ManagerDataTransfer : public TaskClassS<ManagerDataTransfer_STACK_SIZE> {
   public:
    ManagerDataTransfer(ManagerDataTransferMsg& __manager_transfer_msg)
        : TaskClassS("SlaveDataTransfer", TaskPrio_High),
          transfer_msg(__manager_transfer_msg) {}

   private:
    ManagerDataTransferMsg& transfer_msg;

   private:
    void task() override {
        Log.i("SlaveDataTransfer_Task: Boot");
        uint8_t data;
        uint8_t buffer[DMA_RX_BUFFER_SIZE];
        for (;;) {
            if (transfer_msg.tx_request_sem.take(0)) {
                // while (transfer_msg.tx_data_queue.pop(data, 0)) {
                //     usart2.send(&data, 1);
                // }
                transfer_msg.tx_done_sem.give();
            }
            if (xSemaphoreTake(usart2_info.dmaRxDoneSema, 0) == pdPASS) {
                // uint16_t len =
                //     usart2.getReceivedData(buffer, DMA_RX_BUFFER_SIZE);
                // for (int i = 0; i < len; i++) {
                //     transfer_msg.rx_data_queue.add(buffer[i]);
                // }
                transfer_msg.rx_done_sem.give();
            }
            TaskBase::delay(10);
        }
    }
};

class SlaveManager : public TaskClassS<SlaveManager_STACK_SIZE> {
   public:
    struct slave_dev {
        uint8_t id[4];
        uint8_t timeSlot;
    };
    SlaveManager(PCmanagerMsg& _pc_manager_msg,
                 ManagerDataTransferMsg& __manager_transfer_msg)
        : TaskClassS("SlaveManager", TaskPrio_Mid),
          pc_manager_msg(_pc_manager_msg),
          cfg_processor(__manager_transfer_msg),
          mode_processor(__manager_transfer_msg),
          ctrl_processor(__manager_transfer_msg) {}

   private:
    PCmanagerMsg& pc_manager_msg;
    DeviceConfigProcessor cfg_processor;
    DeviceModeProcessor mode_processor;
    DeviceCtrlProcessor ctrl_processor;

    DataForward forward_data;

    uint8_t timeSlot = 0;

   private:
    void task() override {
        Log.i("SlaveManager_Task: Boot");
        for (;;) {
            // 从pc_manager_msg中获取数据
            while (pc_manager_msg.data_forward_queue.pop(forward_data) ==
                   pdPASS) {
                Log.i("SlaveManager: Forward data received");
                switch ((uint8_t)forward_data.type) {
                    case CmdType::DEV_CONF: {
                        Log.i("SlaveManager: Config data received");
                        if (cfg_processor.process(forward_data.cfg_cmd,
                                                  timeSlot)) {
                            // 配置成功
                            pc_manager_msg.event.set(CONFIG_SUCCESS_EVENT);
                        } else {
                            // 配置失败
                            pc_manager_msg.event.clear(CONFIG_SUCCESS_EVENT);
                        }

                        if (forward_data.cfg_cmd.is_last_dev) {
                            timeSlot = 0;
                        } else {
                            timeSlot++;
                        }

                        break;
                    }
                    case (uint8_t)CmdType::DEV_MODE: {
                        Log.i("SlaveManager: Mode data received");
                        if(mode_processor.process(forward_data.mode_cmd)){
                            pc_manager_msg.event.set(MODE_SUCCESS_EVENT);
                        } else {
                            pc_manager_msg.event.clear(MODE_SUCCESS_EVENT);
                        }
                        break;
                    }
                    case (uint8_t)CmdType::DEV_RESET: {
                        Log.i("SlaveManager: Reset data received");
                        break;
                    }
                    case (uint8_t)CmdType::DEV_CTRL: {
                        Log.i("SlaveManager: Ctrl data received");
                        if (ctrl_processor.process(forward_data.ctrl_cmd,
                                                   mode_processor.mode)) {
                            pc_manager_msg.event.set(CTRL_SUCCESS_EVENT);
                        }
                        else {
                            pc_manager_msg.event.clear(CTRL_SUCCESS_EVENT);
                        }
                        break;
                    }
                    case (uint8_t)CmdType::DEV_QUERY: {
                        Log.i("SlaveManager: Query data received");
                        break;
                    }
                    default:
                        break;
                }
                pc_manager_msg.event.set(FORWARD_SUCCESS_EVENT);
            }
            // vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
};

#endif