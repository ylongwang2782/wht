#ifndef __FORWARD_HPP
#define __FORWARD_HPP
#include <algorithm>

#include "bsp_log.hpp"
#include "master_cfg.hpp"
#include "master_def.hpp"
extern Logger Log;
class DataForwardBase {
   public:
    DataForwardBase(PCmanagerMsg& msg) : pc_manager_msg(msg) {
        std::fill(reinterpret_cast<char*>(&data_forward),
                  reinterpret_cast<char*>(&data_forward) + sizeof(data_forward),
                  0);
    };
    ~DataForwardBase() {};

    DataForward data_forward;
    PCmanagerMsg& pc_manager_msg;

    bool forward() {
        if (pc_manager_msg.data_forward_queue.add(
                data_forward, PCinterface_FORWARD_QUEUE_TIMEOUT)) {
        } else {
            Log.e("Forward","pc_manager_msg.data_forward_queue.add failed");
            return false;
        }

        if (pc_manager_msg.event.wait(FORWARD_SUCCESS_EVENT, true, true,
                                      PCinterface_FORWARD_TIMEOUT)) {
        } else {
            Log.e("Forward","pc_manager_msg.event.take failed");
            return false;
        }
        return true;
    }
};

#endif