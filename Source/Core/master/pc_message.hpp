#ifndef __PC_MESSAGE_HPP
#define __PC_MESSAGE_HPP

#include <sys/types.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "bsp_log.hpp"
#include "forward.hpp"
#include "master_cfg.hpp"
#include "master_def.hpp"
#include "protocol.hpp"

class __PcMessageBase : public DataForwardBase {
   public:
    __PcMessageBase(PCmanagerMsg& msg) : DataForwardBase(msg) {};
};
class SlaveConfig : private __PcMessageBase {
   public:
    SlaveConfig(PCmanagerMsg& msg) : __PcMessageBase(msg) {};

   private:
    CfgCmd cfg_cmd;
    bool is_success;
    Master2Backend::SlaveCfgMsg::SlaveConfig slave_cfg;
    Master2Backend::SlaveCfgMsg rsp_msg;
    uint16_t index = 0;
    uint16_t slave_num = 0;
    // std::vector<Master2Backend::SlaveCfgMsg::SlaveConfig> slaves_dev;

   public:
    std::vector<uint8_t> forward() {
        is_success = true;
        index = 0;
        data_forward.type = DEV_CONF;
        slave_num = Backend2Master::SlaveCfgMsg::slaves.size();
        rsp_msg.slaves.reserve(slave_num);
        rsp_msg.slaves.clear();
        memset(&cfg_cmd, 0, sizeof(cfg_cmd));
        for (auto dev : Backend2Master::SlaveCfgMsg::slaves) {
            cfg_cmd.totalHarnessNum += dev.conductionNum;
        }
        for (auto dev : Backend2Master::SlaveCfgMsg::slaves) {
            cfg_cmd.is_last_dev = (++index == slave_num);

            cfg_cmd.slave_dev_num = slave_num;

            memcpy(cfg_cmd.id, &dev.id, sizeof(dev.id));
            // 计算开始引脚
            cfg_cmd.startHarnessNum += cfg_cmd.cond;

            cfg_cmd.Z = dev.resistanceNum;
            cfg_cmd.cond = dev.conductionNum;

            cfg_cmd.clip_exist = true;
            cfg_cmd.clip_pin = dev.clipStatus;
            cfg_cmd.clip_mode = dev.clipMode;

            data_forward.cfg_cmd = cfg_cmd;

            slave_cfg.id = dev.id;
            slave_cfg.clipMode = dev.clipMode;
            slave_cfg.clipStatus = dev.clipStatus;
            slave_cfg.conductionNum = dev.conductionNum;
            slave_cfg.resistanceNum = dev.resistanceNum;
            rsp_msg.slaves.push_back(slave_cfg);
            Log.i("SlaveConfig","0x%.8X  configing... ", slave_cfg.id);
            // Log.i("SlaveConfig","conductionNum = %u", cfg_cmd.cond);
            // Log.i("SlaveConfig","resistanceNum = %u", cfg_cmd.Z);
            // Log.i("SlaveConfig","clipMode = %u", cfg_cmd.clip_mode);
            // Log.i("SlaveConfig","clipStatus = %u", cfg_cmd.clip_pin);
            Log.i("SlaveConfig","startHarnessNum = %u",
                  cfg_cmd.startHarnessNum);

            if (__PcMessageBase::forward()) {
                if ((pc_manager_msg.event.get() & CONFIG_SUCCESS_EVENT)) {
                    Log.i("SlaveConfig","%.2X-%.2X-%.2X-%.2X config success\n",
                          cfg_cmd.id[0], cfg_cmd.id[1], cfg_cmd.id[2],
                          cfg_cmd.id[3]);

                } else {
                    Log.e("SlaveConfig","%.2X-%.2X-%.2X-%.2X config failed\n",
                          cfg_cmd.id[0], cfg_cmd.id[1], cfg_cmd.id[2],
                          cfg_cmd.id[3]);
                    is_success = false;
                }
            }
        }

        rsp_msg.slaveNum = slave_num;
        rsp_msg.status = !is_success;
        auto rsp_packet = PacketPacker::master2BackendPack(rsp_msg);
        return FramePacker::pack(rsp_packet);
    }
};

class ModeConfig : private __PcMessageBase {
   public:
    ModeConfig(PCmanagerMsg& msg) : __PcMessageBase(msg) {};

   private:
    ModeCmd mode_cmd;
    bool is_success;
    Master2Backend::ModeCfgMsg rsp_msg;

   public:
    std::vector<uint8_t> forward() {
        is_success = true;
        data_forward.type = DEV_MODE;
        mode_cmd.mode = (SysMode)Backend2Master::ModeCfgMsg::mode;
        data_forward.mode_cmd = mode_cmd;
        Log.i("ModeConfig","mode = %u", mode_cmd.mode);
        if (__PcMessageBase::forward()) {
            if ((pc_manager_msg.event.get() & MODE_SUCCESS_EVENT)) {
                Log.i("ModeConfig","config success");
            } else {
                Log.e("ModeConfig","config failed");
                is_success = false;
            }
        }
        rsp_msg.status = !is_success;
        rsp_msg.mode = mode_cmd.mode;
        auto rsp_packet = PacketPacker::master2BackendPack(rsp_msg);
        return FramePacker::pack(rsp_packet);
    }
};

class ResetConfig : private __PcMessageBase {
   public:
    ResetConfig(PCmanagerMsg& msg) : __PcMessageBase(msg) {};

   private:
    ResetCmd rst_cmd;
    bool is_success;
    Master2Backend::RstMsg rsp_msg;
    Master2Backend::RstMsg::SlaveResetConfig rst_cfg;
    uint16_t slave_num;
    // std::vector<Master2Backend::RstMsg::SlaveResetConfig> slaves_dev;

   public:
    std::vector<uint8_t> forward() {
        is_success = true;
        data_forward.type = DEV_RESET;
        slave_num = Backend2Master::RstMsg::slaves.size();
        rsp_msg.slaves.reserve(slave_num);
        rsp_msg.slaves.clear();
        memset(&rst_cmd, 0, sizeof(rst_cmd));
        for (auto dev : Backend2Master::RstMsg::slaves) {
            memcpy(rst_cmd.id, &dev.id, sizeof(dev.id));
            rst_cmd.clip = dev.clipStatus;
            rst_cmd.lock = (LockSta)dev.lock;
            data_forward.rst_cmd = rst_cmd;

            rst_cfg.id = dev.id;
            rst_cfg.clipStatus = dev.clipStatus;
            rst_cfg.lock = dev.lock;
            rsp_msg.slaves.push_back(rst_cfg);

            Log.i("ResetConfig","0x%.8X  reseting... ", rst_cmd.id);
            if (__PcMessageBase::forward()) {
                if ((pc_manager_msg.event.get() & RESET_SUCCESS_EVENT)) {
                    Log.i("ResetConfig"," 0x%.8X reset success\n", rst_cmd.id);
                } else {
                    Log.e("ResetConfig"," 0x%.8X reset failed\n", rst_cmd.id);
                    is_success = false;
                }
            }
        }
        rsp_msg.slaveNum = slave_num;
        rsp_msg.status = !is_success;
        auto rsp_packet = PacketPacker::master2BackendPack(rsp_msg);
        return FramePacker::pack(rsp_packet);
    }
};

class ControlConfig : private __PcMessageBase {
   public:
    ControlConfig(PCmanagerMsg& msg) : __PcMessageBase(msg) {};

   private:
    CtrlCmd ctrl_cmd;
    bool is_success;
    Master2Backend::CtrlMsg rsp_msg;

   public:
    std::vector<uint8_t> forward() {
        is_success = true;
        data_forward.type = DEV_CTRL;
        ctrl_cmd.ctrl = (CtrlType)Backend2Master::CtrlMsg::runningStatus;
        data_forward.ctrl_cmd = ctrl_cmd;
        Log.i("ControlConfig","runningStatus = %u", ctrl_cmd.ctrl);
        if (__PcMessageBase::forward()) {
            if ((pc_manager_msg.event.get() & CTRL_SUCCESS_EVENT)) {
                Log.i("ControlConfig","config success\n");
            } else {
                Log.e("ControlConfig","config failed\n");
                is_success = false;
            }
        }
        rsp_msg.runningStatus = ctrl_cmd.ctrl;
        rsp_msg.status = !is_success;
        auto rsp_packet = PacketPacker::master2BackendPack(rsp_msg);
        return FramePacker::pack(rsp_packet);
    }
};

class ProtocolMessageForward {
   public:
    ProtocolMessageForward(PCmanagerMsg& _msg)
        : slave_config(_msg),
          mode_config(_msg),
          reset_config(_msg),
          control_config(_msg) {};
    ~ProtocolMessageForward() {};

   public:
    static Backend2MasterMessageID rx_msg_id;

   private:
    SlaveConfig slave_config;
    ModeConfig mode_config;
    ResetConfig reset_config;
    ControlConfig control_config;
    FrameParser frame_parser;
    std::vector<uint8_t> rsp_packet;

   public:
    const std::vector<uint8_t> forward(std::vector<uint8_t> raw_data) {
        auto msg = frame_parser.parse(raw_data);
        if (msg != nullptr) {
            // 处理解析后的数据
            msg->process();

            switch (rx_msg_id) {
                case Backend2MasterMessageID::SLAVE_CFG_MSG: {
                    rsp_packet = slave_config.forward();

                    break;
                }
                case Backend2MasterMessageID::MODE_CFG_MSG: {
                    rsp_packet = mode_config.forward();
                    break;
                }
                case Backend2MasterMessageID::RST_MSG: {
                    rsp_packet = reset_config.forward();
                    break;
                }
                case Backend2MasterMessageID::CTRL_MSG: {
                    rsp_packet = control_config.forward();
                    break;
                }
            }
        } else {
            Log.e("SlaveManager","parse failed");
            rsp_packet.clear();
        }
        return rsp_packet;
    }
};

#endif