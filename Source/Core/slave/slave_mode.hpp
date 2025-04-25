#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "FreeRTOS.h"
#include "QueueCPP.h"
#include "SemaphoreCPP.h"
#include "TaskCPP.h"
#include "battery.hpp"
#include "bsp_gpio.hpp"
#include "bsp_led.hpp"
#include "bsp_log.hpp"
#include "mode_entry.h"
#include "msg_proc.hpp"
#include "protocol.hpp"
#include "uwb_interface.hpp"

#ifdef __cplusplus
extern "C" {
#endif
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"
#ifdef __cplusplus
}
#endif

#define MsgProcTask_SIZE     1024
#define MsgProcTask_PRIORITY TaskPrio_High