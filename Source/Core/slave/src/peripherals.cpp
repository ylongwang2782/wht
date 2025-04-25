#include "peripherals.hpp"

LED sysLed(GPIO::Port::C, GPIO::Pin::PIN_13);

UartConfig uart3Conf(uart3_info);
Uart uart3(uart3Conf);
Logger Log(uart3);

