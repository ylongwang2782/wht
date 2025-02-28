#ifndef _MASTER_MODE_HPP_
#define _MASTER_MODE_HPP_
#include <cstdint>
#include <cstring>

#include "FreeRTOScpp.h"
#include "Lock.h"
#include "MutexCPP.h"
#include "QueueCPP.h"
#include "TaskCPP.h"
#include "bsp_log.hpp"
#include "bsp_uart.hpp"
#include "chronolink.h"
#include "json.hpp"
using namespace CLink;
extern UasrtInfo usart1_info;
extern UartConfig uart1Conf;
extern Uart uart1;
#define QUEUE_LEN 1024
struct Frame {
    CompleteFrameType frame_recv;
    CommandFrameType<DEV_CONF> dev_cfg;
    CommandFrameType<DEV_UNLOCK> dev_ulk;
    std::vector<uint8_t> frame_output;
};
class Usartask : public TaskClassS<2048> {
   public:
    Usartask(Queue<uint8_t>& q)
        : TaskClassS<2048>("UsartDMATask", TaskPrio_High), __q(q) {}
    void task() override {
        Logger& log = Logger::getInstance();
        log.i("UsartDMATask boot\r\n");
        for (;;) {
            // 等待 DMA 完成信号
            if (xSemaphoreTake(usart1_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                log.d("Usart recv.\r\n");
                uint8_t buffer[512];
                uint16_t len =
                    uart1.getReceivedData(buffer, DMA_RX_BUFFER_SIZE);
                for (int i = 0; i < len; i++) {
                    __q.add(buffer[i]);
                }
            };
        }
    }

   private:
    Queue<uint8_t>& __q;
};

class PCinterface : public TaskClassS<1024> {
   public:
    using json = nlohmann::json;
    enum INST : uint8_t {
        ACQ = 0,
        ULK,
    };
    enum CTRL : uint8_t {
        DISABLE = 0,
        ENABLE,
    };
    PCinterface(Frame& frame, Lock& frame_lock)
        : TaskClassS<1024>("MasterMain", TaskPrio_Low),
          frame(frame),
          frame_lock(frame_lock) {}
    Logger& log = Logger::getInstance();
    void task() override {
        log.i("MasterMode boot\r\n");
        dev_slot = 0;
        uint8_t buffer[DMA_RX_BUFFER_SIZE];
        while (1) {
            if (xSemaphoreTake(usart1_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                uint16_t len = uart1.getReceivedData(buffer, 512);
                frame_lock.lock();
                if (frame_lock.locked()) {
                    jsonSorting(buffer, len);
                    frame_lock.unlock();
                }
                // vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }

   private:
    CTRL ctrl = DISABLE;
    Frame& frame;
    Lock& frame_lock;

   private:
    bool parseIdString(const std::string& id, uint8_t tar_id[4]) {
        const char* ptr = id.c_str();
        int index = 0;

        while (*ptr && index < 4) {
            uint8_t value = 0;
            for (int i = 0; i < 2; ++i) {
                if (*ptr >= '0' && *ptr <= '9') {
                    value = (value << 4) + (*ptr - '0');
                } else if (*ptr >= 'A' && *ptr <= 'F') {
                    value = (value << 4) + (*ptr - 'A' + 10);
                } else if (*ptr >= 'a' && *ptr <= 'f') {
                    value = (value << 4) + (*ptr - 'a' + 10);
                } else {
                    return false;    // 解析出错
                }
                ptr++;
            }
            tar_id[index++] = value;
            if (*ptr == '-') {
                ptr++;
            }
        }
        return index == 4;    // 确保解析了 4 个字节
    }
    void jsonSorting(uint8_t* ch, uint16_t len) {
        json j = json::parse((uint8_t*)ch, ch + len);
        if (j.contains("inst")) {
            uint8_t inst = j["inst"];
            if (inst == ACQ) {
                // 设备配置解析
                if (j.contains("cfg")) {
                    auto cfg = j["cfg"];
                    uint16_t totalHarnessNum = 0;
                    uint16_t startHarnessNum = 0;
                    uint16_t hrnsNum;
                    uint8_t slot = 0;
                    uint8_t tar_id[4];
                    // 计算总线数
                    for (const auto& item : cfg) {
                        hrnsNum = item["hrnsNum"];
                        totalHarnessNum += hrnsNum;
                    }

                    for (const auto& item : cfg) {
                        // 提取id
                        std::string id = item["id"];
                        parseIdString(id, tar_id);

                        // 分配时隙
                        frame.dev_cfg.cfg.normalCfg.timeslot = slot++;

                        // 设置总线数
                        frame.dev_cfg.cfg.normalCfg.totalHarnessNum =
                            totalHarnessNum;

                        // 设置clip数
                        frame.dev_cfg.cfg.normalCfg.clipNum = item["clipNum"];

                        // 设置目标设备的检测线数
                        frame.dev_cfg.cfg.normalCfg.harnessNum =
                            item["hrnsNum"];

                        // 设置起始总线号
                        frame.dev_cfg.cfg.normalCfg.startHarnessNum =
                            startHarnessNum;
                        startHarnessNum +=
                            frame.dev_cfg.cfg.normalCfg.harnessNum;

                        // 设置电阻索引
                        if (item.contains("resIdx")) {
                            frame.dev_cfg.cfg.resNum =
                                item["resIdx"].get<std::vector<uint8_t>>();
                        }

                        log.d("tar_id:          %.2X-%.2X-%.2X-%.2X", tar_id[0],
                              tar_id[1], tar_id[2], tar_id[3]);
                        log.d("timeslot:        %d",
                              frame.dev_cfg.cfg.normalCfg.timeslot);
                        log.d("totalHarnessNum: %d",
                              frame.dev_cfg.cfg.normalCfg.totalHarnessNum);
                        log.d("startHarnessNum: %d",
                              frame.dev_cfg.cfg.normalCfg.startHarnessNum);
                        log.d("hrnsNum:         %d",
                              frame.dev_cfg.cfg.normalCfg.harnessNum);
                        log.d("clipNum:         %d",
                              frame.dev_cfg.cfg.normalCfg.clipNum);

                        frame.frame_output.clear();
                        frame.dev_cfg.pack(tar_id, frame.frame_output);
                        // log.d("pack data:");
                        // for (auto i : frame.frame_output) {
                        //     log.r("%.2X ", i);
                        // }
                        log.r("\n\n");
                    }
                }
                if (j.contains("ctrl")) {
                    ctrl = j["ctrl"];
                }
            } else if (inst == ULK) {
                auto ulk = j["id"];
                uint8_t tar_id[4];
                for (const auto& id : ulk) {
                    std::string idStr = id.get<std::string>();
                    parseIdString(idStr, tar_id);
                    frame.dev_ulk.unlock.lockStatus = 0;
                    frame.frame_output.clear();
                    frame.dev_ulk.pack(tar_id, frame.frame_output);

                    log.d("tar_id: ");
                    for (int i = 0; i < 4; i++) {
                        log.r("%.2X ", tar_id[i]);
                    }
                    log.r("\r\n");
                }
            }
        }
    }
};

#endif    // _MASTER_MODE_HPP_