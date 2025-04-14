## 示例消息
| Packet Type | Msg Type | Type Value | Context |
| --- | --- | --- | --- |
| Master2Slave | SYNC_MSG | 0x00 | AB CD 00 00 00 00 0A 00 00 37 32 48 5B 00 12 34 56 78  |
| | CONDUCTION_CFG_MSG | 0x10 | AB CD 00 00 00 00 0D 00 10 37 32 48 5B 01 0A 04 00 00 00 02 00 |
| | RESISTANCE_CFG_MSG | 0x11 | AB CD 00 00 00 00 0D 00 11 37 32 48 5B 01 0A 04 00 00 00 02 00 |
| | CLIP_CFG_MSG | 0x12 | AB CD 00 00 00 00 09 00 12 37 32 48 5B 0A 01 03 00 |
| | READ_COND_DATA_MSG | 0x20 | AB CD 00 00 00 00 06 00 20 37 32 48 5B 00 |
| | READ_RES_DATA_MSG | 0x21 | AB CD 00 00 00 00 06 00 21 37 32 48 5B 00 |
| | READ_CLIP_DATA_MSG | 0x22 | AB CD 00 00 00 00 06 00 22 37 32 48 5B 00 |
| | RST_MSG | 0x30 | AB CD 00 00 00 00 08 00 30 37 32 48 5B 00 0F 00 |
| Slave2Master | CONDUCTION_CFG_MSG | 0x00 | AB CD 00 01 00 00 0E 00 00 37 32 48 5B 00 01 0A 04 00 00 00 02 00 |
|  | RESISTANCE_CFG_MSG | 0x01 | AB CD 00 01 00 00 0E 00 01 37 32 48 5B 00 01 0A 04 00 00 00 02 00 |
|  | CLIP_CFG_MSG | 0x02 | AB CD 00 01 00 00 0A 00 02 37 32 48 5B 00 0A 01 AB CD |
|  | RST_MSG | 0x03 | AB CD 00 01 00 00 09 00 03 37 32 48 5B 00 01 AB CD |
| Backend2Master | SLAVE_CFG_MSG | 0x00 | AB CD 00 02 00 00 14 00 00 02 37 32 48 5B 02 00 00 00 00 37 32 48 55 02 00 00 00 00 |
| | MODE_CFG_MSG | 0x01 | AB CD 00 02 00 00 02 00 01 01 |
| | RST_MSG | 0x02 | AB CD 00 02 00 00 10 00 02 02 37 32 48 5B 01 12 34 37 32 48 55 01 12 34 |
| | CTRL_MSG | 0x03 | AB CD 00 02 00 00 02 00 03 01 |
| Master2Backend | SLAVE_CFG_MSG | 0x00 | AB CD 00 03 00 00 15 00 00 00 02 37 32 48 5B 02 00 00 00 00 37 32 48 55 02 00 00 00 00 |
| | MODE_CFG_MSG | 0x01 | AB CD 00 03 00 00 03 00 01 00 01 |
| | RST_MSG | 0x02 | AB CD 00 03 00 00 11 00 02 00 02 37 32 48 5B 01 12 34 37 32 48 55 01 12 34 |
| | CTRL_MSG | 0x03 | AB CD 00 03 00 00 03 00 03 00 01 |
| Slave2Backend | CONDUCTION_DATA_MSG | 0x00 | AB CD 00 04 00 00 06 00 00 55 FF 01 00 24 |
| | RESISTANCE_DATA_MSG | 0x01 | AB CD 00 04 00 00 07 00 01 55 FF 02 00 78 78 |
| | CLIP_DATA_MSG | 0x02 | AB CD 00 04 00 00 05 00 02 55 FF 78 90 |


## 📤 Backend2Master 协议消息说明
本 Packet 用于后台（Backend）与主控（Master）之间的通信，消息以帧形式发送，每帧包含**帧头、长度、命令类型**与**数据内容**，并按照不同类型指令划分消息格式。

---

### 1️⃣ SLAVE_CFG_MSG（0x00） — 从机配置消息
用于配置所有从机的相关参数，如导通检测通道、阻值检测、卡钉检测等。

#### 🧱 数据结构
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Slave Num | `u8` | 1 Byte | 从机数量 |
| Slave X ID | `u8` | 4 Byte | 从机唯一 ID |
| Conduction Num | `u8` | 1 Byte | 导通检测通道数量 |
| Resistance Num | `u8` | 1 Byte | 阻值检测通道数量 |
| Clip Mode | `u8` | 1 Byte | 卡钉检测模式 |
| Clip Status | `u16` | 2 Byte | 卡钉初始化状态（位图） |
| ... | | | 后续从机以此结构重复添加 |


---

#### 🧪 示例解析
示例原始数据：

```plain
AB CD 00 02 00 00 14 00 00 
02 37 32 48 5B 02 00 00 00 00 
37 32 48 55 02 00 00 00 00
```

提取数据部分：

```plain
02 37 32 48 5B 02 00 00 00 00 
37 32 48 55 02 00 00 00 00
```

解析如下：

| 字段名称 | 数据值（HEX） | 含义 |
| --- | --- | --- |
| Slave Num | `02` | 共 2 个从机 |
| Slave 0 ID | `37 32 48 5B` | ID = `0x3732485B` |
| Conduction Num | `02` | 2 路导通检测 |
| Resistance Num | `00` | 无阻值检测 |
| Clip Mode | `00` | 模式 0 |
| Clip Status | `00 00` | 无初始化卡钉 |
| Slave 1 ID | `37 32 48 55` | ID = `0x37324855` |
| Conduction Num | `02` | 2 路导通检测 |
| Resistance Num | `00` | 无阻值检测 |
| Clip Mode | `00` | 模式 0 |
| Clip Status | `00 00` | 无初始化卡钉 |


---

### 2️⃣ MODE_CFG_MSG（0x01） — 模式设置消息
用于设置整个系统的运行模式，如启动/停止等。

#### 🧱 数据结构
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Mode | `u8` | 1 Byte | 0：导通检测模式<br/>1：阻值检测模式<br/>2：卡钉检测模式 |


---

#### 🧪 示例解析
示例原始数据：

```plain
AB CD 00 02 00 00 02 00 01 01
```

提取数据内容部分：

```plain
01
```

+ `Mode = 01` → 系统进入阻值检测模式

---

### 3️⃣ RST_MSG（0x02） — 复位消息
用于设置从机锁定状态以及复位指定卡钉孔位。

#### 🧱 数据结构
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Slave Num | `u8` | 1 Byte | 从机数量 |
| Slave X ID | `u8` | 4 Byte | 从机唯一 ID |
| Lock | `u8` | 1 Byte | 1：上锁，0：解锁 |
| Clip Status | `u16` | 2 Byte | 需要复位的卡钉状态 |
| ... | | | 重复上述结构以支持多个从机 |


---

#### 🧪 示例解析
示例原始数据：

```plain
AB CD 00 02 00 00 10 00 02 02 
37 32 48 5B 01 12 34 
37 32 48 55 01 12 34
```

提取数据内容部分：

```plain
02 37 32 48 5B 01 12 34 37 32 48 55 01 12 34
```

#### 解析如下：
| 字段名称 | 数据值 | 说明 |
| --- | --- | --- |
| Slave Num | `02` | 共 2 个从机 |
| Slave 0 ID | `3732485B` | 从机 ID |
| Lock | `01` | 上锁 |
| Clip Status | `12 34` | 复位位图（0x1234） |
| Slave 1 ID | `37324855` | 从机 ID |
| Lock | `01` | 上锁 |
| Clip Status | `12 34` | 复位位图（0x1234） |


---

### 4️⃣ CTRL_MSG（0x03） — 运行控制消息
用于控制系统整体运行状态的开关。

#### 🧱 数据结构
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Running Status | `u8` | 1 Byte | 0：停止；1：开启 |


---

#### 🧪 示例解析
示例原始数据：

```plain
AB CD 00 02 00 00 02 00 03 01
```

提取数据内容部分：

```plain
01
```

+ `Running Status = 01` → 控制系统进入运行状态

## 📤 Master2Backend 协议消息说明
以下为主控设备（Master）向后台系统（Backend）上报的协议帧结构。用于反馈配置状态、当前模式、控制命令执行结果等。

---

### 1️⃣ SLAVE_CFG_MSG（0x00） — 从机配置响应消息
主控回应后台配置命令的执行结果，并上报当前有效的从机配置信息。

#### 🧱 数据结构
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Status | `u8` | 1 Byte | 状态码，0 表示成功 |
| Slave Num | `u8` | 1 Byte | 从机数量 |
| Slave X ID | `u8` | 4 Byte | 从机唯一 ID |
| Conduction Num | `u8` | 1 Byte | 导通检测通道数量 |
| Resistance Num | `u8` | 1 Byte | 阻值检测通道数量 |
| Clip Mode | `u8` | 1 Byte | 卡钉检测模式 |
| Clip Status | `u16` | 2 Byte | 卡钉初始化状态（位图） |
| ... | | | 重复上述结构 |


---

#### 🧪 示例解析
原始帧数据：

```plain
AB CD 00 03 00 00 15 00 00 00 02 
37 32 48 5B 02 00 00 00 00 
37 32 48 55 02 00 00 00 00
```

数据字段如下：

| 字段 | HEX值 | 含义 |
| --- | --- | --- |
| Status | `00` | 成功响应 |
| Slave Num | `02` | 2 个从机 |
| Slave 0 ID | `3732485B` | 从机 0 ID |
| Conduction Num | `02` | 2 路导通检测 |
| Resistance Num | `00` | 无阻值检测 |
| Clip Mode | `00` | 卡钉检测模式 0 |
| Clip Status | `00 00` | 无卡钉初始化 |
| Slave 1 ID | `37324855` | 从机 1 ID |
| Conduction Num | `02` | 2 路导通检测 |
| Resistance Num | `00` | 无阻值检测 |
| Clip Mode | `00` | 模式 0 |
| Clip Status | `00 00` | 无卡钉初始化 |


---

### 2️⃣ MODE_CFG_MSG（0x01） — 模式响应消息
主控上报当前模式设置的执行结果。

#### 🧱 数据结构
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Status | `u8` | 1 | 响应状态码 |
| Mode | `u8` | 1 | 当前检测模式： |
| | | | 0：导通检测模式 |
| | | | 1：阻值检测模式 |
| | | | 2：卡钉检测模式 |


---

#### 🧪 示例解析
原始帧数据：

```plain
AB CD 00 03 00 00 03 00 01 00 01
```

数据字段：

| 字段 | HEX值 | 含义 |
| --- | --- | --- |
| Status | `00` | 成功 |
| Mode | `01` | 当前为阻值检测模式 |


---

### 3️⃣ RST_MSG（0x02） — 卡钉复位响应消息
主控响应复位命令并反馈卡钉孔位及锁定状态。

#### 🧱 数据结构
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Status | `u8` | 1 Byte | 响应状态码（0 成功） |
| Slave Num | `u8` | 1 Byte | 从机数量 |
| Slave X ID | `u8` | 4 Byte | 从机 ID |
| Clip Status | `u16` | 2 Byte | 已复位的卡钉位图 |
| Lock | `u8` | 1 Byte | 当前锁定状态（1：上锁） |
| ... | | | 多个从机重复该结构 |


---

#### 🧪 示例解析
原始帧数据：

```plain
AB CD 00 03 00 00 11 00 02 00 02 
37 32 48 5B 01 12 34 
37 32 48 55 01 12 34
```

数据字段如下：

| 字段 | HEX值 | 含义 |
| --- | --- | --- |
| Status | `00` | 响应成功 |
| Slave Num | `02` | 2 个从机 |
| Slave 0 ID | `3732485B` | 从机 ID |
| Clip Status | `12 34` | 位图 0x1234 |
| Lock | `01` | 当前为上锁状态 |
| Slave 1 ID | `37324855` | 从机 ID |
| Clip Status | `12 34` | 位图 0x1234 |
| Lock | `01` | 当前为上锁状态 |


---

### 4️⃣ CTRL_MSG（0x03） — 控制响应消息
用于反馈运行控制指令的执行结果。

#### 🧱 数据结构
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Status | `u8` | 1 | 响应状态码（0成功） |
| Running Status | `u8` | 1 | 当前运行状态 |
| | | | 0：停止 |
| | | | 1：运行 |


---

#### 🧪 示例解析
原始帧数据：

```plain
AB CD 00 03 00 00 03 00 03 00 01
```

字段解析：

| 字段 | HEX值 | 含义 |
| --- | --- | --- |
| Status | `00` | 成功响应 |
| Running Status | `01` | 系统处于运行 |


---

### ✅ 响应状态码定义（Status 字段）
| 状态码 | 含义 |
| --- | --- |
| `0x00` | 成功 |
| `0x01` | 失败 |


## 📤 Slave2Backend 协议消息说明
从机（Slave）定时或在事件触发后向后台（Backend）主动上报状态及检测数据，包含导通、阻值和卡钉三种类型。

---

### 1️⃣ Message ID 定义
| 消息名称 | 值 (`Message ID`<br/>) | 描述 |
| --- | --- | --- |
| CONDUCTION_DATA_MSG | `0x00` | 导通数据上报 |
| RESISTANCE_DATA_MSG | `0x01` | 阻值数据上报 |
| CLIP_DATA_MSG | `0x02` | 卡钉数据上报 |


---

### 2️⃣ Device Status 字段结构（2 Byte 状态字）
按位定义如下：

| 位位置 | 名称 | 描述 |
| --- | --- | --- |
| Bit 0 | Color Sensor | 0：不匹配/无传感器；1：颜色匹配 |
| Bit 1 | Sleeve Limit | 0：断开；1：导通 |
| Bit 2 | Electromagnet Unlock Button | 0：未按下；1：按下 |
| Bit 3 | Battery Low Alarm | 0：正常；1：低电量 |
| Bit 4 | Pressure Sensor | 0：未触发；1：已触发 |
| Bit 5 | Electromagnetic Lock 1 | 0：未上锁；1：已上锁 |
| Bit 6 | Electromagnetic Lock 2 | 0：未上锁；1：已上锁 |
| Bit 7 | Accessory 1 | 0：无；1：有 |
| Bit 8 | Accessory 2 | 0：无；1：有 |
| Bit 9-15 | 保留 | 预留位 |


⚠️ 高位 Bit 9~15 建议保留，便于后续扩展

---

### 3️⃣ Payload 类型说明
#### 📘 CONDUCTION_DATA_MSG（0x00） — 导通数据
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Conduction Length | `u16` | 2 Byte | 数据长度（单位：Byte） |
| Conduction Data | `u8[]` | 可变 | 导通通道数据，逐位表示通断 |


---

#### 📘 RESISTANCE_DATA_MSG（0x01） — 阻值数据
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Resistance Length | `u16` | 2 Byte | 数据长度（单位：Byte） |
| Resistance Data | `u8[]` | 可变 | 通道电阻值原始数据 |


---

#### 📘 CLIP_DATA_MSG（0x02） — 卡钉板数据
| 字段名称 | 类型 | 长度 | 描述 |
| --- | --- | --- | --- |
| Clip Data | `u16` | 2 Byte | 卡钉板检测数据（位图） |


---

### 4️⃣ 示例消息解析
---

#### 示例 1：CONDUCTION_DATA_MSG
原始数据：

```plain
AB CD 00 04 00 00 06 00 00 55 FF 01 00 24
```

字段解析：

| 字段 | HEX值 | 描述 |
| --- | --- | --- |
| 帧头 | AB CD | 起始 |
| 源标识 | 00 04 | Slave 来源 |
| 数据长度 | 00 06 | 6 字节数据长度 |
| Message ID | 00 | 导通数据类型 |
| Device Status | 55 FF | 状态字（按位解析） |
| Conduction Length | 01 00 | 长度为 1 字节 |
| Conduction Data | 24 | 数据内容，二进制 `00100100` |


---

#### 示例 2：RESISTANCE_DATA_MSG
原始数据：

```plain
AB CD 00 04 00 00 07 00 01 55 FF 02 00 78 78
```

字段解析：

| 字段 | HEX值 | 描述 |
| --- | --- | --- |
| 数据长度 | 00 07 | 7 字节数据 |
| Message ID | 01 | 阻值数据 |
| Device Status | 55 FF | 状态字 |
| Resistance Length | 02 00 | 数据 2 字节 |
| Resistance Data | 78 78 | 阻值数据内容 |


---

#### 示例 3：CLIP_DATA_MSG
原始数据：

```plain
AB CD 00 04 00 00 05 00 02 55 FF 78 90
```

字段解析：

| 字段 | HEX值 | 描述 |
| --- | --- | --- |
| 数据长度 | 00 05 | 共 5 字节 |
| Message ID | 02 | 卡钉数据 |
| Device Status | 55 FF | 状态字 |
| Clip Data | 78 90 | 卡钉状态 0x9078 |