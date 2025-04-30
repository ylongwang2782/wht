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
                    reinterpret_cast<const uint8_t *>(logMsg.message.data()),
                    strlen(logMsg.message.data()));
            }
        }
    }
};
