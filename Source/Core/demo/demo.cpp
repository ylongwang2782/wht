
#include "demo.hpp"

#include "TaskCPP.h"
#include "bsp_uid.hpp"
#include "peripherals.hpp"

#ifdef DEMO

class ComEchoTask : public TaskClassS<1024> {
   public:
    ComEchoTask() : TaskClassS<1024>("ComEchoTask", TaskPrio_High) {}
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

class GpioTestTask : public TaskClassS<1024> {
   public:
    GpioTestTask() : TaskClassS<1024>("GpioTestTask", TaskPrio_High) {}
    void task() override {
        HarnessGpio harnessGpio;
        harnessGpio.init();
        for (;;) {
            harnessGpio.toggle();
            TaskBase::delay(1000);
        }
    }
};

class LedTestTask : public TaskClassS<1024> {
   public:
    LedTestTask() : TaskClassS<1024>("LedTestTask", TaskPrio_High) {}
    void task() override {
        // LED1:PG9
        // LED2:PG12
        // LED3:PG15
        LED led1(GPIO::Port::G, GPIO::Pin::PIN_9);
        LED led2(GPIO::Port::G, GPIO::Pin::PIN_12);
        LED led3(GPIO::Port::G, GPIO::Pin::PIN_15);
        for (;;) {
            led1.toggle();
            led2.toggle();
            led3.toggle();
            TaskBase::delay(1000);
        }
    }
};

static void Demo_Task(void* pvParameters) {
    uint32_t myUid = UIDReader::get();
    Log.d("Slave Boot: %08X", myUid);

    LogTask logTask;
    logTask.give();

    ComEchoTask ComEchoTask;
    ComEchoTask.give();

    GpioTestTask GpioTestTask;
    GpioTestTask.give();

    LedTestTask LedTestTask;
    LedTestTask.give();

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