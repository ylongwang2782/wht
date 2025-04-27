#pragma once
#include "bsp_led.hpp"
#include "bsp_log.hpp"

#define USART_LOG      USART1

extern LED sysLed;
extern Logger Log;
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