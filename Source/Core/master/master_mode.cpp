// master_mode.c
#include "FreeRTOS.h"
#include "FreeRTOScpp.h"
#include "bsp_uart.hpp"
#include "mode_entry.h"
#include "task.h"
#include "master_mode.hpp"
#include <cstdint>


extern Uart uart1;
static void Master_Task(void *pvParameters) {
    
    MasterMode master;
    master.give();
    while (1) {
        // 主节点的操作
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int Master_Init(void) {
    // 创建主节点任务,10kByte堆栈
    xTaskCreate(Master_Task, "MasterMain", 2560, NULL, 1, NULL);
    return 0;
}
