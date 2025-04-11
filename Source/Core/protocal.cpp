#include "protocol.hpp"

// Master2Slave 命名空间静态变量初始化
uint8_t Master2Slave::SyncMsg::mode = 0;
uint32_t Master2Slave::SyncMsg::timestamp = 0;

uint8_t Master2Slave::CondCfgMsg::timeSlot = 0;
uint8_t Master2Slave::CondCfgMsg::interval = 0;
uint16_t Master2Slave::CondCfgMsg::totalConductionNum = 0;
uint16_t Master2Slave::CondCfgMsg::startConductionNum = 0;
uint16_t Master2Slave::CondCfgMsg::conductionNum = 0;

uint8_t Master2Slave::ResCfgMsg::timeSlot = 0;
uint8_t Master2Slave::ResCfgMsg::interval = 0;
uint16_t Master2Slave::ResCfgMsg::totalResistanceNum = 0;
uint16_t Master2Slave::ResCfgMsg::startResistanceNum = 0;
uint16_t Master2Slave::ResCfgMsg::resistanceNum = 0;

uint8_t Master2Slave::ClipCfgMsg::interval = 0;
uint8_t Master2Slave::ClipCfgMsg::mode = 0;
uint16_t Master2Slave::ClipCfgMsg::clipPin = 0;

uint8_t Master2Slave::RstMsg::lock = 0;
uint16_t Master2Slave::RstMsg::clipLed = 0;

// Slave2Master 命名空间静态变量初始化
uint8_t Slave2Master::CondCfgMsg::status = 0;
uint8_t Slave2Master::CondCfgMsg::timeSlot = 0;
uint8_t Slave2Master::CondCfgMsg::interval = 0;
uint16_t Slave2Master::CondCfgMsg::totalConductionNum = 0;
uint16_t Slave2Master::CondCfgMsg::startConductionNum = 0;
uint16_t Slave2Master::CondCfgMsg::conductionNum = 0;

uint8_t Slave2Master::ResCfgMsg::status = 0;
uint8_t Slave2Master::ResCfgMsg::timeSlot = 0;
uint8_t Slave2Master::ResCfgMsg::interval = 0;
uint16_t Slave2Master::ResCfgMsg::totalResistanceNum = 0;
uint16_t Slave2Master::ResCfgMsg::startResistanceNum = 0;
uint16_t Slave2Master::ResCfgMsg::resistanceNum = 0;

uint8_t Slave2Master::ClipCfgMsg::status = 0;
uint8_t Slave2Master::ClipCfgMsg::interval = 0;
uint8_t Slave2Master::ClipCfgMsg::mode = 0;
uint16_t Slave2Master::ClipCfgMsg::clipPin = 0;

uint8_t Slave2Master::RstMsg::status = 0;
uint8_t Slave2Master::RstMsg::lockStatus = 0;
uint16_t Slave2Master::RstMsg::clipLed = 0;

// Backend2Master 命名空间静态变量初始化
uint8_t Backend2Master::SlaveCfgMsg::slaveNum = 0;
std::vector<Backend2Master::SlaveCfgMsg::SlaveConfig> Backend2Master::SlaveCfgMsg::slaves;

uint8_t Backend2Master::ModeCfgMsg::mode = 0;

uint8_t Backend2Master::RstMsg::slaveNum = 0;
std::vector<Backend2Master::RstMsg::SlaveResetConfig> Backend2Master::RstMsg::slaves;

// Master2Backend 命名空间静态变量初始化
uint8_t Master2Backend::SlaveCfgMsg::status = 0;
uint8_t Master2Backend::SlaveCfgMsg::slaveNum = 0;
std::vector<Master2Backend::SlaveCfgMsg::SlaveConfig> Master2Backend::SlaveCfgMsg::slaves;

uint8_t Master2Backend::ModeCfgMsg::status = 0;
uint8_t Master2Backend::ModeCfgMsg::mode = 0;

uint8_t Master2Backend::RstMsg::status = 0;
uint8_t Master2Backend::RstMsg::slaveNum = 0;
std::vector<Master2Backend::RstMsg::SlaveResetConfig> Master2Backend::RstMsg::slaves;

uint8_t Master2Backend::CtrlMsg::status = 0;
uint8_t Master2Backend::CtrlMsg::runningStatus = 0;

// Slave2Backend 命名空间静态变量初始化
uint16_t Slave2Backend::CondDataMsg::conductionLength = 0;
std::vector<uint8_t> Slave2Backend::CondDataMsg::conductionData;

uint16_t Slave2Backend::ResDataMsg::resistanceLength = 0;
std::vector<uint8_t> Slave2Backend::ResDataMsg::resistanceData;

uint16_t Slave2Backend::ClipDataMsg::clipData = 0;