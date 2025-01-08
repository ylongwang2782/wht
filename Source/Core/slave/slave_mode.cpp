#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "FreeRTOS.h"
#include "bsp_led.h"
#include "bsp_log.h"
#include "bsp_uid.h"
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

// 任务列表
void ledBlinkTask(void *pvParameters);
void timerTask(void *pvParameters);
void uartDMATask(void *pvParameters);
void logTask(void *pvParameters);

void frameSorting(ChronoLink::CompleteFrame complete_frame);

extern UasrtConfig usart1_info;
extern Harness harness;
ChronoLink::Fragment frame_fragment;
ChronoLink chronoLink;

// 全局信号量
extern SemaphoreHandle_t dmaCompleteSemaphore;

Logger Log;

// 创建 USART_DMA_Handler 实例
USART_DMA_Handler uartDMA = USART_DMA_Handler(usart1_info);

void gpioCollectCallback(void) { harness.collect_pin_states(); }

TimerHandle_t xTimerHandle;    // 定时器句柄
TaskHandle_t xTaskHandle;      // 任务句柄

// 定时器回调函数
void timerCallback(TimerHandle_t xTimer) {
    // Log.v("Timer callback triggered.");
    xTaskNotifyGive(xTaskHandle);
}

int Slave_Init(void) {
    UIDReader &uid = UIDReader::getInstance();

    Log.logQueue = xQueueCreate(10, LOG_QUEUE_SIZE);

    // 创建定时器
    xTimerHandle = xTimerCreate("Timer",                // 定时器名称
                                pdMS_TO_TICKS(1000),    // 周期 10ms
                                pdTRUE,                 // 自动重装
                                NULL,                   // 定时器 ID
                                timerCallback           // 回调函数
    );

    harness.init();
    xTaskCreate(uartDMATask, "uartDMATask", 1024, &uartDMA, 1, NULL);
    xTaskCreate(timerTask, "TimerTask", 256, NULL, 2, &xTaskHandle);
    xTaskCreate(ledBlinkTask, "ledBlinkTask", 256, NULL, 4, NULL);
    xTaskCreate(logTask, "logTask", 1024, nullptr, 5, nullptr);

    // 启动定时器
    if (xTimerStart(xTimerHandle, 0) != pdPASS) {
        printf("Error: Timer start failed!\n");
        return -1;
    }
    return 0;
}

void uartDMATask(void *pvParameters) {
    // 创建信号量
    dmaCompleteSemaphore = xSemaphoreCreateBinary();
    // Log.d("Usart DMA task start.");
    USART_DMA_Handler *uartDMA = static_cast<USART_DMA_Handler *>(pvParameters);
    for (;;) {
        // 等待 DMA 完成信号
        if (xSemaphoreTake(dmaCompleteSemaphore, portMAX_DELAY) == pdPASS) {
            Log.d("Usart recv.");
            chronoLink.push_back(uartDMA->DMA_RX_Buffer, usart1_info.rx_count);
            while (chronoLink.parseFrameFragment(frame_fragment)) {
                Log.d("Get frame fragment.");
                chronoLink.receiveAndAssembleFrame(frame_fragment,
                                                   frameSorting);
            };
        }
    }
}

ChronoLink::DataReplyContext dataReply = {.deviceStatus = 0x0002,
                                          .harnessLength = 2,
                                          .harnessData = {0x10, 0x20},
                                          .clipLength = 1,
                                          .clipData = {0x30}};

void frameSorting(ChronoLink::CompleteFrame complete_frame) {
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

                ChronoLink::DeviceConfig deviceConfig = {.timeslot = 0,
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
                    .deviceStatus = 0x0002,
                    .harnessLength = 2,
                    .harnessData = {0x10, 0x20},
                    .clipLength = 1,
                    .clipData = {0x30}};

                chronoLink.sendReply(2, ChronoLink::REPLY, ChronoLink::DATA_REQ,
                                     ChronoLink::OK, dataReply);

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
        case ChronoLink::REPLY:
            Log.d("Frame: Reply");
            break;

        default:
            break;
    }
}

// 任务函数
void timerTask(void *pvParameters) {
    Log.d("Timer task started.");

    uint8_t trigger_count = 0;

    Log.v("trigger_count: %d", trigger_count);

    for (;;) {
        // 等待通知
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // 增加触发次数
        trigger_count++;
    }

    // 删除任务
    vTaskDelete(NULL);
}

void ledBlinkTask(void *pvParameters) {
    LED led0(GPIOC, GPIO_PIN_6);
    for (;;) {
        // Log.d("ledBlinkTask!");
        led0.toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void logTask(void *pvParameters) {
    char buffer[LOG_QUEUE_SIZE + 8];
    for (;;) {
        if (xQueueReceive(Log.logQueue, buffer, portMAX_DELAY)) {
            // TODO 更换为dma发送
            for (const char *p = buffer; *p; ++p) {
                while (RESET == usart_flag_get(USART_LOG, USART_FLAG_TBE));
                usart_data_transmit(USART_LOG, (uint8_t)*p);
            }
        }
    }
}