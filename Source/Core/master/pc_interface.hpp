#ifndef _MASTER_MODE_HPP_
#define _MASTER_MODE_HPP_
#include <cstdint>
#include <cstring>
#include <string>

#include "FreeRTOScpp.h"
#include "MutexCPP.h"
#include "QueueCPP.h"
#include "TaskCPP.h"
#include "bsp_log.hpp"
#include "bsp_uart.hpp"
#include "json.hpp"
#include "json_sorting.hpp"
#include "master_cfg.hpp"
#include "master_def.hpp"
#include "pc_message.hpp"

extern Logger Log;
extern UasrtInfo& pc_com_info;

// using json = nlohmann::json;

class PCdataTransfer : public TaskClassS<PCdataTransfer_STACK_SIZE> {
   public:
    PCdataTransfer(PCdataTransferMsg& msg)
        : TaskClassS<PCdataTransfer_STACK_SIZE>("PCdataTransfer",
                                                TaskPrio_High),
          __msg(msg) {}
    void task() override {
        Log.i("PCdataTransfer_Task: Boot");

        taskENTER_CRITICAL();
        UartConfig pc_com_cfg(pc_com_info, true);
        Uart pc_com(pc_com_cfg);
        taskEXIT_CRITICAL();

        uint8_t buffer[DMA_RX_BUFFER_SIZE];
        std::vector<uint8_t> rx_data;
        for (;;) {
            // 等待 DMA 完成信号
            if (xSemaphoreTake(pc_com_info.dmaRxDoneSema, 0) == pdPASS) {
                rx_data = pc_com.getReceivedData();
                for (auto it : rx_data) {
                    __msg.rx_data_queue.add(it);
                }
                __msg.rx_done_sem.give();
            };
            if (__msg.tx_request_sem.take(0)) {
                __msg.tx_share_mem.lock();
                const uint8_t* ptr = __msg.tx_share_mem.get();
                size_t size = __msg.tx_share_mem.size();

                pc_com.send(ptr, size);

                __msg.tx_share_mem.unlock();
                __msg.tx_done_sem.give();
            }
            TaskBase::delay(5);
        }
    }

   private:
    PCdataTransferMsg& __msg;
};

class PCinterface : public TaskClassS<PCinterface_STACK_SIZE> {
   public:
    using INST = CmdType;

    PCinterface(PCmanagerMsg& pc_manager_msg,
                PCdataTransferMsg& pc_data_transfer_msg)
        : TaskClassS<PCinterface_STACK_SIZE>("PCinterface_Task", TaskPrio_Mid),
          cfg_inst(pc_manager_msg),
          mode_inst(pc_manager_msg),
          reset_inst(pc_manager_msg),
          ctrl_inst(pc_manager_msg),
          query_inst(pc_manager_msg),
          transfer_msg(pc_data_transfer_msg),
          pmf(pc_manager_msg) {}

    void task() override {
        Log.i("PCinterface_Task: Boot");

        std::vector<uint8_t> buffer;
        std::vector<uint8_t> rsp_data;
        buffer.reserve(PCdataTransferMsg_DATA_QUEUE_SIZE);
        uint8_t recv_data;
        while (1) {
            transfer_msg.rx_done_sem.take();
            while (transfer_msg.rx_data_queue.pop(recv_data, 0) == pdPASS) {
                buffer.push_back(recv_data);
            }
            rsp_data = pmf.forward(buffer);
            rsp(rsp_data.data(), rsp_data.size());
            // jsonSorting(buffer.data(), buffer.size());
            buffer.clear();
        }
    }

   private:
    DeviceConfigInst cfg_inst;
    DevaceModeInst mode_inst;
    DeviceResetInst reset_inst;
    DeviceCtrlInst ctrl_inst;
    DeviceQueryInst query_inst;
    PCdataTransferMsg& transfer_msg;

    ProtocolMessageForward pmf;

   private:
    // 直接序列化到数组的函数
    void pc_message() {}

    void jsonSorting(uint8_t* ch, uint16_t len) {
        // 先检查JSON字符串是否有效
        if (!json::accept(ch, ch + len)) {
            Log.e("[PCinterface]: Invalid JSON format");
            return;
        }

        json j = json::parse((uint8_t*)ch, ch + len, nullptr, false);
        if (j.is_discarded()) {
            Log.e("[PCinterface]: json parse failed");
            return;
        }

        if (j.contains("inst")) {
            uint8_t inst = j["inst"];
            std::string rsp;
            switch (inst) {
                case DEV_CONF: {
                    // 设备配置解析
                    rsp = cfg_inst.forward(j);
                    break;
                }

                case DEV_MODE: {
                    // 设备模式解析
                    rsp = mode_inst.forward(j);
                    break;
                }
                case DEV_RESET:
                    // 设备复位解析
                    rsp = reset_inst.forward(j);
                    break;
                case DEV_CTRL:
                    // 设备控制解析
                    rsp = ctrl_inst.forward(j);
                    break;
                case DEV_QUERY:
                    // 设备查询解析
                    query_inst.forward(j);
                    break;
                default:
                    break;
            }

            // 获取共享内存写访问权限，避免在数据发送前共享内存内的数据被复写
            if (transfer_msg.tx_share_mem.get_write_access(
                    PC_TX_SHARE_MEM_ACCESS_TIMEOUT)) {
                transfer_msg.tx_share_mem.write((uint8_t*)rsp.c_str(),
                                                rsp.size());
                transfer_msg.tx_request_sem.give();
                if (!transfer_msg.tx_done_sem.take(PCinterface_RSP_TIMEOUT)) {
                    Log.e(
                        "[PCinterface]: json respond failed. tx_done_sem.take "
                        "failed");
                }
                // 释放写访问权限
                transfer_msg.tx_share_mem.release_write_access();
            } else {
                Log.e("[PCinterface]: tx_share_mem.get_write_access failed");
            }
        }
    }

    void rsp(uint8_t* ch, uint16_t len) {
        if (transfer_msg.tx_share_mem.get_write_access(
                PC_TX_SHARE_MEM_ACCESS_TIMEOUT)) {
            transfer_msg.tx_share_mem.write(ch, len);
            transfer_msg.tx_request_sem.give();
            if (!transfer_msg.tx_done_sem.take(PCinterface_RSP_TIMEOUT)) {
                Log.e(
                    "[PCinterface]: respond to PC failed: tx_done_sem.take "
                    "failed");
            }
            // 释放写访问权限
            transfer_msg.tx_share_mem.release_write_access();
        } else {
            Log.e(
                "[PCinterface]: respond to PC failed: "
                "tx_share_mem.get_write_access failed");
        }
    }
};

#endif    // _MASTER_MODE_HPP_