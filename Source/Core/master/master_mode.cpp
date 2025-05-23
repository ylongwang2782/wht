// master_mode.c
#include <cstdint>
#include <cstdio>

#include "FreeRTOS.h"
#include "bsp_led.hpp"
#include "bsp_spi.hpp"
#include "master_def.hpp"
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

bool __ProcessBase::rsp_parsed = false;    // 从机回复正确标志位

uint8_t __ProcessBase::expected_rsp_msg_id;    // 期望从机回复的消息ID

// 发送数据入队超时
#define MsgProc_TX_QUEUE_TIMEOUT 1000
// 发送超时
#define MsgProc_TX_TIMEOUT 1000

class SlaveUploadProc {
   public:
    SlaveUploadProc(SlaveUploadTransferMgr& __transfer_msg)
        : transfer_msg(__transfer_msg) {}

    static constexpr const char TAG[] = "SlaveUploadProc";
    std::vector<uint8_t> recv_data;

   public:
    SlaveUploadTransferMgr& transfer_msg;
    FrameParser frame_parser;

    void proc() {
        uint8_t data;
        while (transfer_msg.rx_data_queue.pop(data, 0)) {
            // 处理接收到的数据
            recv_data.push_back(data);
        }
        // check if recv_data is empty
        if (recv_data.size() != 0) {
            auto msg = frame_parser.parse(recv_data);
            if (msg != nullptr) {
                // 处理解析后的数据
                msg->process();
            } else {
                Log.e(TAG, "parse failed");
            }
            recv_data.clear();
        }
    }

    bool send(std::vector<uint8_t>& frame) {
        // 将发送数据写入队列
        for (auto it = frame.begin(); it != frame.end(); it++) {
            if (transfer_msg.tx_data_queue.add(*it, MsgProc_TX_QUEUE_TIMEOUT) ==
                false) {
                Log.e(TAG, "tx_data_queue.add failed");
                return false;
            }
        }

        // 请求数据发送
        transfer_msg.tx_request_sem.give();

        // 等待数据发送完成
        if (transfer_msg.tx_done_sem.take(MsgProc_TX_TIMEOUT) == false) {
            Log.e(TAG, "tx_done_sem.take failed, timeout");
            return false;
        }
        return true;
    }
};

#define ShortIdAssignTask_SIZE     1024
#define ShortIdAssignTask_PRIORITY TaskPrio_Mid
class ShortIdAssignTask : public TaskClassS<ShortIdAssignTask_SIZE> {
   public:
    ShortIdAssignTask()
        : TaskClassS<ShortIdAssignTask_SIZE>("ShortIdAssignTask",
                                             ShortIdAssignTask_PRIORITY) {}

    /*
    创建一个ShortIdAssign任务，用于从机简易组网
    从机上电后，每隔500ms发送一次Anounce消息，主机这边等待接受Anounce消息，接受到后回复Short
    ID Assign消息，shortId由主机动态分配
    */
    void task() override {
        for (;;) {
            /*
            通过检查Slave2Master::AnnounceMsg::versionMajor是否为0来判断是否收到了
            如果收到了则发送ShortIdAssign消息，并且存储从机上报的信息到DeviceList中

            */
            if (Slave2Master::AnnounceMsg::versionMajor != 0) {
                // 发送ShortIdAssign消息
                Master2Slave::ShortIdAssignMsg shortIdAssignMsg;
                shortIdAssignMsg.shortId = 1;
            }
            TaskBase::delay(100);
        }
    }

   private:
    static constexpr const char TAG[] = "ShortIdAssignTask";
    static constexpr const uint8_t ANOUNCE_MAX_COUNT = 5;
};

#define SlaveUploadProcTask_SIZE     1024
#define SlaveUploadProcTask_PRIORITY TaskPrio_HMI
#define SlaveUploadProcTask_INTERVAL 5

class SlaveUploadProcTask : public TaskClassS<SlaveUploadProcTask_SIZE> {
   public:
    SlaveUploadProcTask(SlaveUploadProc& slaveUploadProc)
        : TaskClassS<SlaveUploadProcTask_SIZE>("SlaveUploadProcTask",
                                               SlaveUploadProcTask_PRIORITY),
          slaveUploadProc(slaveUploadProc) {}

   private:
    SlaveUploadProc& slaveUploadProc;

    void task() override {
        for (;;) {
            slaveUploadProc.proc();
            TaskBase::delay(SlaveUploadProcTask_INTERVAL);
        }
    }
};

SlaveUploadTransferMgr slaveUploadTransferMgr;
SlaveUploadProc slaveUploadProc(slaveUploadTransferMgr);

static void Master_Task(void* pvParameters) {
    static constexpr const char TAG[] = "BOOT";
    LED led(GPIO::Port::A, GPIO::Pin::PIN_0);
    LogTask logTask(Log);
    logTask.give();
    Log.d(TAG, "LogTask initialized");

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

    Log.d(TAG, "Slave Firmware v%s, Build: %s %s", FIRMWARE_VERSION, __DATE__,
          __TIME__);

    EthDevice ethDevice;
    ethDevice.init();
    Log.d(TAG, "ethDevice initialized");

    // 上位机数据传输任务 json解析任务 初始化
    PCdataTransferMsg pc_data_transfer_msg;
    PCmanagerMsg pc_manger_msg(pc_data_transfer_msg.tx_share_mem,
                               pc_data_transfer_msg.tx_request_sem,
                               pc_data_transfer_msg.tx_done_sem);

    PCinterface pc_interface(pc_manger_msg, pc_data_transfer_msg);
    PCdataTransfer pc_data_transfer(pc_data_transfer_msg);

    // 从机数据传输任务 从机管理任务 初始化
    ManagerDataTransferMsg manager_transfer_msg;
    // 用于主机被动接受的数据传输管理类

    // 从机数据传输任务
    ManagerDataTransfer manager_data_transfer(manager_transfer_msg,
                                              slaveUploadTransferMgr);

    // 主机主动发送数据处理任务
    SlaveManager slave_manager(pc_manger_msg, manager_transfer_msg);

    // 从机自主上传数据处理任务
    SlaveUploadProcTask slaveUploadProcTask(slaveUploadProc);

    pc_interface.give();
    Log.d(TAG, "PCinterface initialized");
    pc_data_transfer.give();
    Log.d(TAG, "PCdataTransfer initialized");

    slave_manager.give();
    Log.d(TAG, "SlaveManager initialized");
    slaveUploadProcTask.give();
    Log.d(TAG, "SlaveUploadProcTask initialized");

    manager_data_transfer.give();
    Log.d(TAG, "ManagerDataTransfer initialized");

    DataForward tmp;
    while (1) {
        // Log.v("SYS", "heap minimum: %d", xPortGetMinimumEverFreeHeapSize());
        led.toggle();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int Master_Init(void) {
    // 创建主节点任务
    xTaskCreate(Master_Task, "MasterTask", 20 * 1024, NULL, 2, NULL);
    return 0;
}
namespace Master2Slave {
void SyncMsg::process() {}

void CondCfgMsg::process() {}

void ClipCfgMsg::process() {}

void ResCfgMsg::process() {}

void ReadCondDataMsg::process() {}

void PingReqMsg::process() {}

void ShortIdAssignMsg::process() {}

}    // namespace Master2Slave

namespace Slave2Master {
using WriteCondInfoMsg = Master2Slave::CondCfgMsg;
using WriteClipInfoMsg = Master2Slave::ClipCfgMsg;
using WriteResInfoMsg = Master2Slave::ResCfgMsg;

void CondCfgMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        (uint8_t)(Slave2MasterMessageID::COND_CFG_MSG)) {
        Log.e("CondCfgMsg", "msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
    if (CondCfgMsg::timeSlot != WriteCondInfoMsg::timeSlot) {
        Log.e("CondCfgMsg", "timeSlot not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondCfgMsg::interval != WriteCondInfoMsg::interval) {
        Log.e("CondCfgMsg", "interval not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondCfgMsg::totalConductionNum !=
        WriteCondInfoMsg::totalConductionNum) {
        Log.e("CondCfgMsg", "totalConductionNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondCfgMsg::startConductionNum !=
        WriteCondInfoMsg::startConductionNum) {
        Log.e("CondCfgMsg", "startConductionNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondCfgMsg::conductionNum != WriteCondInfoMsg::conductionNum) {
        Log.e("CondCfgMsg", "conductionNum not match");
        __ProcessBase::rsp_parsed = false;
    }
}
void ClipCfgMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        (uint8_t)(Slave2MasterMessageID::CLIP_CFG_MSG)) {
        Log.e("ClipCfgMsg", " msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
    if (ClipCfgMsg::mode != WriteClipInfoMsg::mode) {
        Log.e("ClipCfgMsg", " mode not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ClipCfgMsg::clipPin != WriteClipInfoMsg::clipPin) {
        Log.e("ClipCfgMsg", " clipPin not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ClipCfgMsg::interval != WriteClipInfoMsg::interval) {
        Log.e("ClipCfgMsg", " clipNum not match");
        __ProcessBase::rsp_parsed = false;
    }
}
void ResCfgMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        (uint8_t)(Slave2MasterMessageID::RES_CFG_MSG)) {
        Log.e("ResCfgMsg", "msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
    if (ResCfgMsg::timeSlot != WriteResInfoMsg::timeSlot) {
        Log.e("ResCfgMsg", "timeSlot not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResCfgMsg::interval != WriteResInfoMsg::interval) {
        Log.e("ResCfgMsg", "interval not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResCfgMsg::totalResistanceNum != WriteResInfoMsg::totalResistanceNum) {
        Log.e("ResCfgMsg", "totalResistanceNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResCfgMsg::startResistanceNum != WriteResInfoMsg::startResistanceNum) {
        Log.e("ResCfgMsg", "startResistanceNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResCfgMsg::resistanceNum != WriteResInfoMsg::resistanceNum) {
        Log.e("ResCfgMsg", "resistanceNum not match");
        __ProcessBase::rsp_parsed = false;
    }
}

void PingRspMsg::process() { Log.d("PingRspMsg", "process"); }

void AnnounceMsg::process() {
    static constexpr const char TAG[] = "AnnounceMsg";
    Log.d(TAG, "process");
    // 收到宣告消息后，将从机信息存储到DeviceListRspMsg::devices中，并为该从机分配shortId，然后发送ShortIdAssign消息
    // 从机信息存储到DeviceListRspMsg::devices中
    Master2Backend::DeviceListRspMsg::DeviceInfo device;
    device.id = AnnounceMsg::deviceId;
    device.shortId = 0;
    device.online = 1;
    device.versionMajor = AnnounceMsg::versionMajor;
    device.versionMinor = AnnounceMsg::versionMinor;
    device.versionPatch = AnnounceMsg::versionPatch;
    Log.v(TAG, "Device %x announced", device.id);

    // store to DeviceListRspMsg::devices
    Master2Backend::DeviceListRspMsg::devices.push_back(device);
    Log.v(TAG, "Device %x stored", device.id);

    // 发送ShortIdAssign消息
    Master2Slave::ShortIdAssignMsg shortIdAssignMsg;
    shortIdAssignMsg.shortId = 1;
    Log.v(TAG, "Send ShortIdAssignMsg to slave %x", device.id);

    auto shortIdAssignPacket =
        PacketPacker::master2SlavePack(shortIdAssignMsg, device.id);
    Log.v(TAG, "ShortIdAssignMsg packed");

    auto shortIdAssignFrame = FramePacker::pack(shortIdAssignPacket);
    Log.v(TAG, "ShortIdAssignMsg framed");

    slaveUploadProc.send(shortIdAssignFrame);
    Log.v(TAG, "Send ShortIdAssignMsg to slave %x", device.id);
}

void ShortIdConfirmMsg::process() { Log.d("ShortIdConfirmMsg", "process"); }

}    // namespace Slave2Master

namespace Slave2Backend {
void CondDataMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        (uint8_t)(Slave2BackendMessageID::COND_DATA_MSG)) {
        Log.e("CondDataMsg", "msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
}
}    // namespace Slave2Backend

#ifdef ARM
extern "C" {
int fputc(int ch, FILE* f) {
    usart_data_transmit(USART1, (uint8_t)ch);
    while (RESET == usart_flag_get(USART1, USART_FLAG_TBE)) {
    }
    return ch;
}
}
#endif

#ifdef GCC
extern "C" {
int _write(int fd, char* pBuffer, int size) {
    for (int i = 0; i < size; i++) {
        while (RESET == usart_flag_get(UART7, USART_FLAG_TBE));
        usart_data_transmit(UART7, (uint8_t)pBuffer[i]);
    }
    return size;
}
}
#endif

#endif