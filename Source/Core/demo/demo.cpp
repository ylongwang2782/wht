
#include "demo.hpp"

#include "TaskCPP.h"
#include "bsp_uid.hpp"
#include "peripherals.hpp"

#ifdef DEMO

class ComEchoTask : public TaskClassS<256> {
   public:
    ComEchoTask() : TaskClassS<256>("ComEchoTask", TaskPrio_High) {}
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

class GpioTestTask : public TaskClassS<256> {
   public:
    GpioTestTask() : TaskClassS<256>("GpioTestTask", TaskPrio_High) {}
    void task() override {
        HarnessGpio harnessGpio;
        harnessGpio.init();
        for (;;) {
            harnessGpio.toggle();
            TaskBase::delay(1000);
        }
    }
};

class LedElvTestTask : public TaskClassS<256> {
   public:
    LedElvTestTask() : TaskClassS<256>("LedElvTestTask", TaskPrio_High) {}
    void task() override {
        // LED1: PG9
        // LED2: PG12
        // LED3: PG15
        LED led1(GPIO::Port::G, GPIO::Pin::PIN_9);
        LED led2(GPIO::Port::G, GPIO::Pin::PIN_12);
        LED led3(GPIO::Port::G, GPIO::Pin::PIN_15);

        // ELV1: PE1
        // ELV2: PE0
        // ELV3: PB9
        // ELV4: PB8
        Elv elv1(GPIO::Port::E, GPIO::Pin::PIN_1);
        Elv elv2(GPIO::Port::E, GPIO::Pin::PIN_0);
        Elv elv3(GPIO::Port::B, GPIO::Pin::PIN_9);
        Elv elv4(GPIO::Port::B, GPIO::Pin::PIN_8);
        for (;;) {
            led1.toggle();
            led2.toggle();
            led3.toggle();

            elv1.toggle();
            elv2.toggle();
            elv3.toggle();
            elv4.toggle();
            TaskBase::delay(1000);
        }
    }
};

class KeyTestTask : public TaskClassS<256> {
   public:
    KeyTestTask() : TaskClassS<256>("KeyTestTask", TaskPrio_High) {}
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

        // Airpress: PB5
        Key airprse(GPIO::Port::B, GPIO::Pin::PIN_5);
        // ColorSensor: PD7
        Key colorSensor(GPIO::Port::D, GPIO::Pin::PIN_7);

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
            if (airprse.isPressed()) {
                Log.d("Airprse pressed");
            }
            if (colorSensor.isPressed()) {
                Log.d("ColorSensor pressed");
            }
            TaskBase::delay(1000);
        }
    }
};

class DipSwithTestTask : public TaskClassS<256> {
   public:
    DipSwithTestTask() : TaskClassS<256>("DipSwithTestTask", TaskPrio_High) {}
    void task() override {
    // SW1 = PC3
    // SW2 = PC2
    // SW3 = PC1
    // SW4 = PC0
    // SW5 = PF10
    // SW6 = PF9
    // SW7 = PF8
    // SW8 = PF5
    DipSwitchInfo info = {
            .pins = {
               {GPIO::Port::C, GPIO::Pin::PIN_3},    // PC3
               {GPIO::Port::C, GPIO::Pin::PIN_2},    // PC2
               {GPIO::Port::C, GPIO::Pin::PIN_1},    // PC1
               {GPIO::Port::C, GPIO::Pin::PIN_0},    // PC0
               {GPIO::Port::F, GPIO::Pin::PIN_10},   // PF10
               {GPIO::Port::F, GPIO::Pin::PIN_9},    // PF9
               {GPIO::Port::F, GPIO::Pin::PIN_8},    // PF8
               {GPIO::Port::F, GPIO::Pin::PIN_5},    // PF5
            }
        };
        DipSwitch dip(info);
        for (;;) {
            uint8_t switchVal = dip.value();  // 获取当前拨码值
            Log.d("DipSwitch value: %02X", switchVal);
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

    LedElvTestTask LedElvTestTask;
    LedElvTestTask.give();
    Log.d("LedElvTestTask started");

    KeyTestTask KeyTestTask;
    KeyTestTask.give();
    Log.d("KeyTestTask started");

    DipSwithTestTask DipSwithTestTask;
    DipSwithTestTask.give();
    Log.d("DipSwithTestTask started");

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