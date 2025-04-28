#include "protocol.hpp"

// Master2Slave 命名空间静态变量初始化
namespace Master2Slave {
uint8_t SyncMsg::mode = 0;
uint32_t SyncMsg::timestamp = 0;

uint8_t CondCfgMsg::timeSlot = 0;
uint8_t CondCfgMsg::interval = 0;
uint16_t CondCfgMsg::totalConductionNum = 0;
uint16_t CondCfgMsg::startConductionNum = 0;
uint16_t CondCfgMsg::conductionNum = 0;

uint8_t ResCfgMsg::timeSlot = 0;
uint8_t ResCfgMsg::interval = 0;
uint16_t ResCfgMsg::totalResistanceNum = 0;
uint16_t ResCfgMsg::startResistanceNum = 0;
uint16_t ResCfgMsg::resistanceNum = 0;

uint8_t ClipCfgMsg::interval = 0;
uint8_t ClipCfgMsg::mode = 0;
uint16_t ClipCfgMsg::clipPin = 0;

uint8_t RstMsg::lock = 0;
uint16_t RstMsg::clipLed = 0;

uint16_t PingReqMsg::sequenceNumber = 0;
uint32_t PingReqMsg::timestamp = 0;
}    // namespace Master2Slave

namespace Slave2Master {

uint8_t CondCfgMsg::status = 0;
uint8_t CondCfgMsg::timeSlot = 0;
uint8_t CondCfgMsg::interval = 0;
uint16_t CondCfgMsg::totalConductionNum = 0;
uint16_t CondCfgMsg::startConductionNum = 0;
uint16_t CondCfgMsg::conductionNum = 0;

uint8_t ResCfgMsg::status = 0;
uint8_t ResCfgMsg::timeSlot = 0;
uint8_t ResCfgMsg::interval = 0;
uint16_t ResCfgMsg::totalResistanceNum = 0;
uint16_t ResCfgMsg::startResistanceNum = 0;
uint16_t ResCfgMsg::resistanceNum = 0;

uint8_t ClipCfgMsg::status = 0;
uint8_t ClipCfgMsg::interval = 0;
uint8_t ClipCfgMsg::mode = 0;
uint16_t ClipCfgMsg::clipPin = 0;

uint8_t RstMsg::status = 0;
uint8_t RstMsg::lockStatus = 0;
uint16_t RstMsg::clipLed = 0;
}    // namespace Slave2Master

namespace Backend2Master {

uint8_t SlaveCfgMsg::slaveNum = 0;
std::vector<SlaveCfgMsg::SlaveConfig> SlaveCfgMsg::slaves;

uint8_t ModeCfgMsg::mode = 0;

uint8_t RstMsg::slaveNum = 0;
std::vector<RstMsg::SlaveResetConfig> RstMsg::slaves;

uint8_t CtrlMsg::runningStatus = 0;
}    // namespace Backend2Master

// Master2Backend 命名空间静态变量初始化
namespace Master2Backend {

uint8_t SlaveCfgMsg::status = 0;
uint8_t SlaveCfgMsg::slaveNum = 0;
std::vector<SlaveCfgMsg::SlaveConfig> SlaveCfgMsg::slaves;

uint8_t ModeCfgMsg::status = 0;
uint8_t ModeCfgMsg::mode = 0;

uint8_t RstMsg::status = 0;
uint8_t RstMsg::slaveNum = 0;
std::vector<RstMsg::SlaveResetConfig> RstMsg::slaves;

uint8_t CtrlMsg::status = 0;
uint8_t CtrlMsg::runningStatus = 0;
}    // namespace Master2Backend

// Slave2Backend 命名空间静态变量初始化
namespace Slave2Backend {

uint16_t CondDataMsg::conductionLength = 0;
std::vector<uint8_t> CondDataMsg::conductionData;

uint16_t ResDataMsg::resistanceLength = 0;
std::vector<uint8_t> ResDataMsg::resistanceData;

uint16_t ClipDataMsg::clipData = 0;
}    // namespace Slave2Backend