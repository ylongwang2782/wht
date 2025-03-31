#ifndef _MASTER_MODE_HPP_
#define _MASTER_MODE_HPP_
#include <cstdint>
#include <cstring>

#include "MutexCPP.h"
#include "QueueCPP.h"
#include "TaskCPP.h"
#include "bsp_log.hpp"
#include "bsp_uart.hpp"
#include "json.hpp"
#include "master_cfg.hpp"
#include "master_def.hpp"
extern Logger Log;
extern UasrtInfo usart1_info;
// extern UartConfig uart1Conf;
extern Uart usart1;

using json = nlohmann::json;

class __IfBase {
   public:
    __IfBase(PCmanagerMsg& msg) : pc_manager_msg(msg) {
        memset(&data_forward, 0, sizeof(data_forward));
    };

   public:
    struct __devID {
        uint8_t id[4];
    };
    DataForward data_forward;
    PCmanagerMsg& pc_manager_msg;

   public:
    static bool parseIdString(const std::string& id, uint8_t tar_id[4]) {
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

    static uint16_t hexStringToUint16(const std::string& hex_str) {
        uint16_t result = 0;
        for (char c : hex_str) {
            if (c >= '0' && c <= '9') {
                result = (result << 4) + (c - '0');
            } else if (c >= 'A' && c <= 'F') {
                result = (result << 4) + (c - 'A' + 10);
            } else if (c >= 'a' && c <= 'f') {
                result = (result << 4) + (c - 'a' + 10);
            } else {
                // 遇到非十六进制字符，返回 0
                return 0;
            }
        }
        return result;
    }

    bool forward() {
        if (pc_manager_msg.data_forward_queue.add(
                data_forward, PCinterface_FORWARD_QUEUE_TIMEOUT)) {
        } else {
            Log.e("pc_manager_msg.data_forward_queue.add failed");
            return false;
        }

        if (pc_manager_msg.event.wait(FORWARD_SUCCESS_EVENT, true, true,
                                      PCinterface_FORWARD_TIMEOUT)) {
        } else {
            Log.e("pc_manager_msg.event.take: forward event, failed");
            return false;
        }
        return true;
    }
};

class DeviceConfigInst : private __IfBase {
   public:
    DeviceConfigInst(PCmanagerMsg& forward_msg) : __IfBase(forward_msg) {
        cfg_false_dev.reserve(10);
    }

   private:
    uint16_t hrnsNum;
    bool cfg_success = false;

   public:
    std::vector<__devID> cfg_false_dev;

   public:
    void forward(json& j) {
        data_forward.type = CmdType::DEV_CONF;
        memset(&data_forward.cfg_cmd, 0, sizeof(data_forward.cfg_cmd));
        cfg_false_dev.clear();
        if (j.contains("params")) {
            auto params = j["params"];
            // 计算总线数
            data_forward.cfg_cmd.totalHarnessNum = 0;
            data_forward.cfg_cmd.startHarnessNum = 0;
            for (const auto& item : params) {
                hrnsNum = item["cond"];
                data_forward.cfg_cmd.totalHarnessNum += hrnsNum;
            }

            size_t slave_num = params.size();
            size_t index = 0;

            for (const auto& item : params) {
                if (index == slave_num - 1) {
                    data_forward.cfg_cmd.is_last_dev = true;
                } else {
                    data_forward.cfg_cmd.is_last_dev = false;
                }
                index++;

                // 提取id
                if (item.contains("id")) {
                    std::string id = item["id"];
                    parseIdString(id, data_forward.cfg_cmd.id);
                    Log.i(
                        "cfg_cmd.id: %.2X-%.2X-%.2X-%.2X",
                        data_forward.cfg_cmd.id[0], data_forward.cfg_cmd.id[1],
                        data_forward.cfg_cmd.id[2], data_forward.cfg_cmd.id[3]);
                }

                // 设置目标设备的检测线数
                if (item.contains("cond")) {
                    data_forward.cfg_cmd.cond = item["cond"];
                    Log.i("cfg_cmd.cond: %u", data_forward.cfg_cmd.cond);
                }

                // 设置阻抗检测线数
                if (item.contains("Z")) {
                    data_forward.cfg_cmd.Z = item["Z"];
                    Log.i("cfg_cmd.Z: %u", data_forward.cfg_cmd.Z);
                }

                // 设置clip数
                if (item.contains("clip")) {
                    auto& clip = item["clip"];

                    data_forward.cfg_cmd.clip_exist = true;

                    // 解析mode字段
                    if (clip.contains("mode")) {
                        data_forward.cfg_cmd.clip_mode = clip["mode"];
                    }

                    // 解析pin字段(16进制字符串转数值)
                    if (clip.contains("pin") && clip["pin"].is_string()) {
                        std::string pinStr = clip["pin"];
                        data_forward.cfg_cmd.clip_pin =
                            __IfBase::hexStringToUint16(pinStr);
                    }

                    Log.i("clip_mode: %u, clip_pin: 0x%04X",
                          data_forward.cfg_cmd.clip_mode,
                          data_forward.cfg_cmd.clip_pin);
                }

                Log.i("cfg_cmd.totalHarnessNum: %u",
                      data_forward.cfg_cmd.totalHarnessNum);
                Log.i("cfg_cmd.startHarnessNum: %u",
                      data_forward.cfg_cmd.startHarnessNum);

                cfg_success = false;
                if (__IfBase::forward()) {
                    if ((pc_manager_msg.event.get() & CONFIG_SUCCESS_EVENT)) {
                        cfg_success = true;
                    }
                }

                if (cfg_success) {
                    Log.i(
                        "dev %.2X-%.2X-%.2X-%.2X config success\n",
                        data_forward.cfg_cmd.id[0], data_forward.cfg_cmd.id[1],
                        data_forward.cfg_cmd.id[2], data_forward.cfg_cmd.id[3]);
                } else {
                    Log.e(
                        "dev %.2X-%.2X-%.2X-%.2X config failed\n",
                        data_forward.cfg_cmd.id[0], data_forward.cfg_cmd.id[1],
                        data_forward.cfg_cmd.id[2], data_forward.cfg_cmd.id[3]);

                    __devID tmp;
                    memcpy(tmp.id, data_forward.cfg_cmd.id, 4);
                    cfg_false_dev.push_back(tmp);
                }

                data_forward.cfg_cmd.startHarnessNum +=
                    data_forward.cfg_cmd.cond;
            }
        }
    }
};

class DevaceModeInst : private __IfBase {
   public:
    DevaceModeInst(PCmanagerMsg& forward_msg) : __IfBase(forward_msg) {}
    SysMode mode;

   private:
    bool mode_success = false;

   public:
    void forward(json& j) {
        if (j.contains("mode")) {
            mode = static_cast<SysMode>(j["mode"]);
            data_forward.type = CmdType::DEV_MODE;
            data_forward.mode_cmd.mode = mode;
            Log.i("mode_cmd.mode: %u", data_forward.mode_cmd.mode);

            mode_success = false;
            if (__IfBase::forward()) {
                if ((pc_manager_msg.event.get() & MODE_SUCCESS_EVENT))
                    mode_success = true;
            }

            if (mode_success) {
                Log.i("mode success\n");
            } else {
                Log.e("mode failed\n");
            }
        }
    }
};

class DeviceResetInst : private __IfBase {
   public:
    DeviceResetInst(PCmanagerMsg& forward_msg) : __IfBase(forward_msg) {}

   private:
    bool rst_success = false;

   public:
    std::vector<__devID> rst_false_dev;

   public:
    void forward(json& j) {
        rst_false_dev.clear();
        data_forward.type = CmdType::DEV_RESET;
        if (j.contains("params")) {
            auto params = j["params"];
            for (const auto& item : params) {
                memset(&data_forward.rst_cmd, 0, sizeof(data_forward.rst_cmd));

                std::string id = item["id"];
                parseIdString(id, data_forward.rst_cmd.id);

                if (item.contains("clip")) {
                    data_forward.rst_cmd.clip = hexStringToUint16(item["clip"]);
                }

                if (item.contains("lock")) {
                    data_forward.rst_cmd.lock =
                        static_cast<LockSta>(item["lock"]);
                }

                Log.i("rst_cmd.id: %.2X-%.2X-%.2X-%.2X",
                      data_forward.rst_cmd.id[0], data_forward.rst_cmd.id[1],
                      data_forward.rst_cmd.id[2], data_forward.rst_cmd.id[3]);
                Log.i("rst_cmd.clip: 0x%.4X", data_forward.rst_cmd.clip);
                Log.i("rst_cmd.lock: %u", data_forward.rst_cmd.lock);

                rst_success = false;
                if (__IfBase::forward()) {
                    if (!(pc_manager_msg.event.get() & RESET_SUCCESS_EVENT)) {
                        rst_success = false;
                    }
                }

                if (rst_success) {
                    Log.i(
                        "dev %.2X-%.2X-%.2X-%.2X reset success\n",
                        data_forward.rst_cmd.id[0], data_forward.rst_cmd.id[1],
                        data_forward.rst_cmd.id[2], data_forward.rst_cmd.id[3]);
                } else {
                    Log.e(
                        "dev %.2X-%.2X-%.2X-%.2X reset failed\n",
                        data_forward.rst_cmd.id[0], data_forward.rst_cmd.id[1],
                        data_forward.rst_cmd.id[2], data_forward.rst_cmd.id[3]);

                    __devID tmp;
                    memcpy(tmp.id, data_forward.rst_cmd.id, 4);
                    rst_false_dev.push_back(tmp);
                }
            }
        }
    }
};
class DeviceCtrlInst : private __IfBase {
   public:
    DeviceCtrlInst(PCmanagerMsg& forward_msg) : __IfBase(forward_msg) {}

   private:
    bool ctrl_success = false;

   public:
    void forward(json& j) {
        data_forward.type = CmdType::DEV_CTRL;
        if (j.contains("ctrl")) {
            data_forward.ctrl_cmd.ctrl = (CtrlType)j["ctrl"];

            Log.i("ctrl_cmd.ctrl: %u", data_forward.ctrl_cmd.ctrl);

            ctrl_success = false;
            if (__IfBase::forward()) {
                if ((pc_manager_msg.event.get() & CTRL_SUCCESS_EVENT))
                    ctrl_success = true;
            }

            if (ctrl_success) {
                Log.i("ctrl success\n");
            } else {
                Log.e("ctrl failed\n");
            }
        }
    }
};
class DeviceQueryInst : private __IfBase {
   public:
    DeviceQueryInst(PCmanagerMsg& forward_msg) : __IfBase(forward_msg) {}

   private:
    bool query_success = false;

   public:
    void forward(json& j) {
        data_forward.type = CmdType::DEV_QUERY;
        memset(&data_forward.query_cmd, 0, sizeof(data_forward.query_cmd));
        if (j.contains("id")) {
            if (j.contains("params")) {
                auto params = j["params"];
                if (params.contains("clip")) {
                    data_forward.query_cmd.clip = params["clip"];
                }
            }
            for (auto item : j["id"]) {
                std::string id = item;

                if (id == "*") {
                    memset(data_forward.query_cmd.id, 0xFF, 4);
                } else {
                    parseIdString(id, data_forward.query_cmd.id);
                }

                Log.i(
                    "query_cmd.id: %.2X-%.2X-%.2X-%.2X",
                    data_forward.query_cmd.id[0], data_forward.query_cmd.id[1],
                    data_forward.query_cmd.id[2], data_forward.query_cmd.id[3]);
                Log.i("query_cmd.clip: %u", data_forward.query_cmd.clip);

                query_success = false;

                if (__IfBase::forward()) {
                    if ((pc_manager_msg.event.get() & QUERY_SUCCESS_EVENT))
                        query_success = true;
                }
                if (query_success) {
                    Log.i("query success\n");
                } else {
                    Log.e("query failed\n");
                }
            }
        }
    }
};
class PCdataTransfer : public TaskClassS<PCdataTransfer_STACK_SIZE> {
   public:
    PCdataTransfer(PCdataTransferMsg& msg)
        : TaskClassS<PCdataTransfer_STACK_SIZE>("PCdataTransfer",
                                                TaskPrio_High),
          __msg(msg) {}
    void task() override {
        Log.i("PCdataTransfer_Task: Boot");
        uint8_t buffer[DMA_RX_BUFFER_SIZE];
        for (;;) {
            // 等待 DMA 完成信号
            if (xSemaphoreTake(usart1_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                uint16_t len =
                    usart1.getReceivedData(buffer, DMA_RX_BUFFER_SIZE);
                for (int i = 0; i < len; i++) {
                    __msg.data_queue.add(buffer[i]);
                }
                __msg.rx_done_sem.give();
            };
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
          __transfer_msg(pc_data_transfer_msg) {}

    void task() override {
        Log.i("PCinterface_Task: Boot");

        uint8_t buffer[PCdataTransferMsg_DATA_QUEUE_SIZE];
        uint16_t rx_cnt = 0;
        while (1) {
            __transfer_msg.rx_done_sem.take();
            while (__transfer_msg.data_queue.pop(buffer[rx_cnt], 0) == pdPASS) {
                rx_cnt++;
            }

            jsonSorting(buffer, rx_cnt);
            rx_cnt = 0;
        }
    }

   private:
    DeviceConfigInst cfg_inst;
    DevaceModeInst mode_inst;
    DeviceResetInst reset_inst;
    DeviceCtrlInst ctrl_inst;
    DeviceQueryInst query_inst;
    PCdataTransferMsg& __transfer_msg;

   private:
    void jsonSorting(uint8_t* ch, uint16_t len) {
        // 先检查JSON字符串是否有效
        if (!json::accept(ch, ch + len)) {
            Log.e("jsonSorting: Invalid JSON format");
            return;
        }

        json j = json::parse((uint8_t*)ch, ch + len, nullptr, false);
        if (j.is_discarded()) {
            Log.e("jsonSorting: json parse failed");
            return;
        }

        if (j.contains("inst")) {
            uint8_t inst = j["inst"];
            switch (inst) {
                case DEV_CONF:
                    // 设备配置解析
                    cfg_inst.forward(j);
                    break;
                case DEV_MODE:
                    // 设备模式解析
                    mode_inst.forward(j);
                    break;
                case DEV_RESET:
                    // 设备复位解析
                    reset_inst.forward(j);
                    break;
                case DEV_CTRL:
                    // 设备控制解析
                    ctrl_inst.forward(j);
                    break;
                case DEV_QUERY:
                    // 设备查询解析
                    query_inst.forward(j);
                    break;
                default:
                    break;
            }
        }
    }
};

#endif    // _MASTER_MODE_HPP_