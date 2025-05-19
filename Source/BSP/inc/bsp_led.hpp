#pragma once
#include "bsp_gpio.hpp"
#include "TaskCPP.h"
class LED {
   public:
    LED(GPIO::Port port, GPIO::Pin pin) : gpio(port, pin, GPIO::Mode::OUTPUT) {}

    void on() { gpio.bit_reset(); }
    void off() { gpio.bit_set(); }
    void toggle() { gpio.toggle(); }

   private:
    GPIO gpio;
};

class LedBlinkTask : public TaskClassS<128> {
   private:
    LED &led;
    uint32_t blink_interval;

   public:
    LedBlinkTask(LED &led, uint32_t interval)
        : TaskClassS<128>("LedBlinkTask", TaskPrio_Mid),
          led(led),
          blink_interval(interval) {}

    void task() override {
        for (;;) {
            led.toggle();
            TaskBase::delay(blink_interval);    // 使用传入的闪烁间隔
        }
    }
};
