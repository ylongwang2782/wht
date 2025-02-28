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
#include "chronolink.h"
#include "harness.h"
#include "mode_entry.h"

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

ChronoLink chronoLink;
Harness harness(2, 4);
class MyTimer {
   public:
    MyTimer()
        : myTimer("MyTimer", this, &MyTimer::myTimerCallback, pdMS_TO_TICKS(100),
                  pdTRUE) {}

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

    void task() override {
        for (;;) {
            // 等待 DMA 完成信号
            if (xSemaphoreTake(usart1_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                Log.d("Usart recv.");
                uint8_t buffer[DMA_RX_BUFFER_SIZE];
                uint16_t len =
                    usart1.getReceivedData(buffer, DMA_RX_BUFFER_SIZE);
                chronoLink.push_back(buffer, len);
                while (chronoLink.parseFrameFragment(frame_fragment)) {
                    Log.d("Get frame fragment.");
                    chronoLink.receiveAndAssembleFrame(frame_fragment,
                                                       frameSorting);
                };
            }
        }
    }

   private:
    ChronoLink::Fragment frame_fragment;
    static ChronoLink::DeviceConfig config;
    static void frameSorting(ChronoLink::CompleteFrame complete_frame) {
        extern MyTimer myTimer;
        std::vector<ChronoLink::DevConf> device_configs;
        ChronoLink::Instruction instruction;
        switch (complete_frame.type) {
            case ChronoLink::SYNC:
                Log.d("Frame: Sync");
                // harness.run();
                myTimer.startWithCount(4);
                break;
            case ChronoLink::COMMAND:
                Log.d("Frame: Instuction");

                instruction = chronoLink.parseInstruction(complete_frame.data);
                if (instruction.type == 0x00) {
                    config =
                        std::get<ChronoLink::DeviceConfig>(instruction.context);
                    Log.d("Instruction: Device Config");
                    Log.d("timeslot: %d", config.timeslot);
                    Log.d("totalHarnessNum: %d", config.totalHarnessNum);
                    Log.d("startHarnessNum: %d", config.startHarnessNum);
                    Log.d("harnessNum: %d", config.harnessNum);
                    Log.d("clipNum: %d", config.clipNum);

                    ChronoLink::DeviceConfig deviceConfig = {
                        .timeslot = 0,
                        .totalHarnessNum = 0,
                        .startHarnessNum = 0,
                        .harnessNum = 0,
                        .clipNum = 0,
                        .resNum = {0}};

                    chronoLink.sendReply(config.timeslot, ChronoLink::REPLY,
                                         ChronoLink::DEV_CONF, ChronoLink::OK,
                                         deviceConfig);

                } else if (instruction.type == 0x01) {
                    Log.d("Instruction: Data Request");

                    ChronoLink::DataReplyContext dataReply = {
                        .deviceStatus = {.colorSensor = 1,
                                         .sleeveLimit = 1,
                                         .electromagnetUnlockButton = 1,
                                         .batteryLowPowerAlarm = 1,
                                         .pressureSensor = 1,
                                         .electromagneticLock1 = 1,
                                         .electromagneticLock2 = 1,
                                         .accessory1 = 1,
                                         .accessory2 = 1,
                                         .reserved = 1},
                        .harnessLength = harness.data.getSize(),
                        .harnessData = harness.data.flatten(),
                        .clipLength = 1,
                        .clipData = {0x30}};

                    chronoLink.sendReply(config.timeslot, ChronoLink::REPLY,
                                         ChronoLink::DATA_REQ, ChronoLink::OK,
                                         dataReply);

                } else if (instruction.type == 0x02) {
                    Log.d("Instruction: Device Unlock");
                    const ChronoLink::DeviceUnlock &unlock =
                        std::get<ChronoLink::DeviceUnlock>(instruction.context);
                    Log.d("unlock: %d", unlock.lockStatus);

                    ChronoLink::DeviceUnlock unlockStatus = {.lockStatus = 0};

                    chronoLink.sendReply(config.timeslot, ChronoLink::REPLY,
                                         ChronoLink::DEV_UNLOCK, ChronoLink::OK,
                                         unlockStatus);
                } else {
                }
                break;
            default:
                break;
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

        for (;;) {
            led.toggle();
            TaskBase::delay(500);
        }
    }
};

UsartDMATask usartDMATask;
LedBlinkTask ledBlinkTask;
LogTask logTask;
MyTimer myTimer;

int Slave_Init(void) {
    UIDReader &uid = UIDReader::getInstance();
    harness.init();

    return 0;
}