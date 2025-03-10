# Wireless Conduction Testing System
1. 通信双方：Master node <-> Slave Node

# Frame Format
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Frame Delimiter | uint8 | 2 Byte | 0xAB、0xCD |
| Slot | u8 | 1 Byte | 0x00：时隙 0<br/>0x01：时隙 1<br/>…… |
| Packet ID | u8 | 1 Byte |  |
| Fragments Sequence | u8 | 1 Byte | 帧分片的序号 |
| More FragmentsFlag | u8 | 1 Byte | 0：无更多分片<br/>1：有更多分片 |
| Data Length | u16 | 2 Byte | 数据长度 |
| Data Payload | u8 | Payload Size | 帧实际负载 |


| Packet ID | Value | 描述 |
| --- | --- | --- |
| Master2Slave | 0x00 |  |
| Slave2Master | 0x01 |  |


## Master2Slave Packet
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Message ID | uint8_t  | 1 Byte |  |
| Target ID | uint8_t | 4 Byte | 目标 ID <br/>FFFFFFFF：广播 |
| Payload |  | payload size |  |


| Message ID | Value | 描述 |
| --- | --- | --- |
| SYNC | 0x00 | 同步 |
| WRITE_CONDUCTION_INFO | 0x01 | 配置导通 |
| WRITE_RESISTANCE_INFO | 0x02 | 配置阻值 |
| WRITE_CLIP_INFO | 0x03 | 配置卡钉 |
| READ_DATA | 0x04 | 请求数据 |
| LOCK | 0x05 | 解锁 |
| READ_CLIP_NUM | 0x06 | 查询卡钉板数量 |


### Sync Message
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Time Stamp | uint32_t | 4 Byte | 时间戳 |


### Write Conduction Info Message
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Mode | uint8_t | 1 Byte | 0：导通检测模式 |
| Time Slot | uint8_t | 1 Byte | 为从节点分配的时隙 |
| Total Conduction Num | uint16_t | 2 Byte | 系统中总导通检测的数量 |
| Start Conduction Num | uint16_t | 2 Byte | 起始导通数量 |
| Conduction Num | uint16_t | 2 Byte | 导通检测数量 |


### Write Resistance Info Message
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Mode | uint8_t | 1 Byte | 1：阻值检测模式 |
| Resistance Num | uint8_t | 1 Byte | 阻值检测数量 |


### Write Clip Info MessageP
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Mode | uint8_t | 1 Byte | 2：卡钉检测模式 |
| Clip Num | uint8_t | 1 Byte | 卡钉板检测数量 |


### Read Data Message
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Type | uint8_t | 1 Byte | 0：Read 导通数据<br/>1：Read 阻值数据<br/>2：Read 卡钉数据 |


### Lock Message
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Lock | uint8_t | 1 Byte | 0：解锁<br/>1：上锁 |


### Read Clip Num Message
Null

## Slave2Master Packet
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Message ID | uint8_t  | 1 Byte |  |
| Payload |  | Payload Size |  |


| Message ID | Value | 描述 |
| --- | --- | --- |
| CONDUCTION_INFO | 0x00 | 导通信息 |
| RESISTANCE_INFO | 0x01 | 阻值信息 |
| CLIP_INFO | 0x02 | 卡钉信息 |
| CONDUCTION_DATA | 0x03 | 导通数据 |
| RESISTANCE_DATA | 0x04 | 阻值数据 |
| CLIP_DATA | 0x05 | 卡钉数据 |
| LOCK_STATUS | 0x06 | 锁状态 |
| CLIP_NUM | 0x07 | 卡钉数量 |


### Conduction Info Message
NULL

### Resistance Info Message
NULL

### Clip Info Message
NULL

### Conduction Data Message
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Device Status | DeviceStatus | 2 Byte | 状态字 |
| Conduction Length | uint16_t | 2 Byte | 导通数据字段长度 |
| Conduction Data | uint8_t | Conduction Length | 导通数据 |


```c
typedef struct {
    uint16_t colorSensor : 1;
    uint16_t sleeveLimit : 1; 
    uint16_t electromagnetUnlockButton : 1; 
    uint16_t batteryLowPowerAlarm : 1;  
    uint16_t pressureSensor : 1;  
    uint16_t electromagneticLock1 : 1; 
    uint16_t electromagneticLock2 : 1; 
    uint16_t accessory1 : 1; 
    uint16_t accessory2 : 1; 
    uint16_t res : 7;    
} DeviceStatus;
```

| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Color Sensor | 颜色传感器匹配状态 | bit | 0，颜色不匹配或无传感器<br/>1，颜色匹配 |
| Sleeve Limit | 限位开关状态 | bit | 0，探针断开；<br/>1，探针导通 |
| Electromagnet Unlock Button | 电磁锁解锁按钮 | bit | 0，按钮未按下；<br/>1，按钮按下 |
| Battery Low Alarm | 电池低电量报警 | bit | 0，电池正常；<br/>1，电池低电量 |
| Pressure Sensor | 气压传感器 | bit | 0，气压传感器断开；<br/>1，气压传感器触发 |
| Electromagnetic Lock1 | 电磁锁1状态 | bit | 0，电磁锁1未锁；<br/>1，电磁锁1上锁 |
| Electromagnetic Lock2 | 电磁锁 2 状态 | bit | 0，电磁锁 2 未锁；<br/>1，电磁锁 2 上锁 |
| Accessory1 | 辅件 1 状态 | bit | 0，辅件1不存在；<br/>1，辅件1存在 |
| Accessory2 | 辅件 2 状态  | bit | 0，辅件 2 不存在；<br/>1，辅件 2 存在 |


### Resistance Data Message
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Device Status | DeviceStatus | 2 Byte | 状态字 |
| Resistance Length | uint8_t | 1 Byte | 阻值数据长度 |
| Resistance Data | uint8_t[ ] | Resistance Length | 阻值数据 |


### Clip Data Message
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Device Status | DeviceStatus | 2 Byte | 状态字 |
| Clip Num | uint8_t | 1 Byte | 卡钉板数量 |
| Clip Data | uint16_t[ ] | `Clip Length`x 2 Bytes | 卡钉数据 |


### Clip Num Message
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Device Status | DeviceStatus | 2 Byte | 状态字 |
| Clip Length | uint8_t | 1 Byte | 卡钉数据字段长度 |
| Clip Data | uint8_t[ ] | Clip Length | 卡钉数据 |


### Lock Status Message
| Data | Type | Length | Description |
| --- | --- | --- | --- |
| Lock Status | uint8 | 1 Byte | 0，已解锁<br/>1，已上锁 |


# Document Version
| Version | Date | Description |
| --- | --- | --- |
| 1.0 | 20250103 | + 初次发布 |
| 1.1 | 20250225 | + 优化了帧、包和消息描述 |
| 1.2 | 20250306 | + 删除了 Harness Config 消息<br/>+ 新增 Conduction Config Message<br/>+ 新增 Resistance Config Message<br/>+ 新增 CLip Data Message<br/>+ 新增 Conduction Data Message<br/>+ 新增 Resistance Data Message<br/>+ 新增 CLip Data Message<br/>+ 删除 Command Packet，Command Reply Packet<br/>+ 新增 Master2Slave, Slave2Master Packet 极其描述<br/>+ 新增 Packet ID List 和 Message ID List |


