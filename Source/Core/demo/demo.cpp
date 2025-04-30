
#include "demo.hpp"

#include "TaskCPP.h"
#include "peripherals.hpp"
#include "bsp_uid.hpp"

#ifdef DEMO

class Rs232Task : public TaskClassS<1024> {
   public:
    Rs232Task() : TaskClassS<1024>("Rs232Task", TaskPrio_High) {}
    void task() override {
        std::vector<uint8_t> rs232_data;
        std::vector<uint8_t> rs485_data;
        for (;;) {
            rs232_data = rs232.getReceivedData();
            rs485_data = rs485.getReceivedData();

            if (rs232_data.size() != 0) {
                // echo back
                rs232.data_send(rs232_data.data(), rs232_data.size());
                rs232_data.clear();
            }
            if (rs485_data.size() != 0) {
                // echo back
                rs485.data_send(rs485_data);
                rs485_data.clear();
            }
            TaskBase::delay(10);
        }
    }
};

static void Demo_Task(void* pvParameters) {
    uint32_t myUid = UIDReader::get();
    Log.d("Slave Boot: %08X", myUid);

    LogTask logTask;
    logTask.give();

    Rs232Task rs232Task;
    rs232Task.give();

    while (1) {
        // Log.d("heap minimum: %d", xPortGetMinimumEverFreeHeapSize());
        sysLed.toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int Slave_Init(void) {
    xTaskCreate(Demo_Task, "MasterTask", 4 * 1024, NULL, 2, NULL);

    return 0;
}

#endif