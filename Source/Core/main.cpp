#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "FreeRTOS.h"
#include "bsp_log.hpp"
#include "bsp_uid.h"
#include "chronolink.h"
#include "harness.h"
#include "mode_entry.h"

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

int main(void) {
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

#ifdef MASTER
    // 主节点模式入口
    Master_Init();
#elif defined(SLAVE)
    // 从节点模式入口
    Slave_Init();
#else
#error "Please define either MASTER or SLAVE mode"
#endif

    vTaskStartScheduler();
    for (;;);
    return 0;
}

