
#include "slave_mode.hpp"

#include <stdint.h>

#include "protocol.hpp"

#ifdef SLAVE

ManagerDataTransferMsg manager_transfer_msg;
MsgProc msgProc(manager_transfer_msg);

class MsgProcTask : public TaskClassS<MsgProcTask_SIZE> {
   public:
    MsgProcTask()
        : TaskClassS<MsgProcTask_SIZE>("MsgProcTask", MsgProcTask_PRIORITY) {}

    void task() override {
        for (;;) {
            msgProc.proc();
            TaskBase::delay(1);
        }
    }
};

#define AnounceTask_SIZE     1024
#define AnounceTask_PRIORITY TaskPrio_Mid

/*
创建一个Anounce任务，用于从机简易组网
从机上电后，每隔500ms发送一次Anounce消息，等待主机回复Short ID Assign消息
如果收到，则将其中的Short ID记录
如果没有收到，则继续发送Anounce消息，最多发送五次
收到Short ID Assign消息后，从机将向主机发送一个Short ID Confirm消息
*/
class AnounceTask : public TaskClassS<AnounceTask_SIZE> {
   public:
    AnounceTask()
        : TaskClassS<AnounceTask_SIZE>("AnounceTask", AnounceTask_PRIORITY) {}

    void task() override {
        uint32_t uid = UIDReader::get();
        Slave2Master::AnnounceMsg announceMsg;
        announceMsg.deviceId = uid;
        announceMsg.versionMajor = 3;
        announceMsg.versionMinor = 1;
        announceMsg.versionPatch = 0;

        auto annoucePacket = PacketPacker::slave2MasterPack(announceMsg, uid);
        // 1.3 打包为帧
        auto annouceFrame = FramePacker::pack(annoucePacket);

        for (;;) {
            // 没有被分配shortId，发送announce消息
            if (Slave2Master::ShortIdConfirmMsg::shortId == 0) {
                count++;
                Log.d(TAG, "Send Announce Message");
                msgProc.send(annouceFrame);
                if (count > ANOUNCE_MAX_COUNT) {
                    count = 0;
                    // 停止任务
                    this->suspend();
                }
                TaskBase::delay(500);
            }
            // 检查是否有收到shortIDAssign消息，收到后shortId将会被置位
            if (Slave2Master::ShortIdConfirmMsg::shortId != 0) {
                Log.d(TAG, "Get Short ID: %d");
                // 发送Short ID Confirm消息
                Slave2Master::ShortIdConfirmMsg shortIdConfirmMsg;
                shortIdConfirmMsg.status = 0;
                shortIdConfirmMsg.shortId =
                    Slave2Master::ShortIdConfirmMsg::shortId;

                auto shortIdConfirmPacket =
                    PacketPacker::slave2MasterPack(shortIdConfirmMsg, uid);
                auto shortIdConfirmFrame =
                    FramePacker::pack(shortIdConfirmPacket);
                msgProc.send(shortIdConfirmFrame);
                // 停止任务
                this->suspend();
            }
        }
    }

   private:
    static constexpr const char TAG[] = "AnounceTask";
    static constexpr const uint8_t ANOUNCE_MAX_COUNT = 5;
    uint8_t count = 0;
};

static void Slave_Task(void* pvParameters) {
    static constexpr const char TAG[] = "BOOT";
    Log.d(TAG, "Slave Firmware v%s, Build: %s %s", FIRMWARE_VERSION, __DATE__,
          __TIME__);
    uint32_t myUid = UIDReader::get();
    Log.d(TAG, "Slave UID: %08X", myUid);

    LogTask logTask(Log);
    logTask.give();
    Log.d(TAG, "LogTask initialized");

    ManagerDataTransferTask manageDataTransferTask(manager_transfer_msg);
    MsgProcTask msgProcTask;

    manageDataTransferTask.give();
    Log.d(TAG, "ManagerDataTransferTask initialized");
    msgProcTask.give();
    Log.d(TAG, "MsgProcTask initialized");

    AnounceTask anounceTask;
    anounceTask.give();
    Log.d(TAG, "AnounceTask initialized");

    // 系统初始化完成，打开电源指示灯
    pwrLed.on();

    while (1) {
        // Log.d("heap minimum: %d", xPortGetMinimumEverFreeHeapSize());
        sysLed.toggle();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int Slave_Init(void) {
    xTaskCreate(Slave_Task, "MasterTask", 4 * 1024, NULL, 2, NULL);

    return 0;
}

#ifdef ARM
extern "C" {
int fputc(int ch, FILE *f) {
    usart_data_transmit(USART1, (uint8_t)ch);
    while (RESET == usart_flag_get(USART1, USART_FLAG_TBE)) {
    }
    return ch;
}
}
#endif

#ifdef GCC
extern "C" {
int _write(int fd, char *pBuffer, int size) {
    for (int i = 0; i < size; i++) {
        while (RESET == usart_flag_get(UART3, USART_FLAG_TBE));
        usart_data_transmit(UART3, (uint8_t)pBuffer[i]);
    }
    return size;
}
}
#endif

#endif