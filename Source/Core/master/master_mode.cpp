// master_mode.c
#include <cstdint>
#include <cstdio>

#include "FreeRTOS.h"
#include "bsp_led.hpp"
#include "bsp_spi.hpp"
#include "pc_interface.hpp"
#include "protocol.hpp"
#include "slave_manager.hpp"
#include "task.h"

#ifdef MASTER

UasrtInfo& pc_com_info = usart1_info;
UasrtInfo& slave_com_info = usart0_info;
UasrtInfo& log_com_info = uart7_info;

UartConfig log_com_cfg(log_com_info, false);
Uart log_com(log_com_cfg);
Logger Log(log_com);
// UartConfig uart3Conf(uart3_info);

bool __ProcessBase::rsp_parsed = false;    // 从机回复正确标志位

uint8_t __ProcessBase::expected_rsp_msg_id;    // 期望从机回复的消息ID

class __LogTask : public TaskClassS<1024> {
   public:
    __LogTask() : TaskClassS<1024>("LogTask", TaskPrio_Low) {}

    void task() override {
        char buffer[LOG_QUEUE_SIZE + 8];
        for (;;) {
            LogMessage logMsg;
            // 从队列中获取日志消息
            if (Log.logQueue.pop(logMsg, portMAX_DELAY)) {
                Log.uart.send(
                    reinterpret_cast<const uint8_t*>(logMsg.message.data()),
                    strlen(logMsg.message.data()));
            }
        }
    }
};

static void Master_Task(void* pvParameters) {
    LED led(GPIO::Port::A, GPIO::Pin::PIN_0);
    __LogTask logTask;
    logTask.give();
    // printf("Master_Task: Boot\n");
    Log.i("Master_Task: Boot");

    Log.i("[Master_Task]: Cond packet:");
    // Backend2Master::SlaveCfgMsg slave_cfg_msg;
    // Backend2Master::SlaveCfgMsg::SlaveConfig cfg;
    // cfg.id = 0x12345678;
    // cfg.clipMode = 0x01;
    // cfg.clipStatus = 0x00;
    // cfg.conductionNum = 16;
    // cfg.resistanceNum = 0;
    // slave_cfg_msg.slaves.push_back(cfg);

    // cfg.id = 0x11ABCDEF;
    // cfg.clipMode = 0x00;
    // cfg.clipStatus = 0x00;
    // cfg.conductionNum = 20;
    // cfg.resistanceNum = 0;
    // slave_cfg_msg.slaves.push_back(cfg);
    // slave_cfg_msg.slaveNum = 2;
    // auto msg = PacketPacker::backend2MasterPack(slave_cfg_msg);
    // std::vector<uint8_t> data = FramePacker::pack(msg);
    // Log.r(data.data(), data.size());

    // Log.i("[Master_Task]: Mode packet:");
    // Backend2Master::ModeCfgMsg mode_msg;
    // mode_msg.mode = 0x00;
    // msg = PacketPacker::backend2MasterPack(mode_msg);
    // data = FramePacker::pack(msg);
    // Log.r(data.data(), data.size());

    // Log.i("[Master_Task]: Control start packet:");
    // Backend2Master::CtrlMsg ctrl_msg;
    // ctrl_msg.runningStatus = 0x01;
    // msg = PacketPacker::backend2MasterPack(ctrl_msg);
    // data = FramePacker::pack(msg);
    // Log.r(data.data(), data.size());

    // Log.i("[Master_Task]: Control stop packet:");
    // ctrl_msg.runningStatus = 0x00;
    // msg = PacketPacker::backend2MasterPack(ctrl_msg);
    // data = FramePacker::pack(msg);
    // Log.r(data.data(), data.size());

    // 上位机数据传输任务 json解析任务 初始化
    PCdataTransferMsg pc_data_transfer_msg;
    PCmanagerMsg pc_manger_msg(pc_data_transfer_msg.tx_share_mem,
                               pc_data_transfer_msg.tx_request_sem,
                               pc_data_transfer_msg.tx_done_sem);

    PCinterface pc_interface(pc_manger_msg, pc_data_transfer_msg);
    PCdataTransfer pc_data_transfer(pc_data_transfer_msg);

    // 从机数据传输任务 从机管理任务 初始化
    ManagerDataTransferMsg manager_transfer_msg;
    SlaveManager slave_manager(pc_manger_msg, manager_transfer_msg);
    ManagerDataTransfer manager_data_transfer(manager_transfer_msg);

    pc_interface.give();
    pc_data_transfer.give();
    slave_manager.give();
    manager_data_transfer.give();

    DataForward tmp;
    while (1) {
        Log.d("heap minimum: %d", xPortGetMinimumEverFreeHeapSize());
        led.toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int Master_Init(void) {
    // 创建主节点任务
    xTaskCreate(Master_Task, "MasterTask", 4 * 1024, NULL, 2, NULL);
    return 0;
}
namespace Master2Slave {
void SyncMsg::process() { __ProcessBase::rsp_parsed = true; }
void CondCfgMsg::process() {}
void ClipCfgMsg::process() {}
void ResCfgMsg::process() {}
void ReadCondDataMsg::process() {}
void PingReqMsg::process() { Log.i("PingReqMsg"); }
}    // namespace Master2Slave

namespace Slave2Master {
using WriteCondInfoMsg = Master2Slave::CondCfgMsg;
using WriteClipInfoMsg = Master2Slave::ClipCfgMsg;
using WriteResInfoMsg = Master2Slave::ResCfgMsg;

void CondCfgMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        (uint8_t)(Slave2MasterMessageID::COND_CFG_MSG)) {
        Log.e("CondCfgMsg: msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
    if (CondCfgMsg::timeSlot != WriteCondInfoMsg::timeSlot) {
        Log.e("CondCfgMsg: timeSlot not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondCfgMsg::interval != WriteCondInfoMsg::interval) {
        Log.e("CondCfgMsg: interval not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondCfgMsg::totalConductionNum !=
        WriteCondInfoMsg::totalConductionNum) {
        Log.e("CondCfgMsg: totalConductionNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondCfgMsg::startConductionNum !=
        WriteCondInfoMsg::startConductionNum) {
        Log.e("CondCfgMsg: startConductionNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondCfgMsg::conductionNum != WriteCondInfoMsg::conductionNum) {
        Log.e("CondCfgMsg: conductionNum not match");
        __ProcessBase::rsp_parsed = false;
    }
}
void ClipCfgMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        (uint8_t)(Slave2MasterMessageID::CLIP_CFG_MSG)) {
        Log.e("ClipCfgMsg: msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
    if (ClipCfgMsg::mode != WriteClipInfoMsg::mode) {
        Log.e("ClipCfgMsg: mode not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ClipCfgMsg::clipPin != WriteClipInfoMsg::clipPin) {
        Log.e("ClipCfgMsg: clipPin not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ClipCfgMsg::interval != WriteClipInfoMsg::interval) {
        Log.e("ClipCfgMsg: clipNum not match");
        __ProcessBase::rsp_parsed = false;
    }
}
void ResCfgMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        (uint8_t)(Slave2MasterMessageID::RES_CFG_MSG)) {
        Log.e("ResCfgMsg: msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
    if (ResCfgMsg::timeSlot != WriteResInfoMsg::timeSlot) {
        Log.e("ResCfgMsg: timeSlot not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResCfgMsg::interval != WriteResInfoMsg::interval) {
        Log.e("ResCfgMsg: interval not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResCfgMsg::totalResistanceNum != WriteResInfoMsg::totalResistanceNum) {
        Log.e("ResCfgMsg: totalResistanceNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResCfgMsg::startResistanceNum != WriteResInfoMsg::startResistanceNum) {
        Log.e("ResCfgMsg: startResistanceNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResCfgMsg::resistanceNum != WriteResInfoMsg::resistanceNum) {
        Log.e("ResCfgMsg: resistanceNum not match");
        __ProcessBase::rsp_parsed = false;
    }
}

void PingRspMsg::process() { Log.i("PingRspMsg process"); }
 
}    // namespace Slave2Master

namespace Slave2Backend {
void CondDataMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        (uint8_t)(Slave2BackendMessageID::COND_DATA_MSG)) {
        Log.e("CondDataMsg: msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
}
}    // namespace Slave2Backend

#endif