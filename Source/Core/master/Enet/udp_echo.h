#pragma once
#ifndef UDP_ECHO_H
#define UDP_ECHO_H
#include <cstdint>
#include <vector>

#include "TaskCPP.h"

#define UDP_TASK_DEPTH 512

/* initialize the tcp_client application */
class UdpTask : public TaskClassS<UDP_TASK_DEPTH> {
   public:
    UdpTask();
    void task() override;
    std::vector<uint8_t> send_buf;
};
#endif /* UDP_ECHO_H */
