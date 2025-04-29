#include "pc_message.hpp"

#include "protocol.hpp"

Backend2MasterMessageID ProtocolMessageForward::rx_msg_id;
using MSGID = Backend2MasterMessageID;
namespace Backend2Master {

void SlaveCfgMsg::process() {
    ProtocolMessageForward::rx_msg_id = MSGID::SLAVE_CFG_MSG;
}
void ModeCfgMsg::process() {
    ProtocolMessageForward::rx_msg_id = MSGID::MODE_CFG_MSG;
}
void RstMsg::process() { ProtocolMessageForward::rx_msg_id = MSGID::RST_MSG; }
void CtrlMsg::process() { ProtocolMessageForward::rx_msg_id = MSGID::CTRL_MSG; }
void PingCtrlMsg::process() {
    ProtocolMessageForward::rx_msg_id = MSGID::PING_CTRL_MSG;
}
}    // namespace Backend2Master

namespace Master2Backend {
void SlaveCfgMsg::process() {}
void ModeCfgMsg::process() {}
void RstMsg::process() {}
void CtrlMsg::process() {}
void PingResMsg::process() { Log.i("PingResMsg"); }
}    // namespace Master2Backend