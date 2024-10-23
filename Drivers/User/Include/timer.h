/**
 * @file led.h
 * @author wang.yunlong (wang.yunlong9@byd.com)
 * @brief
 * @version 0.1
 * @date 2024-10-22
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "gd32f4xx.h"

#ifdef __cplusplus
}
#endif

// 定义回调函数类型
typedef void (*TimerCallback)(void);

class Timer {
   public:
    void init(uint32_t period_ms, TimerCallback cb);
    void stop();

   private:
};
