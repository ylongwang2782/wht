#include "protocol.hpp"

uint8_t Master2Slave::CondCfgMsg::timeSlot;              // 为从节点分配的时隙
uint8_t  Master2Slave::CondCfgMsg::interval;               // 采集间隔，单位 ms
uint16_t Master2Slave::CondCfgMsg::totalConductionNum;    // 系统中总导通检测的数量
uint16_t Master2Slave::CondCfgMsg::startConductionNum;    // 起始导通数量
uint16_t Master2Slave::CondCfgMsg::conductionNum;         // 导通检测数量