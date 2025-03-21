// master_mode.c
#include <cstdint>

#include "FreeRTOS.h"
#include "interface.hpp"
#include "mode_entry.h"
#include "task.h"

static void Master_Task(void *pvParameters) {
    Log.i("Master_Task: Boot");
    PCmanagerMsg pc_manger_msg;
    PCdataTransferMsg pc_data_transfer_msg;

    PCinterface pci(pc_manger_msg, pc_data_transfer_msg);
    PCdataTransfer pcdt(pc_data_transfer_msg);
    pci.give();
    pcdt.give();
    DataForward tmp;
    while (1) {
        // 主节点的操作
        // pc_manger_msg.forward_sem.take(1000);
        // printf("Master_Task: timeout\r\n");
        // printf("Master_Task: running\r\n");
        // pc_manger_msg.forward_sem.give();
        if (pc_manger_msg.data_forward_queue.pop(tmp, 0)) {
            switch (tmp.type) {
                case CmdType::DEV_CONF:
                pc_manger_msg.event.set(CONFIG_SUCCESS_EVENT);
                    break;
                case CmdType::DEV_MODE:
                pc_manger_msg.event.set(MODE_SUCCESS_EVENT);
                    break;
                case CmdType::DEV_RESET:
                pc_manger_msg.event.set(RESET_SUCCESS_EVENT);
                    break;
                case CmdType::DEV_CTRL:
                pc_manger_msg.event.set(CTRL_SUCCESS_EVENT);
                    break;
                case CmdType::DEV_QUERY:
                pc_manger_msg.event.set(QUERY_SUCCESS_EVENT);
                    break;
                default:
                    break;
            }
            pc_manger_msg.event.set(FORWARD_SUCCESS_EVENT);
            
        }
        vTaskDelay(pdMS_TO_TICKS(1000));    // 示例：100ms周期任务
    }
}

int Master_Init(void) {
    // 创建主节点任务
    xTaskCreate(Master_Task, "MasterTask", 4 * 1024, NULL, 2, NULL);
    return 0;
}
