// master_mode.c
#include "mode_entry.h"
#include "FreeRTOS.h"
#include "task.h"
#include "master_mode.hpp"
#include <cstdint>

static void Master_Task(void *pvParameters) {
    Mutex frame_mtx("frame_lock");
    Lock frame_lock(frame_mtx, false);     // don't take it yet
    Frame frame;
    
    PCinterface pci(frame, frame_lock);
    pci.give();
    // Usartask usartTask(rxQueue);
    // usartTask.give();
    
    
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
