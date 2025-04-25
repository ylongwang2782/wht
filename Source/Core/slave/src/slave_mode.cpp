
#include "slave_mode.hpp"
#ifdef SLAVE

class UsartDMATask : public TaskClassS<1024> {
   public:
    UsartDMATask() : TaskClassS<1024>("UsartDMATask", TaskPrio_High) {}
    FrameParser parser;
    void task() override {
        std::vector<uint8_t> rx_data;
        std::vector<uint8_t> uci_data;

        for (;;) {
            // 等待 DMA 完成信号
            if (xSemaphoreTake(uart3_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                // Log.d("Uart: recv.");
                sysLed.toggle();
                // rx_data = uart3.getReceivedData();
                auto msg = parser.parse(rx_data);
                if (msg != nullptr) {
                    msg->process();
                } else {
                    Log.d("Uart: parse fail.");
                }
            }
        }
    }
};

class LedBlinkTask : public TaskClassS<256> {
   public:
    LedBlinkTask() : TaskClassS<256>("LedBlinkTask", TaskPrio_Low) {}

    void task() override {
        // battery test
        Battery battery;
        battery.init();

        for (;;) {
            // battery.read();
            // Log.d("Battery: %d", battery.value);
            TaskBase::delay(500);
        }
    }
};

ManagerDataTransferMsg manager_transfer_msg;
MsgProc msgProc(manager_transfer_msg);

class MsgProcTask : public TaskClassS<MsgProcTask_SIZE> {
   public:
    MsgProcTask() : TaskClassS<MsgProcTask_SIZE>("MsgProcTask", MsgProcTask_PRIORITY) {}

    void task() override {
        Log.d("MsgProcTask: Boot");
        for (;;) {
            msgProc.proc();
            TaskBase::delay(1);
        }
    }
};

static void Slave_Task(void* pvParameters) {
    uint32_t myUid = UIDReader::get();
    Log.d("Slave Boot: %02X", myUid);

    LogTask logTask;
    logTask.give();

    ManagerDataTransferTask manageDataTransferTask(manager_transfer_msg);
    MsgProcTask msgProcTask;

    manageDataTransferTask.give();
    msgProcTask.give();

    while (1) {
        // Log.d("heap minimum: %d", xPortGetMinimumEverFreeHeapSize());
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int Slave_Init(void) {
    xTaskCreate(Slave_Task, "MasterTask", 4 * 1024, NULL, 2, NULL);

    return 0;
}

#endif