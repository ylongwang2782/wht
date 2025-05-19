#include "peripherals.hpp"

UartConfig uart3Conf(uart3_info);
Uart uart3(uart3Conf);
Logger Log(uart3);

// SYS_LED = PC13
// PWR_LED = PG9
// RUN_LED = PG12
// ERR_LED = PG15
LED sysLed(SYS_LED_PORT, SYS_LED_PIN);
LED pwrLed(PWR_LED_PORT, PWR_LED_PIN);
LED runLed(RUN_LED_PORT, RUN_LED_PIN);
LED errLed(ERR_LED_PORT, ERR_LED_PIN);
