#include <cmath>
#include <cstdio>

#include "FreeRTOS.h"
#include "bsp_led.h"
#include "bsp_log.h"
#include "bsp_uid.h"
#include "chronolink.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#ifdef __cplusplus
}
#endif

// 全局队列
void ledBlinkTask(void *pvParameters);
void uartDMATask(void *pvParameters);
void logTask(void *pvParameters);

extern UasrtConfig usart1_info;
FrameFragment frame_fragment;
ChronoLink chronoLink;

// 全局信号量
extern SemaphoreHandle_t dmaCompleteSemaphore;

Logger Log;

// 创建 USART_DMA_Handler 实例
USART_DMA_Handler uartDMA = USART_DMA_Handler(usart1_info);

int main(void) {
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

    UIDReader &uid = UIDReader::getInstance();

    Log.logQueue = xQueueCreate(10, LOG_QUEUE_SIZE);

    xTaskCreate(uartDMATask, "uartDMATask", 1024, &uartDMA, 1, NULL);
    xTaskCreate(ledBlinkTask, "ledBlinkTask", 256, NULL, 2, NULL);
    xTaskCreate(logTask, "logTask", 1024, nullptr, 3, nullptr);
    vTaskStartScheduler();
    for (;;);
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
                chronoLink.receiveAndAssembleFrame(frame_fragment);
            };
        }
    }
}

void ledBlinkTask(void *pvParameters) {
    LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);
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
                while (RESET == usart_flag_get(USART1, USART_FLAG_TBE));
                usart_data_transmit(USART1, (uint8_t)*p);
            }
        }
    }
}