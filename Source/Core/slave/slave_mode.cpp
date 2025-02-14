#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "FreeRTOS.h"
#include "TaskCPP.h"
#include "TimerCPP.h"
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

extern UasrtInfo usart1_info;
UartConfig uart1Conf(usart1_info);
Uart uart1(uart1Conf);

ChronoLink chronoLink;
extern Harness harness;

class UsartDMATask : public TaskClassS<1024> {
   public:
    UsartDMATask() : TaskClassS<1024>("UsartDMATask", TaskPrio_High) {}

    void task() override {
        Logger &log = Logger::getInstance();
        for (;;) {
            // 等待 DMA 完成信号
            if (xSemaphoreTake(usart1_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                Log.d("Usart recv.");
                uint8_t buffer[DMA_RX_BUFFER_SIZE];
                uint16_t len =
                    uart1.getReceivedData(buffer, DMA_RX_BUFFER_SIZE);
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
    static void frameSorting(ChronoLink::CompleteFrame complete_frame) {
        std::vector<ChronoLink::DevConf> device_configs;
        ChronoLink::Instruction instruction;
        switch (complete_frame.type) {
            case ChronoLink::SYNC:
                Log.d("Frame: Sync");
                break;
            case ChronoLink::COMMAND:
                Log.d("Frame: Instuction");

                instruction = chronoLink.parseInstruction(complete_frame.data);
                if (instruction.type == 0x00) {
                    const ChronoLink::DeviceConfig &config =
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
                        .deviceStatus = {0x0001, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                        .harnessLength = 2,
                        .harnessData = {0x10, 0x20},
                        .clipLength = 1,
                        .clipData = {0x30}};

                    chronoLink.sendReply(2, ChronoLink::REPLY,
                                         ChronoLink::DATA_REQ, ChronoLink::OK,
                                         dataReply);

                } else if (instruction.type == 0x02) {
                    Log.d("Instruction: Device Unlock");
                    const ChronoLink::DeviceUnlock &unlock =
                        std::get<ChronoLink::DeviceUnlock>(instruction.context);
                    Log.d("unlock: %d", unlock.lockStatus);

                    ChronoLink::DeviceUnlock unlockStatus = {.lockStatus = 0};

                    chronoLink.sendReply(2, ChronoLink::REPLY,
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

class MyTimer {
   public:
   MyTimer()
        : myTimer("MyTimer", this, &MyTimer::myTimerCallback,
                  pdMS_TO_TICKS(1000), pdTRUE) {
        // pdMS_TO_TICKS(1000) 表示定时器周期为 1000 毫秒
        // pdTRUE 表示定时器是自动重载的
        myTimer.start();    // 启动定时器
    }

    void myTimerCallback() {
        // 这里是你希望在定时器触发时执行的代码
        printf("Timer triggered!\n");
    }

   private:
    FreeRTOScpp::TimerMember<MyTimer> myTimer;
};

class LogTask : public TaskClassS<1024> {
    public:
     LogTask() : TaskClassS<1024>("LogTask", TaskPrio_Low) {}
 
     void task() override {
         char buffer[LOG_QUEUE_SIZE + 8];
         Logger &log = Logger::getInstance();
         for (;;) {
             if (xQueueReceive(log.logQueue, buffer, portMAX_DELAY)) {
                 for (const char *p = buffer; *p; ++p) {
                     while (RESET == usart_flag_get(USART_LOG, USART_FLAG_TBE));
                     usart_data_transmit(USART_LOG, (uint8_t)*p);
                 }
             }
         }
     }
 };
 
 class LedBlinkTask : public TaskClassS<256> {
    public:
     LedBlinkTask() : TaskClassS<256>("LedBlinkTask", TaskPrio_Low) {}
 
     void task() override {
         Logger &log = Logger::getInstance();
         LED led0(GPIOC, GPIO_PIN_6);
 
         for (;;) {
             led0.toggle();
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