// master_mode.c
#include <cstdint>
#include <cstdio>

#include "FreeRTOS.h"
#include "bsp_led.hpp"
#include "bsp_spi.hpp"
#include "interface.hpp"
#include "mode_entry.h"
#include "slave_manager.hpp"
#include "task.h"

#ifdef MASTER
UartConfig usart1Conf(usart1_info);
UartConfig usart5Conf(usart5_info, false);
UartConfig uart7Conf(uart7_info, false);
// UartConfig uart3Conf(uart3_info);

Uart rs232_1(usart5Conf);
Uart usart1(usart1Conf);
Uart uart7(uart7Conf);

Logger Log(uart7);

bool __ProcessBase::rsp_parsed = false;    // 从机回复正确标志位
Slave2MasterMessageID
    __ProcessBase::expected_rsp_msg_id;    // 期望从机回复的消息ID

class __LogTask : public TaskClassS<1024> {
   public:
    __LogTask() : TaskClassS<1024>("LogTask", TaskPrio_Low) {}

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

static void Master_Task(void *pvParameters) {
    LED led(GPIO::Port::A, GPIO::Pin::PIN_0);
    __LogTask logTask;
    logTask.give();
    // printf("Master_Task: Boot\n");
    Log.i("Master_Task: Boot");

    // std::vector<NSS_IO> nss_io_list = {
    //     {GPIOB, GPIO_PIN_12},
    // };
    // SpiDev<SpiMode::Master> master_dev(SPI1_C1MOSI_C2MISO_B10SCLK_B12NSS,
    // nss_io_list);

    // 上位机数据传输任务 json解析任务 初始化
    PCmanagerMsg pc_manger_msg;
    PCdataTransferMsg pc_data_transfer_msg;

    PCinterface pc_interface(pc_manger_msg, pc_data_transfer_msg);
    PCdataTransfer pc_data_transfer(pc_data_transfer_msg);
    pc_interface.give();
    pc_data_transfer.give();

    // 从机数据传输任务 从机管理任务 初始化
    ManagerDataTransferMsg manager_transfer_msg;
    SlaveManager slave_manager(pc_manger_msg, manager_transfer_msg);
    ManagerDataTransfer manager_data_transfer(manager_transfer_msg);

    slave_manager.give();
    manager_data_transfer.give();

    DataForward tmp;
    while (1) {
        // 主节点的操作

        // if (pc_manger_msg.data_forward_queue.pop(tmp, 0)) {
        //     switch (tmp.type) {
        //         case CmdType::DEV_CONF:
        //         printf("Master_Task: Config data received\n");
        //         pc_manger_msg.event.set(CONFIG_SUCCESS_EVENT);
        //             break;
        //         case CmdType::DEV_MODE:
        //         pc_manger_msg.event.set(MODE_SUCCESS_EVENT);
        //             break;
        //         case CmdType::DEV_RESET:
        //         pc_manger_msg.event.set(RESET_SUCCESS_EVENT);
        //             break;
        //         case CmdType::DEV_CTRL:
        //         pc_manger_msg.event.set(CTRL_SUCCESS_EVENT);
        //             break;
        //         case CmdType::DEV_QUERY:
        //         pc_manger_msg.event.set(QUERY_SUCCESS_EVENT);
        //             break;
        //         default:
        //             break;
        //     }
        //     pc_manger_msg.event.set(FORWARD_SUCCESS_EVENT);
        // }
        // printf("Master_Task running\n");
        // char taskListBuffer[10 * 100];
        // vTaskList(taskListBuffer);
        // printf("\nname          state   priority   stack   task number\n");
        // printf("%s", taskListBuffer);
        // printf("heap minimum: %d\n", xPortGetMinimumEverFreeHeapSize());
        rs232_1.send((uint8_t *)"Master_Task running\n", 20);
        led.toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int Master_Init(void) {
    // 创建主节点任务
    xTaskCreate(Master_Task, "MasterTask", 4 * 1024, NULL, 2, NULL);
    return 0;
}

void WriteCondInfoMsg::process() {}

void WriteClipInfoMsg::process() {}

void SyncMsg::process() { __ProcessBase::rsp_parsed = true; }

void CondInfoMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        Slave2MasterMessageID::COND_INFO_MSG) {
        Log.e("CondInfoMsg: msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
    if (CondInfoMsg::timeSlot != WriteCondInfoMsg::timeSlot) {
        Log.e("CondInfoMsg: timeSlot not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondInfoMsg::interval != WriteCondInfoMsg::interval) {
        Log.e("CondInfoMsg: interval not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondInfoMsg::totalConductionNum !=
        WriteCondInfoMsg::totalConductionNum) {
        Log.e("CondInfoMsg: totalConductionNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondInfoMsg::startConductionNum !=
        WriteCondInfoMsg::startConductionNum) {
        Log.e("CondInfoMsg: startConductionNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (CondInfoMsg::conductionNum != WriteCondInfoMsg::conductionNum) {
        Log.e("CondInfoMsg: conductionNum not match");
        __ProcessBase::rsp_parsed = false;
    }
}

void ClipInfoMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        Slave2MasterMessageID::CLIP_INFO_MSG) {
        Log.e("ClipInfoMsg: msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
    if (ClipInfoMsg::mode != WriteClipInfoMsg::mode) {
        Log.e("ClipInfoMsg: mode not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ClipInfoMsg::clipPin != WriteClipInfoMsg::clipPin) {
        Log.e("ClipInfoMsg: clipPin not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ClipInfoMsg::interval != WriteClipInfoMsg::interval) {
        Log.e("ClipInfoMsg: clipNum not match");
        __ProcessBase::rsp_parsed = false;
    }
}

void ResInfoMsg::process() {
    __ProcessBase::rsp_parsed = true;
    if (__ProcessBase::expected_rsp_msg_id !=
        Slave2MasterMessageID::RES_INFO_MSG) {
        Log.e("ResInfoMsg: msg_id not match");
        __ProcessBase::rsp_parsed = false;
        return;
    }
    if (ResInfoMsg::timeSlot != WriteResInfoMsg::timeSlot) {
        Log.e("ResInfoMsg: timeSlot not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResInfoMsg::interval != WriteResInfoMsg::interval) {
        Log.e("ResInfoMsg: interval not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResInfoMsg::totalResistanceNum != WriteResInfoMsg::totalResistanceNum) {
        Log.e("ResInfoMsg: totalResistanceNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResInfoMsg::startResistanceNum != WriteResInfoMsg::startResistanceNum) {
        Log.e("ResInfoMsg: startResistanceNum not match");
        __ProcessBase::rsp_parsed = false;
    }
    if (ResInfoMsg::resistanceNum != WriteResInfoMsg::resistanceNum) {
        Log.e("ResInfoMsg: resistanceNum not match");
        __ProcessBase::rsp_parsed = false;
    }
}

#endif