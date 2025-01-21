// master_mode.c
#include "mode_entry.h"
#include "FreeRTOS.h"
#include "task.h"

#include <cstddef> // For size_t
#include "bsp_allocate.hpp"


static void Master_Task(void *pvParameters) {
    while (1) {
        // 主节点的操作
        vTaskDelay(pdMS_TO_TICKS(100)); // 示例：100ms周期任务
    }
}

int Master_Init(void) {
    // 创建主节点任务
    xTaskCreate(Master_Task, "MasterTask", 1024, NULL, 2, NULL);
    return 0;
}

