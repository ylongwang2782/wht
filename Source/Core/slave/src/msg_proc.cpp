#include "msg_proc.hpp"

#include <cstdint>

#include "bsp_led.hpp"
#include "bsp_log.hpp"

extern Uart uart3;
extern MsgProc msgProc;

Harness harness;
namespace Master2Slave {
void SyncMsg::process() {
    Log.d("SyncMsg","process");
    runLed.off();
    harness.startWithCount(CondCfgMsg::totalConductionNum);
}

void CondCfgMsg::process() {
    Log.d("CondCfgMsg","process");

    // 1. REPLY
    // 1.1 构造 CondCfgMsg
    Slave2Master::CondCfgMsg condInfoMsg;
    condInfoMsg.timeSlot = timeSlot;
    condInfoMsg.interval = interval;
    condInfoMsg.totalConductionNum = totalConductionNum;
    condInfoMsg.startConductionNum = startConductionNum;
    condInfoMsg.conductionNum = conductionNum;

    harness.setPeriod(interval);
    // 初始化 Harness
    harness.init(conductionNum, totalConductionNum, startConductionNum);
    // 1.2 打包为 Packet
    uint32_t uid = UIDReader::get();
    auto condInfoPacket = PacketPacker::slave2MasterPack(condInfoMsg, uid);
    // 1.3 打包为帧
    auto condInfoFrame = FramePacker::pack(condInfoPacket);
    // 1.4 发送
    msgProc.send(condInfoFrame);
}

void ResCfgMsg::process() { Log.d("ResCfgMsg","process"); }
void ClipCfgMsg::process() { Log.d("ClipCfgMsg","process"); }
void ReadCondDataMsg::process() {
    Log.d("ReadCondDataMsg","process");
    Slave2Backend::CondDataMsg condDataMsg;
    condDataMsg.conductionData = harness.data.flatten();
    condDataMsg.conductionLength = condDataMsg.conductionData.size();
    // 2. 打包为 Packet
    uint32_t uid = UIDReader::get();
    auto condDataPacket = PacketPacker::slave2BackendPack(condDataMsg, uid);
    // 3. 打包为帧
    auto master_data = FramePacker::pack(condDataPacket);
    // 1.4 发送
    msgProc.send(master_data);
}
void ReadResDataMsg::process() { Log.d("ReadResDataMsg","process"); }
void ReadClipDataMsg::process() { Log.d("ReadClipDataMsg","process"); }
void RstMsg::process() { Log.d("RstMsg","process"); }
};    // namespace Master2Slave

namespace Slave2Master {
void Slave2Master::CondCfgMsg::process() { Log.d("CondCfgMsg","process"); }
void Slave2Master::ResCfgMsg::process() { Log.d("ResCfgMsg","process"); }
void Slave2Master::ClipCfgMsg::process() { Log.d("ClipCfgMsg","process"); }
void Slave2Master::RstMsg::process() { Log.d("RstMsg","process"); }
}    // namespace Slave2Master

namespace Backend2Master {
void Backend2Master::SlaveCfgMsg::process() { Log.d("SlaveCfgMsg"," process"); }
void Backend2Master::ModeCfgMsg::process() { Log.d("ModeCfgMsg","process"); }
void Backend2Master::RstMsg::process() { Log.d("RstMsg","process"); }
void Backend2Master::CtrlMsg::process() { Log.d("CtrlMsg","process"); }
}    // namespace Backend2Master

namespace Master2Backend {
void Master2Backend::SlaveCfgMsg::process() { Log.d("SlaveCfgMsg"," process"); }
void Master2Backend::ModeCfgMsg::process() { Log.d("ModeCfgMsg","process"); }
void Master2Backend::RstMsg::process() { Log.d("RstMsg","process"); }
void Master2Backend::CtrlMsg::process() { Log.d("CtrlMsg","process"); }
}    // namespace Master2Backend

namespace Slave2Backend {
void Slave2Backend::CondDataMsg::process() { Log.d("CondDataMsg","process"); }
void Slave2Backend::ResDataMsg::process() { Log.d("ResDataMsg","process"); }
void Slave2Backend::ClipDataMsg::process() { Log.d("ClipDataMsg","process"); }
}    // namespace Slave2Backend