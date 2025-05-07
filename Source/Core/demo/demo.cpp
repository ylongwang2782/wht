
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

class KeyTestTask : public TaskClassS<1024> {
   public:
    KeyTestTask() : TaskClassS<1024>("KeyTestTask", TaskPrio_High) {}
    void task() override {
        // KEY1:PG10
        // KEY2:PG11
        // KEY3:PG13
        // KEY4:PG14
        // KEY5:PB2
        // KEY6:PB3
        Key key1(GPIO::Port::G, GPIO::Pin::PIN_10);
        Key key2(GPIO::Port::G, GPIO::Pin::PIN_11);
        Key key3(GPIO::Port::G, GPIO::Pin::PIN_13);
        Key key4(GPIO::Port::G, GPIO::Pin::PIN_14);
        Key key5(GPIO::Port::B, GPIO::Pin::PIN_3);
        Key key6(GPIO::Port::B, GPIO::Pin::PIN_4);

        for (;;) {
            if (key1.isPressed()) {
                Log.d("Key1 pressed");
            }
            if (key2.isPressed()) {
                Log.d("Key2 pressed");
            }
            if (key3.isPressed()) {
                Log.d("Key3 pressed");
            }
            if (key4.isPressed()) {
                Log.d("Key4 pressed");
            }
            if (key5.isPressed()) {
                Log.d("Key5 pressed");
            }
            if (key6.isPressed()) {
                Log.d("Key6 pressed");
            }
            TaskBase::delay(1000);
        }
    }
};

static void Demo_Task(void* pvParameters) {
    uint32_t myUid = UIDReader::get();
    Log.d("Slave Boot: %08X", myUid);

    LogTask logTask;
    logTask.give();
    Log.d("LogTask started");

    ComEchoTask ComEchoTask;
    ComEchoTask.give();
    Log.d("ComEchoTask started");

    GpioTestTask GpioTestTask;
    GpioTestTask.give();
    Log.d("GpioTestTask started");

    LedTestTask LedTestTask;
    LedTestTask.give();
    Log.d("LedTestTask started");

    KeyTestTask KeyTestTask;
    KeyTestTask.give();
    Log.d("KeyTestTask started");

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