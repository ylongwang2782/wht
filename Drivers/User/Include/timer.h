/**
 * @file led.h
 * @author wang.yunlong (wang.yunlong9@byd.com)
 * @brief
 * @version 0.1
 * @date 2024-10-22
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif

#include "gd32f4xx.h"

#ifdef __cplusplus
}
#endif
class Timer {
   public:
    Timer(uint32_t period_ms) { setup(period_ms); }
    // start the timer
    void start();
    // stop the timer
    void stop();
    // define a timer interrupt handler
    void handleInterrupt();
    // set user callback function
    void setCallback(void (*callback)());
    // static member function to handle timer interrupt
    static void timerISR();
    // register an instance of Timer class
    static void registerInstance(Timer* instance);

   private:
    void setup(uint32_t period_ms);
    void (*callback_)() = nullptr;
    static Timer* instance_;
};
