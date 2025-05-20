
#include "slave_mode.hpp"
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

static void Slave_Task(void* pvParameters) {
    static constexpr const char TAG[] = "BOOT";
    Log.d(TAG,"Slave Firmware %s, Build: %s %s", FIRMWARE_VERSION, __DATE__, __TIME__);
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

#endif