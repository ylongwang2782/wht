#pragma once
#include "bsp_gpio.hpp"
#include "bsp_led.hpp"
#include "bsp_log.hpp"
#include "bsp_uart.hpp"

extern LED sysLed;
extern Uart rs232;
extern Uart uart6;
extern Logger Log;
extern Rs485 rs485;

class LogTask : public TaskClassS<LogTask_SIZE> {
   public:
    LogTask() : TaskClassS<LogTask_SIZE>("LogTask", LogTask_PRIORITY) {}

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

const std::pair<GPIO::Port, GPIO::Pin> condPinInfo[] = {
    // PA3 - PA7
    {GPIO::Port::A, GPIO::Pin::PIN_3},    // PA3
    {GPIO::Port::A, GPIO::Pin::PIN_4},    // PA4
    {GPIO::Port::A, GPIO::Pin::PIN_5},    // PA5
    {GPIO::Port::A, GPIO::Pin::PIN_6},    // PA6
    {GPIO::Port::A, GPIO::Pin::PIN_7},    // PA7
    // PC4 - PC5
    {GPIO::Port::C, GPIO::Pin::PIN_4},    // PC4
    {GPIO::Port::C, GPIO::Pin::PIN_5},    // PC5
    // PB0 1
    {GPIO::Port::B, GPIO::Pin::PIN_0},    // PB0
    {GPIO::Port::B, GPIO::Pin::PIN_1},    // PB1
    // PF11 - PF15
    {GPIO::Port::F, GPIO::Pin::PIN_11},    // PF11
    {GPIO::Port::F, GPIO::Pin::PIN_12},    // PF12
    {GPIO::Port::F, GPIO::Pin::PIN_13},    // PF13
    {GPIO::Port::F, GPIO::Pin::PIN_14},    // PF14
    {GPIO::Port::F, GPIO::Pin::PIN_15},    // PF15
    // PG0 - PG1
    {GPIO::Port::G, GPIO::Pin::PIN_0},    // PG0
    {GPIO::Port::G, GPIO::Pin::PIN_1},    // PG1
    // PE7 - PE15
    {GPIO::Port::E, GPIO::Pin::PIN_7},     // PE7
    {GPIO::Port::E, GPIO::Pin::PIN_8},     // PE8
    {GPIO::Port::E, GPIO::Pin::PIN_9},     // PE9
    {GPIO::Port::E, GPIO::Pin::PIN_10},    // PE10
    {GPIO::Port::E, GPIO::Pin::PIN_11},    // PE11
    {GPIO::Port::E, GPIO::Pin::PIN_12},    // PE12
    {GPIO::Port::E, GPIO::Pin::PIN_13},    // PE13
    {GPIO::Port::E, GPIO::Pin::PIN_14},    // PE14
    {GPIO::Port::E, GPIO::Pin::PIN_15},    // PE15
    // PB10 - PB15
    {GPIO::Port::B, GPIO::Pin::PIN_10},    // PB10
    {GPIO::Port::B, GPIO::Pin::PIN_11},    // PB11
    {GPIO::Port::B, GPIO::Pin::PIN_12},    // PB12
    {GPIO::Port::B, GPIO::Pin::PIN_13},    // PB13
    {GPIO::Port::B, GPIO::Pin::PIN_14},    // PB14
    {GPIO::Port::B, GPIO::Pin::PIN_15},    // PB15
    // PD8 - PD15
    {GPIO::Port::D, GPIO::Pin::PIN_8},     // PD8
    {GPIO::Port::D, GPIO::Pin::PIN_9},     // PD9
    {GPIO::Port::D, GPIO::Pin::PIN_10},    // PD10
    {GPIO::Port::D, GPIO::Pin::PIN_11},    // PD11
    {GPIO::Port::D, GPIO::Pin::PIN_12},    // PD12
    {GPIO::Port::D, GPIO::Pin::PIN_13},    // PD13
    {GPIO::Port::D, GPIO::Pin::PIN_14},    // PD14
    {GPIO::Port::D, GPIO::Pin::PIN_15},    // PD15
    // PG2 - PG8
    {GPIO::Port::G, GPIO::Pin::PIN_2},    // PG2
    {GPIO::Port::G, GPIO::Pin::PIN_3},    // PG3
    {GPIO::Port::G, GPIO::Pin::PIN_4},    // PG4
    {GPIO::Port::G, GPIO::Pin::PIN_5},    // PG5
    {GPIO::Port::G, GPIO::Pin::PIN_6},    // PG6
    {GPIO::Port::G, GPIO::Pin::PIN_7},    // PG7
    {GPIO::Port::G, GPIO::Pin::PIN_8},    // PG8
    // PC6 - PC9
    {GPIO::Port::C, GPIO::Pin::PIN_6},    // PC6
    {GPIO::Port::C, GPIO::Pin::PIN_7},    // PC7
    {GPIO::Port::C, GPIO::Pin::PIN_8},    // PC8
    {GPIO::Port::C, GPIO::Pin::PIN_9},    // PC9
    // PA8 - PA15
    {GPIO::Port::A, GPIO::Pin::PIN_8},     // PA8
    {GPIO::Port::A, GPIO::Pin::PIN_11},    // PA11
    {GPIO::Port::A, GPIO::Pin::PIN_12},    // PA12
    {GPIO::Port::A, GPIO::Pin::PIN_15},    // PA15
    // PC10 - PC12
    {GPIO::Port::C, GPIO::Pin::PIN_10},    // PC10
    {GPIO::Port::C, GPIO::Pin::PIN_11},    // PC11
    {GPIO::Port::C, GPIO::Pin::PIN_12},    // PC12
    //  PD0 - PD6
    {GPIO::Port::D, GPIO::Pin::PIN_0},    // PD0
    {GPIO::Port::D, GPIO::Pin::PIN_1},    // PD1
    {GPIO::Port::D, GPIO::Pin::PIN_2},    // PD2
    {GPIO::Port::D, GPIO::Pin::PIN_3},    // PD3
    {GPIO::Port::D, GPIO::Pin::PIN_4},    // PD4
    {GPIO::Port::D, GPIO::Pin::PIN_5},    // PD5
    {GPIO::Port::D, GPIO::Pin::PIN_6},    // PD6
};

class HarnessGpio {
   public:
    static constexpr size_t NumPins =
        sizeof(condPinInfo) / sizeof(condPinInfo[0]);

    static std::vector<GPIO> condGpioArray;

    void init() {
        condGpioArray.reserve(NumPins);

        for (const auto& info : condPinInfo) {
            condGpioArray.emplace_back(info.first, info.second,
                                       GPIO::Mode::OUTPUT);
        }
    }

    void deinit() { condGpioArray.clear(); }

    void reset() {
        for (auto& gpio : condGpioArray) {
            gpio.bit_reset();    // 每个都重置
        }
    }

    void set() {
        for (auto& gpio : condGpioArray) {
            gpio.bit_set();    // 每个都设置
        }
    }

    void toggle() {
        for (auto& gpio : condGpioArray) {
            gpio.toggle();    // 每个都设置
        }
    }
};

class Key {
   public:
    Key(GPIO::Port port, GPIO::Pin pin) : gpio(port, pin, GPIO::Mode::INPUT) {}
    bool isPressed() { return !gpio.input_bit_get(); }

   private:
    GPIO gpio;    // Add this member declaration
};