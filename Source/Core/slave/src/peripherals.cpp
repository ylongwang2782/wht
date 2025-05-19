#include "peripherals.hpp"

UartConfig uart3Conf(uart3_info);
Uart uart3(uart3Conf);
Logger Log(uart3);

