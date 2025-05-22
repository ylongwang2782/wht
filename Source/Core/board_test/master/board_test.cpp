
#include "board_test.hpp"

#include <cstddef>
#include <cstdio>

#include "TaskCPP.h"
#include "bsp_led.hpp"
#include "bsp_log.hpp"
#include "bsp_uid.hpp"
#include "enet.h"
#include "ethernetif.h"
#include "lwip/api.h"
#include "lwip/memp.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "netcfg.h"
#include "netconf.h"

#ifdef MASTER_BOARDTEST

UasrtInfo& log_com_info = uart7_info;
UartConfig log_com_cfg(log_com_info, false);
Uart log_com(log_com_cfg);
Logger Log(log_com);

#define UDP_ECHO_TASK_DEPTH 1024
#define UDP_ECHO_TASK_PRIO  TaskPrio_Mid

#define MAX_BUF_SIZE 100

class UdpEchoTask : public TaskClassS<UDP_ECHO_TASK_DEPTH> {
   public:
    UdpEchoTask()
        : TaskClassS<UDP_ECHO_TASK_DEPTH>("UdpEchoTask", UDP_ECHO_TASK_PRIO) {}

   public:
    void task() override {
        int ret, recvnum, sockfd = -1;
        int rmt_port = 8080;
        int bod_port = 8080;
        struct sockaddr_in rmt_addr, bod_addr;
        char buf[100];
        u32_t len;
        ip_addr_t ipaddr;

        IP4_ADDR(&ipaddr, IP_S_ADDR0, IP_S_ADDR1, IP_S_ADDR2, IP_S_ADDR3);

        rmt_addr.sin_family = AF_INET;
        rmt_addr.sin_port = htons(rmt_port);
        rmt_addr.sin_addr.s_addr = ipaddr.addr;

        bod_addr.sin_family = AF_INET;
        bod_addr.sin_port = htons(bod_port);
        bod_addr.sin_addr.s_addr = htons(INADDR_ANY);

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        // 设置为非阻塞
        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags < 0) {
            // 获取失败处理
            lwip_close(sockfd);
            vTaskDelete(nullptr);
            return;
        }
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        Log.v("UDP", "rmt_port: %d", rmt_port);
        Log.v("UDP", "bod_port: %d", bod_port);

        if (sockfd < 0) {
            vTaskDelete(nullptr);
            return;
        }

        if (bind(sockfd, (struct sockaddr*)&bod_addr, sizeof(bod_addr)) < 0) {
            lwip_close(sockfd);
            vTaskDelete(nullptr);
            return;
        }

        for (;;) {
            recvnum = recvfrom(sockfd, buf, MAX_BUF_SIZE, 0,
                               (struct sockaddr*)&rmt_addr, &len);
            if (recvnum > 0) {
                Log.v("UDP", "recvfrom: %d", recvnum);
                sendto(sockfd, buf, recvnum, 0, (struct sockaddr*)&rmt_addr,
                       sizeof(rmt_addr));
                recvnum = 0;
            }
            TaskBase::delay(10);
        }
    };
};

class UwbDevice : public TaskClassS<1024> {
   public:
    static constexpr const char TAG[] = "UWB";
    UwbDevice() : TaskClassS<1024>("UwbDevice", TaskPrio_High) {};
    void task() override {
        UWB<UwbUartInterface> uwb;
        for (;;) {
            TaskBase::delay(10);
        }
    }
};

static void BootTask(void* pvParameters) {
    static constexpr const char TAG[] = "BOOT";
    LED sysLed(GPIO::Port::A, GPIO::Pin::PIN_0);
    int ret;

    LogTask logTask(Log);
    logTask.give();
    Log.d(TAG, "LogTask initialized");

    uint32_t myUid = UIDReader::get();
    Log.d(TAG, "Master Board Test Firmware v%s, Build: %s %s", FIRMWARE_VERSION,
          __DATE__, __TIME__);
    Log.d(TAG, "UID: %08X", myUid);

    EthDevice ethDevice;
    ret = ethDevice.init();
    if (ret != 0) {
        Log.e(TAG, "ethDevice init failed");
    } else {
        Log.d(TAG, "ethDevice init success");
        UdpEchoTask udpEchoTask;
        udpEchoTask.give();
        Log.d(TAG, "UDP Echo Task Start");
    }

    UwbDevice uwbDevice;
    uwbDevice.give();
    Log.d(TAG, "UWB Device Task Start");

    while (1) {
        // Log.d("heap minimum: %d", xPortGetMinimumEverFreeHeapSize());
        sysLed.toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int master_boardtest_init(void) {
    xTaskCreate(BootTask, "BootTask", 10 * 1024, NULL, 2, NULL);

    return 0;
}

#endif