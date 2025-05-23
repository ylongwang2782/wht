#ifndef SLAVE_MANAGER_HPP
#define SLAVE_MANAGER_HPP

#include <cstdint>
#include <cstdio>
#include <vector>

#include "TaskCPP.h"
#include "TimerCPP.h"
#include "bsp_log.hpp"
#include "bsp_uart.hpp"
#include "master_cfg.hpp"
#include "master_def.hpp"
#include "protocol.hpp"
#include "uwb.hpp"
#include "uwb_interface.hpp"

extern Logger Log;
extern UasrtInfo& slave_com_info;
class __ProcessBase {
   public:
    __ProcessBase(ManagerDataTransferMsg& __transfer_msg)
        : transfer_msg(__transfer_msg) {}

   public:
    ManagerDataTransferMsg& transfer_msg;
    static bool rsp_parsed;
    static uint8_t expected_rsp_msg_id;

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

    virtual void process_rsp_data() {};

    bool send_frame(std::vector<uint8_t>& frame, bool rsp = true) {
        send_cnd = 0;
        while (send_cnd < SlaveManager_TX_RETRY_TIMES + 1) {
            if (__send(frame)) {
                if (!rsp) {
                    return true;    // 直接返回，不等待从机回复
                }
                transfer_msg.cmd_tx_sem.give();

                // 发送成功，等待从机回复
                if (transfer_msg.rx_done_sem.take(SlaveManager_RSP_TIMEOUT) ==
                    false) {
                    Log.i("SlaveManager",
                          "rx_done_sem.take failed, slave no "
                          "response");

                } else if (__rsp_process()) {
                    Log.i("SlaveManager", "slave response process success");
                    return true;
                }

                send_cnd++;
                if (send_cnd < SlaveManager_TX_RETRY_TIMES + 1) {
                    Log.i("SlaveManager", "send retry %d", send_cnd);
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
                Log.e("SlaveManager", "tx_data_queue.add failed");
                return false;
            }
        }

        // 请求数据发送
        transfer_msg.tx_request_sem.give();

        // 等待数据发送完成
        if (transfer_msg.tx_done_sem.take(SlaveManager_TX_TIMEOUT) == false) {
            Log.e("SlaveManager", "tx_done_sem.take failed, timeout");
            return false;
        }
        return true;
    }

    bool __rsp_process() {
        rsp_parsed = false;
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
        // for (auto it : rsp_data) {
        //     Log.d("SlaveManager","rx_data: 0x%02X", it);
        // }
        // 解析数据
        auto msg = frame_parser.parse(rsp_data);
        if (msg != nullptr) {
            // 处理解析后的数据
            msg->process();
            process_rsp_data();
        } else {
            Log.e("SlaveManager", "parse failed");
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
    Master2Slave::CondCfgMsg wirte_cond_info_msg;
    Master2Slave::ClipCfgMsg write_clip_info_msg;
    Master2Slave::ResCfgMsg write_res_info_msg;

    uint16_t __totalConductionNum;    // 总检测线数

   public:
    // bool slave_response_process() override { return true; }
    uint16_t totalConductionNum() { return __totalConductionNum; }
    bool process(CfgCmd& cfg_cmd, uint8_t timeSlot) {
        bool ret = true;
        if (!__cond_config(cfg_cmd, timeSlot)) {
            Log.e("SlaveManager", "cond config failed");
            ret = false;
        } else {
            Log.i("SlaveManager", "cond config success");
        }

        // if (cfg_cmd.clip_exist) {
        //     if (!__clip_config(cfg_cmd)) {
        //         Log.e("SlaveManager","clip config failed");
        //         ret = false;
        //     }
        // } else {
        //     Log.i("SlaveManager","clip config success");
        // }
        return ret;
    }

   private:
    bool __cond_config(CfgCmd& cfg_cmd, uint8_t timeSlot) {
        Log.i("SlaveManager", "cond config start");
        wirte_cond_info_msg.timeSlot = timeSlot;

        // 配置设备检测线数
        wirte_cond_info_msg.conductionNum = cfg_cmd.cond;

        // 配置起始检测线数
        wirte_cond_info_msg.startConductionNum = cfg_cmd.startHarnessNum;

        // 配置总检测线数
        wirte_cond_info_msg.totalConductionNum = cfg_cmd.totalHarnessNum;
        __totalConductionNum = cfg_cmd.totalHarnessNum;

        // 配置检测间隔
        wirte_cond_info_msg.interval = CONDUCTION_TEST_INTERVAL;

        // 打包数据
        uint32_t target_id = get_id(cfg_cmd.id);
        auto cond_packet =
            PacketPacker::master2SlavePack(wirte_cond_info_msg, target_id);
        auto cond_frame = FramePacker::pack(cond_packet);

        // 设置预期回复消息ID
        expected_rsp_msg_id = (uint8_t)(Slave2MasterMessageID::COND_CFG_MSG);

        // 发送数据
        return send_frame(cond_frame);
    }

    bool __clip_config(CfgCmd& cfg_cmd) {
        Log.i("SlaveManager", "clip config start");

        write_clip_info_msg.clipPin = cfg_cmd.clip_pin;
        write_clip_info_msg.mode = cfg_cmd.clip_mode;
        write_clip_info_msg.interval = CLIP_TEST_INTERVAL;
        // 打包数据
        uint32_t target_id = get_id(cfg_cmd.id);
        auto clip_packet =
            PacketPacker::master2SlavePack(write_clip_info_msg, target_id);
        auto clip_frame = FramePacker::pack(clip_packet);

        // 设置预期回复消息ID
        expected_rsp_msg_id = (uint8_t)(Slave2MasterMessageID::CLIP_CFG_MSG);

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
        Log.i("SlaveManager", "mode config : %u", mode);
        return true;
    }
};

class DeviceCtrlProcessor : private __ProcessBase {
   public:
    DeviceCtrlProcessor(ManagerDataTransferMsg& __transfer_msg)
        : __ProcessBase(__transfer_msg) {}

   public:
    CtrlType state() { return ctrl; }

    bool send_sync_frame() { return send_frame(sync_frame, false); }

   private:
    Master2Slave::SyncMsg sync_msg;
    CtrlType ctrl = DEV_DISABLE;
    std::vector<uint8_t> sync_frame;

   public:
    bool process(CtrlCmd& ctrl_cmd, SysMode mode) {
        Log.i("SlaveManager", "ctrl config start");
        sync_msg.mode = mode;
        sync_msg.timestamp = 0;
        ctrl = ctrl_cmd.ctrl;
        // 打包数据
        auto sync_packet = PacketPacker::master2SlavePack(sync_msg, 0xFFFFFFFF);
        sync_frame = FramePacker::pack(sync_packet);

        return true;
    }
};

class DeviceResetProcessor : private __ProcessBase {
   public:
    DeviceResetProcessor(ManagerDataTransferMsg& __transfer_msg)
        : __ProcessBase(__transfer_msg) {}

   private:
    Master2Backend::RstMsg rst_msg;

   public:
    bool process(ResetCmd& rst_cmd) { return true; }
};

class ReadCondProcessor : private __ProcessBase {
   public:
    ReadCondProcessor(ManagerDataTransferMsg& __transfer_msg)
        : __ProcessBase(__transfer_msg) {}
    uint32_t deviceID;

   private:
    Master2Slave::ReadCondDataMsg read_cond_data_msg;
    Slave2Backend::CondDataMsg upload_cond_data_msg;
    std::vector<uint8_t> upload_frame;

   public:
    const std::vector<uint8_t>& get_upload_frame() { return upload_frame; }
    void process_rsp_data() override {
        Log.v("ReadCondProcessor 3", "slaveID: %08X", deviceID);
        auto upload_msg =
            PacketPacker::slave2BackendPack(upload_cond_data_msg, deviceID);
        upload_frame = FramePacker::pack(upload_msg);
    }
    bool process(uint32_t id) {
        Log.i("ReadCondProcessor", "read cond data start");
        Log.v("ReadCondProcessor 1", "slaveID: %08X", id);
        deviceID = id;
        Log.v("ReadCondProcessor 2", "slaveID: %08X", deviceID);
        // 打包数据
        auto cond_packet =
            PacketPacker::master2SlavePack(read_cond_data_msg, id);
        auto cond_frame = FramePacker::pack(cond_packet);
        expected_rsp_msg_id = (uint8_t)(Slave2BackendMessageID::COND_DATA_MSG);
        return send_frame(cond_frame);
    }
};

class ManagerDataTransfer : public TaskClassS<ManagerDataTransfer_STACK_SIZE> {
   public:
    ManagerDataTransfer(ManagerDataTransferMsg& __manager_transfer_msg,
                        SlaveUploadTransferMgr& __slaveUploadTransferMgr)
        : TaskClassS("SlaveDataTransfer", TaskPrio_High),
          transfer_msg(__manager_transfer_msg),
          slaveUploadTransferMgr(__slaveUploadTransferMgr) {}

   private:
    ManagerDataTransferMsg& transfer_msg;
    SlaveUploadTransferMgr& slaveUploadTransferMgr;

    void task() override {
        Log.d("SlaveDataTransfer_Task", "Boot");

#ifdef SLAVE_USE_UWB
        UWB<UwbUartInterface> uwb;
        std::vector<uint8_t> buffer = {1, 2, 3, 4, 5};
        uint8_t data = 0;
        std::vector<uint8_t> rx;
        uwb.set_recv_mode();
        for (;;) {
            if (transfer_msg.tx_request_sem.take(0)) {
                buffer.reserve(transfer_msg.tx_data_queue.waiting());
                buffer.clear();
                while (transfer_msg.tx_data_queue.pop(data, 0)) {
                    buffer.push_back(data);
                }
                uwb.data_transmit(buffer);
                transfer_msg.tx_done_sem.give();
                uwb.set_recv_mode();
            }

            if (uwb.get_recv_data(buffer)) {
                if (transfer_msg.cmd_tx_sem.take(0)) {
                    for (auto it = buffer.begin(); it != buffer.end(); it++) {
                        transfer_msg.rx_data_queue.add(*it);
                    }
                    transfer_msg.rx_done_sem.give();
                } else {
                    for (auto it = buffer.begin(); it != buffer.end(); it++) {
                        slaveUploadTransferMgr.rx_data_queue.add(*it);
                    }
                    slaveUploadTransferMgr.rx_done_sem.give();
                }
            }
            uwb.update();
            TaskBase::delay(5);
        }

#else
        /* -------------------------<使用串口替代UWB调试>-------------------------
         */
        taskENTER_CRITICAL();
        UartConfig slave_com_cfg(slave_com_info, true);
        Uart slave_com(slave_com_cfg);
        taskEXIT_CRITICAL();
        std::vector<uint8_t> rx_data;
        uint8_t data;
        for (;;) {
            if (transfer_msg.tx_request_sem.take(0)) {
                while (transfer_msg.tx_data_queue.pop(data, 0)) {
                    slave_com.send(&data, 1);
                }

                transfer_msg.tx_done_sem.give();
            }
            if (xSemaphoreTake(slave_com_info.dmaRxDoneSema, 0) == pdPASS) {
                rx_data = slave_com.getReceivedData();

                for (auto it = rx_data.begin(); it != rx_data.end(); it++) {
                    transfer_msg.rx_data_queue.add(*it);
                }
                transfer_msg.rx_done_sem.give();
            }

            TaskBase::delay(10);
        }
#endif
    }
};

class SlaveManager : public TaskClassS<SlaveManager_STACK_SIZE> {
   public:
    struct SlaveDev {
        union {
            uint8_t id[4];
            uint32_t id32;
        } _ID;
        // _ID id;
        uint8_t timeSlot;
    };
    SlaveManager(PCmanagerMsg& _pc_manager_msg,
                 ManagerDataTransferMsg& __manager_transfer_msg)
        : TaskClassS("[SlaveManager]", TaskPrio_Mid),
          pc_manager_msg(_pc_manager_msg),
          cfg_processor(__manager_transfer_msg),
          mode_processor(__manager_transfer_msg),
          ctrl_processor(__manager_transfer_msg),
          read_cond_processor(__manager_transfer_msg),
          sync_timer("sync_timer", this, &SlaveManager::sync_timer_callback,
                     pdMS_TO_TICKS(500), pdTRUE) {}

   private:
    enum CfgState : uint8_t {
        CONGIG_START = 0,
        CONFIG_PROCESSING,
        CONFIG_DONE,
    };

    PCmanagerMsg& pc_manager_msg;
    DeviceConfigProcessor cfg_processor;
    DeviceModeProcessor mode_processor;
    DeviceCtrlProcessor ctrl_processor;
    ReadCondProcessor read_cond_processor;

    DataForward forward_data;
    std::vector<SlaveDev> slave_dev;
    uint8_t timeSlot = 0;
    bool running = false;
    uint16_t slave_dev_index;
    uint16_t slave_num = 0;
    CfgState cfg_state = CONGIG_START;
    bool wait_for_data = false;

    FreeRTOScpp::TimerMember<SlaveManager> sync_timer;
    BinarySemaphore sync_sem;

   private:
    uint16_t get_timer_period() {
        if (mode_processor.mode == CONDUCTION_TEST) {
            return pdMS_TO_TICKS(CONDUCTION_TEST_INTERVAL *
                                     cfg_processor.totalConductionNum() +
                                 SYNC_TIMER_PERIOD_REDUNDANCY_TICKS);
        }
        return 1000;
    }
    void sync_timer_callback() { sync_sem.give(); }
    void config_process() {
        bool ret = true;
        SlaveDev dev;
        switch (cfg_state) {
            case CONGIG_START: {
                Log.d("SlaveManager", "config process start");
                timeSlot = 0;
                slave_dev_index = 0;
                slave_num = forward_data.cfg_cmd.slave_dev_num;
                slave_dev.clear();
                slave_dev.reserve(forward_data.cfg_cmd.slave_dev_num);
                ret = cfg_processor.process(forward_data.cfg_cmd, timeSlot);
                break;
            }
            case CONFIG_PROCESSING:
                ret = cfg_processor.process(forward_data.cfg_cmd, timeSlot);
                break;
            case CONFIG_DONE:
                break;
        }
        // 注册从机设备
        memcpy(dev._ID.id, forward_data.cfg_cmd.id, 4);
        dev.timeSlot = timeSlot;
        slave_dev.push_back(dev);
        timeSlot++;
        slave_dev_index++;
        if (slave_dev_index >= forward_data.cfg_cmd.slave_dev_num) {
            // 所有从机配置完成
            cfg_state = CONGIG_START;
            Log.i("SlaveManager", "config process done, device num: %u",
                  forward_data.cfg_cmd.slave_dev_num);
        } else {
            cfg_state = CONFIG_PROCESSING;
        }

        if (ret) {
            // 配置成功
            pc_manager_msg.event.set(CONFIG_SUCCESS_EVENT);
        } else {
            // 配置失败
            pc_manager_msg.event.clear(CONFIG_SUCCESS_EVENT);
        }
    }
    void ctrl_process() {
        if (ctrl_processor.process(forward_data.ctrl_cmd,
                                   mode_processor.mode)) {
            pc_manager_msg.event.set(CTRL_SUCCESS_EVENT);
        } else {
            pc_manager_msg.event.clear(CTRL_SUCCESS_EVENT);
        }
        if (ctrl_processor.state() == DEV_ENABLE) {
            // 启动检测
            if (slave_num > 0) {
                if (running == false) {
                    running = true;
                    TickType_t period = get_timer_period();
                    Log.i("SlaveManager", "total cond num: %u",
                          cfg_processor.totalConductionNum());
                    Log.i("SlaveManager", "interval: %u",
                          CONDUCTION_TEST_INTERVAL);
                    Log.i("SlaveManager", "sync timer period: %u", period);
                    slave_dev_index = 0;
                    wait_for_data = false;
                    sync_sem.give();
                    sync_timer.period(period);
                }
            } else {
                Log.e("SlaveManager",
                      "have not config. slave "
                      "num is 0, discard "
                      "ctrl data");
            }

        } else {
            // 停止检测
            running = false;
            sync_timer.stop();
        }
    }
    void read_cond_data_process() {
        for (auto it = slave_dev.begin(); it != slave_dev.end(); it++) {
            if (read_cond_processor.process(it->_ID.id32)) {
                Log.i("SlaveManager", "read cond data success");
                if (pc_manager_msg.upload_data.get_write_access(
                        PC_TX_SHARE_MEM_ACCESS_TIMEOUT)) {
                    // 共享资源上锁，禁止外部读写
                    // pc_manager_msg.upload_data.lock();
                    /* ---------------------<上报数据>---------------------*/
                    // upload_cond_data_frame.pack(
                    //     pc_manager_msg.upload_data.get(),
                    //     read_cond_processor.get_data_msg(),
                    //     it->id);
                    pc_manager_msg.upload_data.write(
                        read_cond_processor.get_upload_frame().data(),
                        read_cond_processor.get_upload_frame().size(),
                        PC_TX_TIMEOUT);
                    // 共享资源解锁，允许读取数据
                    // pc_manager_msg.upload_data.unlock();

                    // 发送数据给上位机，释放发送请求信号量
                    pc_manager_msg.upload_request_sem.give();

                    // 等待发送完成
                    if (!pc_manager_msg.upload_done_sem.take(
                            SlaveManager_UPLOAD_TIMEOUT)) {
                        Log.e("SlaveManager",
                              ""
                              "upload_done_sem.take "
                              "failed");
                    }
                    // 释放写访问权限
                    pc_manager_msg.upload_data.release_write_access();
                }
            }
        }
    }
    void task() override {
        Log.i("SlaveManager_Task", "Boot");

        // sync_timer.period()
        for (;;) {
            // 从pc_manager_msg中获取数据
            while (pc_manager_msg.data_forward_queue.pop(forward_data, 0) ==
                   pdPASS) {
                switch ((uint8_t)forward_data.type) {
                    case CmdType::DEV_CONF: {
                        // Log.i("SlaveManager","Config data received");
                        if (!running) {
                            config_process();
                        } else {
                            Log.e("SlaveManager",
                                  "Device is running, discard "
                                  "config data");
                        }

                        break;
                    }
                    case (uint8_t)CmdType::DEV_MODE: {
                        // Log.i("SlaveManager","Mode data received");
                        if (!running) {
                            if (mode_processor.process(forward_data.mode_cmd)) {
                                pc_manager_msg.event.set(MODE_SUCCESS_EVENT);
                            } else {
                                pc_manager_msg.event.clear(MODE_SUCCESS_EVENT);
                            }
                        } else {
                            Log.e("SlaveManager",
                                  "Device is running, discard "
                                  "mode "
                                  "data");
                        }

                        break;
                    }
                    case (uint8_t)CmdType::DEV_RESET: {
                        // Log.i("SlaveManager","Reset data received");
                        break;
                    }
                    case (uint8_t)CmdType::DEV_CTRL: {
                        ctrl_process();
                        // Log.i("SlaveManager","Ctrl data received");

                        break;
                    }
                    case (uint8_t)CmdType::DEV_QUERY: {
                        // Log.i("SlaveManager","Query data received");
                        break;
                    }
                    default:
                        break;
                }
                pc_manager_msg.event.set(FORWARD_SUCCESS_EVENT);
            }

            if (ctrl_processor.state() == DEV_ENABLE) {
                if (sync_sem.take(0)) {
                    if (wait_for_data == true) {
                        sync_timer.stop();
                        switch (mode_processor.mode) {
                            case CONDUCTION_TEST: {
                                Log.i("SlaveManager", "cond data read start");
                                read_cond_data_process();
                                break;
                            }
                            case CLIP_TEST: {
                                Log.i("SlaveManager", "clip data read start");
                                break;
                            }
                            case IMPEDANCE_TEST: {
                                Log.i("SlaveManager",
                                      "impedance data read "
                                      "start");
                            }
                        }

                        wait_for_data = false;
                        sync_timer.start();
                    }
                    if (wait_for_data == false) {
                        wait_for_data = true;
                        ctrl_processor.send_sync_frame();
                        sync_timer.start();
                    }
                }
            }
            TaskBase::delay(5);
        }
    }
};

#endif