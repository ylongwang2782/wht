#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "CX310.hpp"
#include "FreeRTOS.h"
#include "QueueCPP.h"
#include "SemaphoreCPP.h"
#include "TaskCPP.h"
#include "battery.hpp"
#include "bsp_gpio.hpp"
#include "bsp_led.hpp"
#include "bsp_log.hpp"
#include "bsp_uid.hpp"
#include "mode_entry.h"
#include "msg_proc.hpp"
#include "protocol.hpp"
#include "uwb_interface.hpp"

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

#ifdef SLAVE

UartConfig usart0Conf(usart0_info);
// UartConfig usart1Conf(usart1_info);
// UartConfig usart2Conf(usart2_info);
UartConfig uart3Conf(uart3_info);
// UartConfig uart6Conf(uart6_info);

Uart usart0(usart0Conf);
// Uart usart1(usart1Conf);
// Uart usart2(usart2Conf);
Uart uart3(uart3Conf);
// Uart uart6(uart6Conf);

Logger Log(uart3);

LED sysLed(GPIO::Port::C, GPIO::Pin::PIN_13);

ManagerDataTransferMsg manager_transfer_msg;
MsgProc msgProc(manager_transfer_msg);
class MsgProcTask : public TaskClassS<1024> {
   public:
    MsgProcTask() : TaskClassS<1024>("MsgProcTask", TaskPrio_High) {}

    void task() override {
        Log.d("MsgProcTask: Boot");
        for (;;) {
            msgProc.proc();
            TaskBase::delay(5);
        }
    }
};

class UsartDMATask : public TaskClassS<1024> {
   public:
    UsartDMATask() : TaskClassS<1024>("UsartDMATask", TaskPrio_High) {}
    FrameParser parser;
    void task() override {
        std::vector<uint8_t> rx_data;
        std::vector<uint8_t> uci_data;

        for (;;) {
            // 等待 DMA 完成信号
            if (xSemaphoreTake(uart3_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                // Log.d("Uart: recv.");
                sysLed.toggle();
                rx_data = uart3.getReceivedData();
                auto msg = parser.parse(rx_data);
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
                    reinterpret_cast<const uint8_t*>(logMsg.message.data()),
                    strlen(logMsg.message.data()));
            }
        }
    }
};

class LedBlinkTask : public TaskClassS<256> {
   public:
    LedBlinkTask() : TaskClassS<256>("LedBlinkTask", TaskPrio_Low) {}

    void task() override {
        // battery test
        Battery battery;
        battery.init();

        for (;;) {
            // battery.read();
            // Log.d("Battery: %d", battery.value);
            sysLed.toggle();
            TaskBase::delay(500);
        }
    }
};

MsgProcTask msgProcTask;
ManagerDataTransfer manager_data_transfer(manager_transfer_msg);
// UsartDMATask usartDMATask;
LedBlinkTask ledBlinkTask;
LogTask logTask;

int Slave_Init(void) {
    UIDReader& uid = UIDReader::getInstance();
    Log.d("Slave_Init: %02X", uid.value);
    return 0;
}

#endif