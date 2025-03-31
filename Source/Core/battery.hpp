#include <cstdint>

#include "TaskCPP.h"
#include "bsp_adc.hpp"

class Battery {
   public:
    uint16_t value;

    void init() {
        rcu_config();
        gpio_config();
        adc_config();
    }

    void read() {
        /* ADC routine channel config */
        adc_routine_channel_config(ADC0, 0U, 2, ADC_SAMPLETIME_15);
        /* ADC software trigger enable */
        adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);

        /* wait the end of conversion flag */
        while (!adc_flag_get(ADC0, ADC_FLAG_EOC));
        /* clear the end of conversion flag */
        adc_flag_clear(ADC0, ADC_FLAG_EOC);
        /* return regular channel sample value */
        value = adc_routine_data_read(ADC0);
    }

   private:

    void gpio_config() {
        gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_2);
    }

    void rcu_config(void) {
        /* enable ADC clock */
        rcu_periph_clock_enable(RCU_GPIOA);
        rcu_periph_clock_enable(RCU_ADC0);
        // /* config ADC clock */
        // adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    }

    /*!
        \brief      configure the ADC peripheral
        \param[in]  none
        \param[out] none
        \retval     none
    */
    void adc_config(void) {
        /* reset ADC */
        adc_deinit();
        /* ADC mode config */
        adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
        /* ADC contineous function disable */
        adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
        /* ADC scan mode disable */
        adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
        /* ADC data alignment config */
        adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
        /* ADC channel length config */
        adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 2U);

        /* ADC trigger config */
        adc_external_trigger_source_config(ADC0, ADC_ROUTINE_CHANNEL,
                                           ADC_EXTTRIG_ROUTINE_T0_CH0);
        adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL,
                                    EXTERNAL_TRIGGER_DISABLE);

        /* ADC Vbat channel enable */
        adc_channel_16_to_18(ADC_VBAT_CHANNEL_SWITCH, ENABLE);

        /* enable ADC interface */
        adc_enable(ADC0);
        TaskBase::delay(1);
        /* ADC calibration and reset calibration */
        adc_calibration_enable(ADC0);
    }
};