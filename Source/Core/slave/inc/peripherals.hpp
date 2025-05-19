#pragma once
#include "bsp_led.hpp"
#include "bsp_log.hpp"

#define USART_LOG      USART1

#define SYS_LED_PORT GPIO::Port::C
#define SYS_LED_PIN  GPIO::Pin::PIN_13

#define PWR_LED_PORT GPIO::Port::G
#define PWR_LED_PIN  GPIO::Pin::PIN_9

#define RUN_LED_PORT GPIO::Port::G
#define RUN_LED_PIN  GPIO::Pin::PIN_12

#define ERR_LED_PORT GPIO::Port::G
#define ERR_LED_PIN  GPIO::Pin::PIN_15

extern LED sysLed;
extern LED pwrLed;
extern LED runLed;
extern LED errLed;
extern Logger Log;