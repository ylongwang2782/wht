#pragma once
#include <vector>

#include "FreeRTOS.h"
#include "QueueCPP.h"
#include "SemaphoreCPP.h"
#include "TaskCPP.h"
#include "bsp_log.hpp"
#include "harness.h"
#include "protocol.hpp"
#include "uwb_interface.hpp"

#define SLAVE_USE_UWB
#define ManagerDataTransferMsg_RXDATA_QUEUE_SIZE 2048
// 发送数据入队超时
#define MsgProc_TX_QUEUE_TIMEOUT 1000
// 发送超时
#define MsgProc_TX_TIMEOUT 1000

#define ManagerDataTransferTask_SIZE     1024
#define ManagerDataTransferTask_PRIORITY TaskPrio_High

class ManagerDataTransferMsg {
   public:
    ManagerDataTransferMsg()
        : rx_done_sem("rx_done_sem"),
          tx_request_sem("tx_request_sem"),
          tx_done_sem("tx_done_sem"),

          tx_data_queue("manager_tx_data_queue"),
          rx_data_queue("manager_rx_data_queue") {}

   public:
    BinarySemaphore rx_done_sem;
    BinarySemaphore tx_request_sem;
    BinarySemaphore tx_done_sem;

    Queue<uint8_t, ManagerDataTransferMsg_RXDATA_QUEUE_SIZE> tx_data_queue;
    Queue<uint8_t, ManagerDataTransferMsg_RXDATA_QUEUE_SIZE> rx_data_queue;
};

class ManagerDataTransferTask
    : public TaskClassS<ManagerDataTransferTask_SIZE> {
   public:
    ManagerDataTransferTask(ManagerDataTransferMsg& __manager_transfer_msg)
        : TaskClassS("SlaveDataTransfer", ManagerDataTransferTask_PRIORITY),
          transfer_msg(__manager_transfer_msg) {}

   private:
    ManagerDataTransferMsg& transfer_msg;
    void task() override {
        UWB<UwbUartInterface> uwb;
        Log.i("ManagerDataTransferTask", "uwb.size=%d", sizeof(uwb));
        std::vector<uint8_t> buffer = {1, 2, 3, 4, 5};
        uint8_t data = 0;
        std::vector<uint8_t> rx;
        uwb.set_recv_mode();

        for (;;) {
            if (transfer_msg.tx_request_sem.take(0)) {
                buffer.reserve(transfer_msg.tx_data_queue.waiting());
                buffer.clear();
                while (transfer_msg.tx_data_queue.pop(data, 0)) {
                    buffer.push_back(data);
                }
                uwb.data_transmit(buffer);
                transfer_msg.tx_done_sem.give();
                uwb.set_recv_mode();
            }

            if (uwb.get_recv_data(buffer)) {
                for (auto it = buffer.begin(); it != buffer.end(); it++) {
                    transfer_msg.rx_data_queue.add(*it);
                }
                transfer_msg.rx_done_sem.give();
            }

            uwb.update();
            TaskBase::delay(1);
        }
    }
};

class MsgProc {
   public:
    MsgProc(ManagerDataTransferMsg& __transfer_msg)
        : transfer_msg(__transfer_msg) {}

    static constexpr const char TAG[] = "MsgProc";
    std::vector<uint8_t> recv_data;

   public:
    ManagerDataTransferMsg& transfer_msg;
    FrameParser frame_parser;

    void proc() {
        uint8_t data;
        while (transfer_msg.rx_data_queue.pop(data, 0)) {
            // 处理接收到的数据
            recv_data.push_back(data);
        }
        // check if recv_data is empty
        if (recv_data.size() != 0) {
            auto msg = frame_parser.parse(recv_data);
            if (msg != nullptr) {
                // 处理解析后的数据
                msg->process();
            } else {
                Log.e(TAG, "parse failed");
            }
            recv_data.clear();
        }
    }

    bool send(std::vector<uint8_t>& frame) {
        // 将发送数据写入队列
        for (auto it = frame.begin(); it != frame.end(); it++) {
            if (transfer_msg.tx_data_queue.add(*it, MsgProc_TX_QUEUE_TIMEOUT) ==
                false) {
                Log.e(TAG, "tx_data_queue.add failed");
                return false;
            }
        }

        // 请求数据发送
        transfer_msg.tx_request_sem.give();

        // 等待数据发送完成
        if (transfer_msg.tx_done_sem.take(MsgProc_TX_TIMEOUT) == false) {
            Log.e(TAG, "tx_done_sem.take failed, timeout");
            return false;
        }
        return true;
    }

   private:
};