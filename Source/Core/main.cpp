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

extern SerialConfig usart1_info;
FrameFragment frame_fragment;
ChronoLink chronoLink;

// 全局信号量
extern SemaphoreHandle_t dmaCompleteSemaphore;

int main(void) {
    // read uid
    UIDReader &uid = UIDReader::getInstance();

    // 创建信号量
    dmaCompleteSemaphore = xSemaphoreCreateBinary();

    // 创建 UART_DMA_Handler 实例
    UART_DMA_Handler uartDMA(usart1_info);

    printf("init done\n");
    // 创建任务
    xTaskCreate(uartDMATask, "UART DMA Task", 128, &uartDMA, 1, NULL);
    xTaskCreate(led_task, "Task 2", 128, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;);
}

void uartDMATask(void *pvParameters) {
    printf("Usart DMA task start.\n");
    UART_DMA_Handler *uartDMA = static_cast<UART_DMA_Handler *>(pvParameters);
    for (;;) {
        // 等待 DMA 完成信号
        if (xSemaphoreTake(dmaCompleteSemaphore, portMAX_DELAY) == pdPASS) {
            LOG("Usart recv.\n");
            chronoLink.push_back(uartDMA->DMA_RX_Buffer, usart1_info.rx_count);
            while (chronoLink.parseFrameFragment(frame_fragment)) {
                LOG("Get frame fragment.\n");
                chronoLink.receiveAndAssembleFrame(frame_fragment);
            };
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void led_task(void *pvParameters) {
    LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);
    for (;;) {
        led0.toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}