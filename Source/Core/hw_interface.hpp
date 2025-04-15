#ifndef HW_INTERFACE_HPP
#define HW_INTERFACE_HPP
#include <cstdint>
#include <cstdio>
#include <vector>

#include "FreeRTOScpp.h"
#include "QueueCPP.h"
#include "TaskCPP.h"
#include "bsp_gpio.hpp"
#include "bsp_log.hpp"
#include "bsp_uart.hpp"
#include "projdefs.h"
#include "uwb.hpp"

extern Logger Log;
class UwbUartInterface : public CxUwbInterface {
   public:
    UwbUartInterface()
        : uwb_com_info(usart0_info), uwb_com_cfg(uwb_com_info, false, 1024) {}
    ~UwbUartInterface() {
        delete uwb_com;
        delete en_pin;
    }

   private:
    UasrtInfo& uwb_com_info;
    UartConfig uwb_com_cfg;
    Uart* uwb_com;
    GPIO* en_pin;
    GPIO* rst_pin;

   public:
    void reset_pin_init() override {
        rst_pin = new GPIO(GPIO::Port::B, GPIO::Pin::PIN_4, GPIO::Mode::OUTPUT);
    }
    void generate_reset_signal() override { rst_pin->bit_reset(); }
    void turn_of_reset_signal() override { rst_pin->bit_set(); }
    bool send(std::vector<uint8_t>& tx_data) override {
        uwb_com->send(tx_data.data(), tx_data.size());
        return true;
    }

    bool get_recv_data(std::vector<uint8_t>& rx_data) override {
        rx_data = uwb_com->getReceivedData();
        if (rx_data.size() > 0) {
            // Log.r(rx_data.data(), rx_data.size());
            return true;
        }
        return false;
    }

    void commuication_peripheral_init() override {
        uwb_com = new Uart(uwb_com_cfg);
    }
    void chip_en_init() override {
        en_pin = new GPIO(GPIO::Port::E, GPIO::Pin::PIN_2, GPIO::Mode::OUTPUT);
    }
    void chip_enable() override { en_pin->bit_set(); }
    void chip_disable() override { en_pin->bit_reset(); }

    uint32_t get_system_1ms_ticks() override {
        return xTaskGetTickCount() * portTICK_PERIOD_MS;
    }

    void delay_ms(uint32_t ms) override { TaskBase::delay(ms); }

    void log(const char* format, ...) override {
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Log.d(buffer);
    }

    // GPIO en_pin(GPIO::Port::F, GPIO::Pin::PIN_0);
};

#endif