#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "CX310.hpp"
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

Harness harness;
LED sysLed(GPIO::Port::C, GPIO::Pin::PIN_13);
CX310Class CX310(uart3);

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
        std::vector<uint8_t> rx_data;
        std::vector<uint8_t> uci_data;

        for (;;) {
            // 等待 DMA 完成信号
            if (xSemaphoreTake(uart3_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                // Log.d("Uart: recv.");
                sysLed.toggle();

                rx_data = uart3.getReceivedData();

                // uci_data = CX310.parse_received_data(rx_data);
                // Log.d("Uart: uci_data.size(): %d", uci_data.size());

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
        // battery test
        Battery battery;
        battery.init();

        // CX310.setChannel(5);
        CX310.setRate(2);
        CX310.setRxMode();

        std::vector<uint8_t> tx_data = {0x01, 0x02, 0x03, 0x04, 0x05};

        for (;;) {
            battery.read();
            // Log.d("Battery: %d", battery.value);

            // CX310.startTransmit(tx_data);
            // sysLed.toggle();

            TaskBase::delay(500);
        }
    }
};

UsartDMATask usartDMATask;
LedBlinkTask ledBlinkTask;
MyTimer myTimer;
LogTask logTask;

void Master2Slave::SyncMsg::process() {
    Log.d("SyncMsg process");
    // myTimer.startWithCount(4);
}

void Master2Slave::CondCfgMsg::process() {
    Log.d("CondCfgMsg process");

    // // 1. REPLY
    // // 1.1 构造 CondCfgMsg
    // Slave2Master::CondCfgMsg condInfoMsg;
    // condInfoMsg.timeSlot = timeSlot;
    // condInfoMsg.interval = interval;
    // condInfoMsg.totalConductionNum = totalConductionNum;
    // condInfoMsg.startConductionNum = startConductionNum;
    // condInfoMsg.conductionNum = conductionNum;

    // // 初始化 Harness
    // harness.init(conductionNum, totalConductionNum, startConductionNum);

    // // 1.2 打包为 Packet
    // auto condInfoPacket = PacketPacker::slavePack(condInfoMsg, 0x3732485B);
    // // 1.3 打包为帧
    // auto condInfoFrame = FramePacker::pack(condInfoPacket);
    // // 1.4 发送
    // uart3.send(condInfoFrame.data(), condInfoFrame.size());
}

void Master2Slave::ResCfgMsg::process() { Log.d("ResCfgMsg process"); }
void Master2Slave::ClipCfgMsg::process() { Log.d("ClipCfgMsg process"); }
void Master2Slave::ReadCondDataMsg::process() {
    // Log.d("ReadCondDataMsg process");
    // Slave2Backend::CondDataMsg condDataMsg;
    // condDataMsg.conductionData = harness.data.flatten();
    // condDataMsg.conductionLength = condDataMsg.conductionData.size();

    // // 2. 打包为 Packet
    // auto condDataPacket = PacketPacker::slavePack(condDataMsg, 0x3732485B);

    // // 3. 打包为帧
    // auto master_data = FramePacker::pack(condDataPacket);

    // uart3.send(master_data.data(), master_data.size());
}
void Master2Slave::ReadResDataMsg::process() {
    Log.d("ReadResDataMsg process");
}
void Master2Slave::ReadClipDataMsg::process() {
    Log.d("ReadClipDataMsg process");
}
void Master2Slave::RstMsg::process() { Log.d("RstMsg process"); }

void Slave2Master::CondCfgMsg::process() { Log.d("CondCfgMsg process"); }
void Slave2Master::ResCfgMsg::process() { Log.d("ResCfgMsg process"); }
void Slave2Master::ClipCfgMsg::process() { Log.d("ClipCfgMsg process"); }
void Slave2Master::RstMsg::process() { Log.d("RstMsg process"); }

void Backend2Master::SlaveCfgMsg::process() { Log.d("SlaveCfgMsg process"); }
void Backend2Master::ModeCfgMsg::process() { Log.d("ModeCfgMsg process"); }
void Backend2Master::RstMsg::process() { Log.d("RstMsg process"); }
void Backend2Master::CtrlMsg::process() { Log.d("CtrlMsg process"); }

void Master2Backend::SlaveCfgMsg::process() { Log.d("SlaveCfgMsg process"); }
void Master2Backend::ModeCfgMsg::process() { Log.d("ModeCfgMsg process"); }
void Master2Backend::RstMsg::process() { Log.d("RstMsg process"); }
void Master2Backend::CtrlMsg::process() { Log.d("CtrlMsg process"); }

void Slave2Backend::CondDataMsg::process() { Log.d("CondDataMsg process"); }
void Slave2Backend::ResDataMsg::process() { Log.d("ResDataMsg process"); }
void Slave2Backend::ClipDataMsg::process() { Log.d("ClipDataMsg process"); }

int Slave_Init(void) {
    UIDReader &uid = UIDReader::getInstance();

    Log.d("Slave_Init: %02X", uid.value);

    return 0;
}

#endif