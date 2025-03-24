#include "protocol.hpp"

uint8_t WriteCondInfoMsg::timeSlot;               // 为从节点分配的时隙
uint8_t WriteCondInfoMsg::interval;               // 采集间隔，单位 ms
uint16_t WriteCondInfoMsg::totalConductionNum;    // 系统中总导通检测的数量
uint16_t WriteCondInfoMsg::startConductionNum;    // 起始导通数量
uint16_t WriteCondInfoMsg::conductionNum;         // 导通检测数量