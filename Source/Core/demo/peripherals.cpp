#include "peripherals.hpp"

LED sysLed(GPIO::Port::C, GPIO::Pin::PIN_13);

UartConfig uart3Conf(uart3_info);
UartConfig uart6Conf(uart6_info);
Uart rs232(uart3Conf);

Uart uart6(uart6Conf);
Rs485 rs485(uart6, GPIO::Port::F, GPIO::Pin::PIN_4);

Logger Log(rs232);