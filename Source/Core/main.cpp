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
void led_task(void *pvParameters);
void uartDMATask(void *pvParameters);
void LogTask(void *pvParameters);

extern SerialConfig usart1_info;
FrameFragment frame_fragment;
ChronoLink chronoLink;

// 全局信号量
extern SemaphoreHandle_t dmaCompleteSemaphore;

Logger Log;

// 创建 UART_DMA_Handler 实例
UART_DMA_Handler uartDMA = UART_DMA_Handler(usart1_info);

int main(void) {
    // read uid
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

    UIDReader &uid = UIDReader::getInstance();

    Log.logQueue = xQueueCreate(10, 64);    

    xTaskCreate(uartDMATask, "UART DMA Task", 1024, &uartDMA, 1, NULL);
    xTaskCreate(led_task, "Task 2", 128, NULL, 2, NULL);
    xTaskCreate(LogTask, "LogTask", 1024, nullptr, 3, nullptr);
    vTaskStartScheduler();
    for (;;);
}

void uartDMATask(void *pvParameters) {
    // 创建信号量
    dmaCompleteSemaphore = xSemaphoreCreateBinary();
    // Log.d("Usart DMA task start.");
    UART_DMA_Handler *uartDMA = static_cast<UART_DMA_Handler *>(pvParameters);
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

void led_task(void *pvParameters) {
    LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);
    for (;;) {
        // Log.d("led_task!");
        led0.toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void LogTask(void *pvParameters) {
    char buffer[30];
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