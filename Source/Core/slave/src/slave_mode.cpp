
#include "slave_mode.hpp"
#ifdef SLAVE

#define SYS_LED_PORT GPIO::Port::C
#define SYS_LED_PIN  GPIO::Pin::PIN_13

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
                    Log.d("UART", "parse fail.");
                }
            }
        }
    }
};

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
    Log.v("BOOT", "Slave_Task start");
    uint32_t myUid = UIDReader::get();
    Log.v("BOOT", "UID: %08X", myUid);

    LogTask logTask(Log);
    logTask.give();
    Log.v("BOOT", "LogTask initialized");

    LED led0(SYS_LED_PORT, SYS_LED_PIN);
    LedBlinkTask ledBlinkTask(led0, 500);
    ledBlinkTask.give();
    Log.v("BOOT", "LedBlinkTask initialized");

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