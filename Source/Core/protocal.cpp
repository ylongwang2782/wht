#include "protocol.hpp"
uint8_t SyncMsg::mode;
uint32_t SyncMsg::timestamp;

uint8_t WriteCondInfoMsg::timeSlot;               // 为从节点分配的时隙
uint8_t WriteCondInfoMsg::interval;               // 采集间隔，单位 ms
uint16_t WriteCondInfoMsg::totalConductionNum;    // 系统中总导通检测的数量
uint16_t WriteCondInfoMsg::startConductionNum;    // 起始导通数量
uint16_t WriteCondInfoMsg::conductionNum;         // 导通检测数量

uint8_t WriteResInfoMsg::timeSlot;               // 为从节点分配的时隙
uint8_t WriteResInfoMsg::interval;               // 采集间隔，单位 ms
uint16_t WriteResInfoMsg::totalResistanceNum;    // 系统中总阻值检测的数量
uint16_t WriteResInfoMsg::startResistanceNum;    // 起始阻值数量
uint16_t WriteResInfoMsg::resistanceNum;         // 阻值检测数量

uint8_t WriteClipInfoMsg::interval;    // 采集间隔，单位 ms
uint8_t WriteClipInfoMsg::mode;        // 0：非自锁，1：自锁
uint16_t WriteClipInfoMsg::clipPin;    // 16 个卡钉激活信息，激活的位置
                                       // 1，未激活的位置 0

uint8_t InitMsg::lock;        // 锁定状态，0：未锁定，1：锁定
uint16_t InitMsg::clipLed;    // 新增卡钉灯位初始化信息

uint8_t CondInfoMsg::timeSlot;               // 为从节点分配的时隙
uint8_t CondInfoMsg::interval;               // 采集间隔，单位 ms
uint16_t CondInfoMsg::totalConductionNum;    // 系统中总导通检测的数量
uint16_t CondInfoMsg::startConductionNum;    // 起始导通数量
uint16_t CondInfoMsg::conductionNum;         // 导通检测数量

uint8_t ResInfoMsg::timeSlot;               // 为从节点分配的时隙
uint8_t ResInfoMsg::interval;               // 采集间隔，单位 ms
uint16_t ResInfoMsg::totalResistanceNum;    // 系统中总阻值检测的数量
uint16_t ResInfoMsg::startResistanceNum;    // 起始阻值数量
uint16_t ResInfoMsg::resistanceNum;         // 阻值检测数量

uint8_t ClipInfoMsg::interval;    // 采集间隔，单位 ms
uint8_t ClipInfoMsg::mode;        // 0：非自锁，1：自锁
uint16_t ClipInfoMsg::clipPin;    // 16 个卡钉激活信息，激活的位置 1，未激活的位置 0

DeviceStatus CondDataMsg::deviceStatus;              // 设备状态
uint16_t CondDataMsg::conductionLength;              // 导通数据字段长度
std::vector<uint8_t> CondDataMsg::conductionData;    // 导通数据字段

DeviceStatus ResistanceDataMsg::deviceStatus;              // 设备状态
uint16_t ResistanceDataMsg::resistanceLength;              // 阻值数据字段长度
std::vector<uint8_t> ResistanceDataMsg::resistanceData;    // 阻值数据字段

DeviceStatus ClipDataMsg::deviceStatus;     // 设备状态
uint16_t ClipDataMsg::clipData;             // 卡钉数据字段a

uint8_t InitStatusMsg::lockStatus;        // 锁状态
uint16_t InitStatusMsg::clipLed;          // 卡钉灯位初始化信息
