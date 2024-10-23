#include "timer.h"

#define TIMER_PRESCALER 2400
#define TIMER_PERIOD    10000

const uint8_t timer_nvic_pre_prio = 0;
const uint8_t timer_nvic_sub_prio = 0;
TimerCallback timerCallback;

extern "C" void TIMER1_IRQHandler(void) {
    if (timer_interrupt_flag_get(TIMER1, TIMER_INT_FLAG_UP)) {
        // 清除定时器中断标志
        timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_UP);

        // 调用回调函数
        if (timerCallback) {
            timerCallback();
        }
    }
}

void Timer::init(uint32_t period_ms, TimerCallback cb) {
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

    /* Configure the TIMER1 to make a timer interrupt */
    timer_deinit(TIMER1);
    timer_initpara.prescaler = (TIMER_PRESCALER - 1);
    timer_initpara.period = (period_ms * (240000000 / 1000 / TIMER_PRESCALER) - 1);
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_init(TIMER1, &timer_initpara);
    timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER1, TIMER_INT_UP);
    timer_enable(TIMER1);

    // 设置中断优先级
    nvic_irq_enable(TIMER1_IRQn, timer_nvic_pre_prio, timer_nvic_sub_prio);

    // 保存回调函数
    timerCallback = cb;
}

// 停止定时器
void Timer::stop() { timer_disable(TIMER1); }
