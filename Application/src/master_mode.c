#include "master_mode.h"

/* function prototypes */
void master_gpio_config(void);
void master_timer_config(void);

int master_mode_entry(void) {
    // MASTER MODE 输出脉冲波
    master_gpio_config();
    master_timer_config();

    while (1);
}

/*!
    \brief      configure the GPIO ports
    \param[in]  none
    \param[out] none
    \retval     none
*/
void master_gpio_config(void) {
    rcu_periph_clock_enable(RCU_GPIOB);

    /*configure PB3(TIMER1 CH1) as alternate function*/
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_3);

    gpio_af_set(GPIOB, GPIO_AF_1, GPIO_PIN_3);
}

// 下面两个越小精度越高
#define TIMER_PERIOD 250
#define TIMER_PRESCALER 100
/*!
    \brief      configure the timer peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
void master_timer_config(void) {
    /* ---------------------------------------------------------------------------
    TIMER1 configuration: output compare toggle mode:
    TIMER1CLK = systemcoreclock / 10000=20K,
    CH1 update rate = TIMER1 counter clock / CH1VAL = 20000/4000 = 5 Hz
    ----------------------------------------------------------------------------*/
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

    timer_deinit(TIMER1);

    /* TIMER1 configuration */
    timer_initpara.prescaler = (TIMER_PRESCALER - 1);
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = (TIMER_PERIOD - 1);
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1, &timer_initpara);

    /* CH1 configuration in OC TOGGLE mode */
    timer_ocintpara.ocpolarity = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
    timer_ocintpara.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER1, TIMER_CH_1, &timer_ocintpara);

    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1,
                                            (TIMER_PERIOD - 1));
    timer_channel_output_mode_config(TIMER1, TIMER_CH_1, TIMER_OC_MODE_TOGGLE);
    timer_channel_output_shadow_config(TIMER1, TIMER_CH_1,
                                       TIMER_OC_SHADOW_DISABLE);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER1);
    timer_enable(TIMER1);
}
