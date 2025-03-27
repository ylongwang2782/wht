#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "FreeRTOS.h"
#include "TaskCPP.h"
#include "TimerCPP.h"
#include "battery.hpp"
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

// UartConfig usart0Conf(usart0_info);
// UartConfig usart1Conf(usart1_info);
// UartConfig usart2Conf(usart2_info);
UartConfig uart3Conf(uart3_info);
// UartConfig uart6Conf(uart6_info);
// Uart usart0(usart0Conf);
// Uart usart1(usart1Conf);
// Uart usart2(usart2Conf);
Uart uart3(uart3Conf);
// Uart uart6(uart6Conf);

Logger Log(uart3);

Harness harness;
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
            if (xSemaphoreTake(uart3_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                Log.d("Uart: recv.");
                uint8_t buffer[DMA_RX_BUFFER_SIZE];
                uint16_t len =
                    uart3.getReceivedData(buffer, DMA_RX_BUFFER_SIZE);

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
        LED led(GPIO::Port::C, GPIO::Pin::PIN_13);
        // battery test

        Battery battery;
        battery.init();
        for (;;) {
            battery.read();
            Log.d("Battery: %d", battery.value);
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

void SyncMsg::process() {
    Log.d("SyncMsg process");
    myTimer.startWithCount(4);
}

void WriteCondInfoMsg::process() {
    Log.d("WriteCondInfoMsg process");

    // 1. REPLY
    // 1.1 构造 CondInfoMsg
    CondInfoMsg condInfoMsg;
    condInfoMsg.timeSlot = timeSlot;
    condInfoMsg.interval = interval;
    condInfoMsg.totalConductionNum = totalConductionNum;
    condInfoMsg.startConductionNum = startConductionNum;
    condInfoMsg.conductionNum = conductionNum;

    // 初始化 Harness
    harness.init(conductionNum, totalConductionNum, startConductionNum);

    // 1.2 打包为 Packet
    auto condInfoPacket = PacketPacker::slavePack(condInfoMsg, 0x3732485B);
    // 1.3 打包为帧
    auto condInfoFrame = FramePacker::pack(condInfoPacket);
    // 1.4 发送
    uart3.send(condInfoFrame.data(), condInfoFrame.size());
}

void WriteResInfoMsg::process() { Log.d("WriteResInfoMsg process"); }

void ReadCondDataMsg::process() {
    Log.d("ReadCondDataMsg process");
    CondDataMsg condDataMsg;
    condDataMsg.conductionData = harness.data.flatten();
    condDataMsg.conductionLength = condDataMsg.conductionData.size();
    condDataMsg.deviceStatus = DeviceStatus{
        1,    // colorSensor
        1,    // sleeveLimit
        1,    // electromagnetUnlockButton
        1,    // batteryLowPowerAlarm
        1,    // pressureSensor
        1,    // electromagneticLock1
        1,    // electromagneticLock2
        1,    // accessory1
        1,    // accessory2
        1     // reserved
    };

    // 2. 打包为 Packet
    auto condDataPacket = PacketPacker::slavePack(condDataMsg, 0x3732485B);

    // 3. 打包为帧
    auto master_data = FramePacker::pack(condDataPacket);

    uart3.send(master_data.data(), master_data.size());
}

int Slave_Init(void) {
    UIDReader &uid = UIDReader::getInstance();
    // print uid in hex format
    Log.d("Slave_Init: %02X", uid.value);

    // // enable cx310 recv mode with 23 01 00 00 using usart0
    // std::array<uint8_t, 4> cx310_recv_mode = {0x23, 0x01, 0x00, 0x00};
    // usart0.send(cx310_recv_mode.data(), cx310_recv_mode.size());

    return 0;
}