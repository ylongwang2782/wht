#include "protocol.hpp"

// Master2Slave 命名空间静态变量初始化
namespace Master2Slave {

/*
Sync Message
Mode	u8
Time Stamp	uint32_t
*/
uint8_t SyncMsg::mode = 0;
uint32_t SyncMsg::timestamp = 0;

/*
CondCfg Message
Time Slot	u8
Interval	u8
Total Conduction Num	u16
Start Conduction Num	u16
Conduction Num	u16
*/
uint8_t CondCfgMsg::timeSlot = 0;
uint8_t CondCfgMsg::interval = 0;
uint16_t CondCfgMsg::totalConductionNum = 0;
uint16_t CondCfgMsg::startConductionNum = 0;
uint16_t CondCfgMsg::conductionNum = 0;

/*
Resistance Config Message
Time Slot	u8
Interval	u8
Total Num	u16
Start Num	u16
Num	u16
*/
uint8_t ResCfgMsg::timeSlot = 0;
uint8_t ResCfgMsg::interval = 0;
uint16_t ResCfgMsg::totalResistanceNum = 0;
uint16_t ResCfgMsg::startResistanceNum = 0;
uint16_t ResCfgMsg::resistanceNum = 0;

/*
Clip Config Message
Interval	u8
mode	u8
Clip Pin	u16
*/
uint8_t ClipCfgMsg::interval = 0;
uint8_t ClipCfgMsg::mode = 0;
uint16_t ClipCfgMsg::clipPin = 0;

/*
Reset Message
Lock Status	u8
Clip Led	u16
*/
uint8_t RstMsg::lock = 0;
uint16_t RstMsg::clipLed = 0;

/*
Ping Req Message
Sequence Number	u16
Timestamp	uint32
*/
uint16_t PingReqMsg::sequenceNumber = 0;
uint32_t PingReqMsg::timestamp = 0;

/*
Short ID Assign Message
Short ID	uint8_t
*/
uint8_t ShortIdAssignMsg::shortId = 1;

}    // namespace Master2Slave

namespace Slave2Master {

/*
Conduction Config Message
Status	u8
Time Slot	u8
Interval	u8
Total Conduction Num	u16
Start Conduction Num	u16
Conduction Num	u16
*/
uint8_t CondCfgMsg::status = 0;
uint8_t CondCfgMsg::timeSlot = 0;
uint8_t CondCfgMsg::interval = 0;
uint16_t CondCfgMsg::totalConductionNum = 0;
uint16_t CondCfgMsg::startConductionNum = 0;
uint16_t CondCfgMsg::conductionNum = 0;

/*
Resistance Config Message
Status	u8
Time Slot	u8
Interval	u8
Total Conduction Num	u16
Start Conduction Num	u16
Conduction Num	u16
*/
uint8_t ResCfgMsg::status = 0;
uint8_t ResCfgMsg::timeSlot = 0;
uint8_t ResCfgMsg::interval = 0;
uint16_t ResCfgMsg::totalResistanceNum = 0;
uint16_t ResCfgMsg::startResistanceNum = 0;
uint16_t ResCfgMsg::resistanceNum = 0;

/*
Clip Config Message 
Status	u8
Interval	u8
mode	u8
Clip Pin	u16
*/
uint8_t ClipCfgMsg::status = 0;
uint8_t ClipCfgMsg::interval = 0;
uint8_t ClipCfgMsg::mode = 0;
uint16_t ClipCfgMsg::clipPin = 0;

/*
Reset Message
Status	u8
Lock Status	u8
Clip Led	u16
*/
uint8_t RstMsg::status = 0;
uint8_t RstMsg::lockStatus = 0;
uint16_t RstMsg::clipLed = 0;

/*
Ping Rsp Message
Sequence Number	u16
Timestamp	uint32
*/
uint16_t PingRspMsg::sequenceNumber = 0;
uint32_t PingRspMsg::timestamp = 0;

/*
Announce Message
Device ID	u32
VersionMajor	uint8_t 
VersionMinor	uint8_t 
VersionPatch	uint16_t 
*/
uint32_t AnnounceMsg::deviceId = 0;
uint8_t AnnounceMsg::versionMajor = 0;
uint8_t AnnounceMsg::versionMinor = 0;
uint16_t AnnounceMsg::versionPatch = 0;

/*
Short ID Confirm Message
status	uint8_t
Short ID	uint8_t
*/
uint8_t ShortIdConfirmMsg::status = 0;
uint8_t ShortIdConfirmMsg::shortId = 0;

}    // namespace Slave2Master

namespace Backend2Master {
/*Slave Config Message*/
uint8_t SlaveCfgMsg::slaveNum = 0;
std::vector<SlaveCfgMsg::SlaveConfig> SlaveCfgMsg::slaves;

/*Mode Config Message*/
uint8_t ModeCfgMsg::mode = 0;

/*Reset Message*/
uint8_t RstMsg::slaveNum = 0;
std::vector<RstMsg::SlaveResetConfig> RstMsg::slaves;

/*Ctrl Message*/
uint8_t CtrlMsg::runningStatus = 0;

/*
Device List Request Message
Reserve	u8	1 Byte
*/
uint8_t DeviceListReqMsg::reserve = 0;
}    // namespace Backend2Master

namespace Master2Backend {

/*Slave Config Message*/
uint8_t SlaveCfgMsg::status = 0;
uint8_t SlaveCfgMsg::slaveNum = 0;
std::vector<SlaveCfgMsg::SlaveConfig> SlaveCfgMsg::slaves;

/*Mode Config Message*/
uint8_t ModeCfgMsg::status = 0;
uint8_t ModeCfgMsg::mode = 0;

/*Reset Message*/
uint8_t RstMsg::status = 0;
uint8_t RstMsg::slaveNum = 0;
std::vector<RstMsg::SlaveResetConfig> RstMsg::slaves;

/*Ctrl Message*/
uint8_t CtrlMsg::status = 0;
uint8_t CtrlMsg::runningStatus = 0;

/*
Device List Response Message
Device Count	u8	1 Byte
Device ID	u32	4 Byte
Short ID	u8	1 Byte
Online	u8	1 Byte
VersionMajor	u8	1 Byte
VersionMinor	u8	1 Byte
VersionPatch	u16	2 Byte
*/
uint8_t DeviceListRspMsg::deviceCount = 0;
std::vector<DeviceListRspMsg::DeviceInfo> DeviceListRspMsg::devices;
}    // namespace Master2Backend

namespace Slave2Backend {

/*Conduction Data Message*/
uint16_t CondDataMsg::conductionLength = 0;
std::vector<uint8_t> CondDataMsg::conductionData;

/*Resistance Data Message*/
uint16_t ResDataMsg::resistanceLength = 0;
std::vector<uint8_t> ResDataMsg::resistanceData;

/*Clip Data Message*/
uint16_t ClipDataMsg::clipData = 0;
}    // namespace Slave2Backend