#include "wht_timer.h"
#include <stdint.h>

// TIEMR TRIGGER INTERVAL = 240M / TIMER_PRESCALER / TIMER_PERIOD = 10kHz
// TIEMR TRIGGER INTERVAL = 240M / 2400 / 1000 = 100Hz
// TIEMR TRIGGER INTERVAL = 240M / 2400 / 10000 = 10Hz
#define TIMER_PRESCALER 2400 - 1
#define TIMER_PERIOD 10000 - 1

const uint8_t timer_nvic_pre_prio = 0;
const uint8_t timer_nvic_sub_prio = 0;

void (*timer1_callback)(void) = NULL;

void timer1_init(void (*callback)(void)) {
    
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

    /* Configure the TIMER1 to make a timer interrupt */
    timer_deinit(TIMER1);
    timer_initpara.prescaler = (TIMER_PRESCALER - 1);
    timer_initpara.period = (TIMER_PERIOD - 1);
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_init(TIMER1, &timer_initpara);
    timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER1, TIMER_INT_UP);
    timer_enable(TIMER1);

    // 设置中断优先级
    nvic_irq_enable(TIMER1_IRQn, timer_nvic_pre_prio, timer_nvic_sub_prio);
    // 注册回调函数
    timer1_callback = callback;
}
