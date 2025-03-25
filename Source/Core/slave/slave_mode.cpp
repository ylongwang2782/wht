#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "FreeRTOS.h"
#include "TaskCPP.h"
#include "TimerCPP.h"
#include "bsp_gpio.hpp"
#include "bsp_led.hpp"
#include "bsp_log.hpp"
#include "bsp_uid.hpp"
#include "harness.h"
#include "mode_entry.h"
#include "protocol.hpp"

#ifdef __cplusplus
extern "C" {
#endif
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"
#ifdef __cplusplus
}
#endif

UartConfig usart1Conf(usart1_info);
UartConfig usart2Conf(usart2_info);
UartConfig uart3Conf(uart3_info);
Uart usart1(usart1Conf);
Uart usart2(usart2Conf);
Uart uart3(uart3Conf);

Logger Log(usart1);

Harness harness(2, 4);
class MyTimer {
   public:
    MyTimer()
        : myTimer("MyTimer", this, &MyTimer::myTimerCallback,
                  pdMS_TO_TICKS(100), pdTRUE) {}

    void startWithCount(int count) {
        if (count > 0) {
            triggerCount = 0;
            maxTriggerCount = count;
            myTimer.start();
        }
    }

    void myTimerCallback() {
        // printf("Timer triggered!\n");
        if (maxTriggerCount > 0 && triggerCount++ >= maxTriggerCount) {
            myTimer.stop();
            harness.reload();
            return;
        }
        harness.run();
        harness.rowIndex++;
    }

   private:
    FreeRTOScpp::TimerMember<MyTimer> myTimer;
    int triggerCount;       // 当前触发次数
    int maxTriggerCount;    // 最大触发次数
};

class UsartDMATask : public TaskClassS<1024> {
   public:
    UsartDMATask() : TaskClassS<1024>("UsartDMATask", TaskPrio_High) {}
    FrameParser parser;
    void task() override {
        for (;;) {
            // 等待 DMA 完成信号
            if (xSemaphoreTake(usart1_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                Log.d("Uart: recv.");
                uint8_t buffer[DMA_RX_BUFFER_SIZE];
                uint16_t len =
                    usart1.getReceivedData(buffer, DMA_RX_BUFFER_SIZE);

                // 将 buffer 转换为 vector
                std::vector<uint8_t> raw_data(buffer, buffer + len);
                auto msg = parser.parse(raw_data);
                if (msg != nullptr) {
                    msg->process();
                } else {
                    Log.d("Uart: parse fail.");
                }
            }
        }
    }
};

class LogTask : public TaskClassS<1024> {
   public:
    LogTask() : TaskClassS<1024>("LogTask", TaskPrio_Mid) {}

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

class LedBlinkTask : public TaskClassS<256> {
   public:
    LedBlinkTask() : TaskClassS<256>("LedBlinkTask", TaskPrio_Low) {}

    void task() override {
        LED led(GPIO::Port::C, GPIO::Pin::PIN_6);

        SyncMsg syncMsg;
        syncMsg.mode = 0;
        syncMsg.timestamp = 0x12345678;

        uint32_t target_id = 0x3732485B;

        // 2. 打包为 Packet
        auto master_packet = PacketPacker::masterPack(syncMsg, target_id);
        auto slave_packet = PacketPacker::slavePack(syncMsg, target_id);

        // 3. 打包为帧
        auto master_data = FramePacker::pack(master_packet);
        auto slave_data = FramePacker::pack(slave_packet);

        for (;;) {
            // usart1.send(master_data.data(), master_data.size());
            // usart1.send(slave_data.data(), slave_data.size());

            led.toggle();
            TaskBase::delay(500);
        }
    }
};

#ifdef SLAVE
UsartDMATask usartDMATask;
LedBlinkTask ledBlinkTask;
MyTimer myTimer;
#endif
LogTask logTask;


int Slave_Init(void) {
    UIDReader &uid = UIDReader::getInstance();
    harness.init();

    return 0;
}