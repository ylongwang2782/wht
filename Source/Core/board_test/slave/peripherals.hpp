#pragma once
#include <cstdint>
#include <cstdio>

#include "TaskCPP.h"
#include "bsp_gpio.hpp"
#include "bsp_led.hpp"
#include "bsp_log.hpp"
#include "bsp_spi.hpp"
#include "bsp_uart.hpp"
#include "deca_device_api.h"
#include "deca_regs.h"

extern LED sysLed;
extern Uart rs232;
extern Uart uart6;
extern Uart usart0;
extern Logger Log;
extern Rs485 rs485;

const std::pair<GPIO::Port, GPIO::Pin> condPinInfo[] = {
    // PA3 - PA7
    {GPIO::Port::A, GPIO::Pin::PIN_3},    // PA3
    {GPIO::Port::A, GPIO::Pin::PIN_4},    // PA4
    {GPIO::Port::A, GPIO::Pin::PIN_5},    // PA5
    {GPIO::Port::A, GPIO::Pin::PIN_6},    // PA6
    {GPIO::Port::A, GPIO::Pin::PIN_7},    // PA7
    // PC4 - PC5
    {GPIO::Port::C, GPIO::Pin::PIN_4},    // PC4
    {GPIO::Port::C, GPIO::Pin::PIN_5},    // PC5
    // PB0 1
    {GPIO::Port::B, GPIO::Pin::PIN_0},    // PB0
    {GPIO::Port::B, GPIO::Pin::PIN_1},    // PB1
    // PF11 - PF15
    {GPIO::Port::F, GPIO::Pin::PIN_11},    // PF11
    {GPIO::Port::F, GPIO::Pin::PIN_12},    // PF12
    {GPIO::Port::F, GPIO::Pin::PIN_13},    // PF13
    {GPIO::Port::F, GPIO::Pin::PIN_14},    // PF14
    {GPIO::Port::F, GPIO::Pin::PIN_15},    // PF15
    // PG0 - PG1
    {GPIO::Port::G, GPIO::Pin::PIN_0},    // PG0
    {GPIO::Port::G, GPIO::Pin::PIN_1},    // PG1
    // PE7 - PE15
    {GPIO::Port::E, GPIO::Pin::PIN_7},     // PE7
    {GPIO::Port::E, GPIO::Pin::PIN_8},     // PE8
    {GPIO::Port::E, GPIO::Pin::PIN_9},     // PE9
    {GPIO::Port::E, GPIO::Pin::PIN_10},    // PE10
    {GPIO::Port::E, GPIO::Pin::PIN_11},    // PE11
    {GPIO::Port::E, GPIO::Pin::PIN_12},    // PE12
    {GPIO::Port::E, GPIO::Pin::PIN_13},    // PE13
    {GPIO::Port::E, GPIO::Pin::PIN_14},    // PE14
    {GPIO::Port::E, GPIO::Pin::PIN_15},    // PE15
    // PB10 - PB15
    {GPIO::Port::B, GPIO::Pin::PIN_10},    // PB10
    {GPIO::Port::B, GPIO::Pin::PIN_11},    // PB11
    {GPIO::Port::B, GPIO::Pin::PIN_12},    // PB12
    {GPIO::Port::B, GPIO::Pin::PIN_13},    // PB13
    {GPIO::Port::B, GPIO::Pin::PIN_14},    // PB14
    {GPIO::Port::B, GPIO::Pin::PIN_15},    // PB15
    // PD8 - PD15
    {GPIO::Port::D, GPIO::Pin::PIN_8},     // PD8
    {GPIO::Port::D, GPIO::Pin::PIN_9},     // PD9
    {GPIO::Port::D, GPIO::Pin::PIN_10},    // PD10
    {GPIO::Port::D, GPIO::Pin::PIN_11},    // PD11
    {GPIO::Port::D, GPIO::Pin::PIN_12},    // PD12
    {GPIO::Port::D, GPIO::Pin::PIN_13},    // PD13
    {GPIO::Port::D, GPIO::Pin::PIN_14},    // PD14
    {GPIO::Port::D, GPIO::Pin::PIN_15},    // PD15
    // PG2 - PG8
    {GPIO::Port::G, GPIO::Pin::PIN_2},    // PG2
    {GPIO::Port::G, GPIO::Pin::PIN_3},    // PG3
    {GPIO::Port::G, GPIO::Pin::PIN_4},    // PG4
    {GPIO::Port::G, GPIO::Pin::PIN_5},    // PG5
    {GPIO::Port::G, GPIO::Pin::PIN_6},    // PG6
    {GPIO::Port::G, GPIO::Pin::PIN_7},    // PG7
    {GPIO::Port::G, GPIO::Pin::PIN_8},    // PG8
    // PC6 - PC9
    {GPIO::Port::C, GPIO::Pin::PIN_6},    // PC6
    {GPIO::Port::C, GPIO::Pin::PIN_7},    // PC7
    {GPIO::Port::C, GPIO::Pin::PIN_8},    // PC8
    {GPIO::Port::C, GPIO::Pin::PIN_9},    // PC9
    // PA8 - PA15
    {GPIO::Port::A, GPIO::Pin::PIN_8},     // PA8
    {GPIO::Port::A, GPIO::Pin::PIN_11},    // PA11
    {GPIO::Port::A, GPIO::Pin::PIN_12},    // PA12
    {GPIO::Port::A, GPIO::Pin::PIN_15},    // PA15
    // PC10 - PC12
    {GPIO::Port::C, GPIO::Pin::PIN_10},    // PC10
    {GPIO::Port::C, GPIO::Pin::PIN_11},    // PC11
    {GPIO::Port::C, GPIO::Pin::PIN_12},    // PC12
    //  PD0 - PD6
    {GPIO::Port::D, GPIO::Pin::PIN_0},    // PD0
    {GPIO::Port::D, GPIO::Pin::PIN_1},    // PD1
    {GPIO::Port::D, GPIO::Pin::PIN_2},    // PD2
    {GPIO::Port::D, GPIO::Pin::PIN_3},    // PD3
    {GPIO::Port::D, GPIO::Pin::PIN_4},    // PD4
    {GPIO::Port::D, GPIO::Pin::PIN_5},    // PD5
    {GPIO::Port::D, GPIO::Pin::PIN_6},    // PD6
};

class HarnessGpio {
   public:
    static constexpr size_t NumPins =
        sizeof(condPinInfo) / sizeof(condPinInfo[0]);

    static std::vector<GPIO> condGpioArray;

    void init() {
        condGpioArray.reserve(NumPins);

        for (const auto& info : condPinInfo) {
            condGpioArray.emplace_back(info.first, info.second,
                                       GPIO::Mode::OUTPUT);
        }
    }

    void deinit() { condGpioArray.clear(); }

    void reset() {
        for (auto& gpio : condGpioArray) {
            gpio.bit_reset();    // 每个都重置
        }
    }

    void set() {
        for (auto& gpio : condGpioArray) {
            gpio.bit_set();    // 每个都设置
        }
    }

    void toggle() {
        for (auto& gpio : condGpioArray) {
            gpio.toggle();    // 每个都设置
        }
    }
};

class Key {
   public:
    Key(const char* name, GPIO::Port port, GPIO::Pin pin)
        : name(name), gpio(port, pin, GPIO::Mode::INPUT) {}

    bool isPressed() const { return !gpio.input_bit_get(); }

    const char* getName() const { return name; }

   private:
    const char* name;
    GPIO gpio;
};
struct DipSwitchInfo {
    struct PinInfo {
        GPIO::Port port;
        GPIO::Pin pin;
    };

    PinInfo pins[8];    // 8个拨码开关，每个有端口和引脚编号
};

class DipSwitch {
   public:
    DipSwitch(const DipSwitchInfo& info)
        : keys{Key("DIP0", info.pins[0].port, info.pins[0].pin),
               Key("DIP1", info.pins[1].port, info.pins[1].pin),
               Key("DIP2", info.pins[2].port, info.pins[2].pin),
               Key("DIP3", info.pins[3].port, info.pins[3].pin),
               Key("DIP4", info.pins[4].port, info.pins[4].pin),
               Key("DIP5", info.pins[5].port, info.pins[5].pin),
               Key("DIP6", info.pins[6].port, info.pins[6].pin),
               Key("DIP7", info.pins[7].port, info.pins[7].pin)} {}

    // 获取某一位拨码开关的状态，0~7
    bool isOn(int index) const {
        if (index < 0 || index >= 8) return false;
        return keys[index].isPressed();    // 拨码开关拨到 ON 为按下
    }

    // 获取整个8位拨码开关的值，按位打包
    uint8_t value() const {
        uint8_t val = 0;
        for (int i = 0; i < 8; ++i) {
            if (isOn(i)) {
                val |= (1 << i);
            }
        }
        return val;
    }

   private:
    Key keys[8];
};

class Elv {
   public:
    Elv(GPIO::Port port, GPIO::Pin pin) : gpio(port, pin, GPIO::Mode::OUTPUT) {}
    void toggle() { gpio.toggle(); }

   private:
    GPIO gpio;    // Add this member declaration
};

extern const Spi_IOConfig SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS;
extern dwt_config_t dw1000_config;

#define FRAME_LEN_MAX 127
class DW1000 {
   public:
   static constexpr const char TAG[] = "DW1000";
    uint8_t rx_buffer[FRAME_LEN_MAX];
    uint32_t status_reg = 0;
    uint16_t frame_len = 0;

    DW1000() { init(); }

    int init() {
        spi_config();
        rst_init();
        reset();

        if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR) {
            Log.e(TAG,"init FAIL");
            return 1;
        }

        port_set_dw1000_fastrate();    // spi freq: 1.875MHz ->15MHz
        dwt_configure(&dw1000_config);

        dwt_write32bitreg(TX_POWER_ID, 0x85858585);
        Log.d(TAG,"init OK");
        return 0;
    }

    // static uint8_t tx_msg[] = {
    //     0xC5,    // Frame control: blink frame
    //     0x00,    // Sequence number
    //     0xDE, 0xCA, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x00    // CRC
    // };
    // #define BLINK_FRAME_SN_IDX 1

    void send(std::vector<uint8_t> txdata) {
        dwt_writetxdata(txdata.size(), txdata.data(), 0);
        dwt_writetxfctrl(txdata.size(), 0, 0);

        dwt_starttx(DWT_START_TX_IMMEDIATE);
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS)) {
        };

        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
    }

    int recv() {
        int i;
        /* Clear the RX buffer. */
        for (i = 0; i < FRAME_LEN_MAX; i++) {
            rx_buffer[i] = 0;
        }

        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_GOOD |
                                             SYS_STATUS_ALL_RX_ERR |
                                             SYS_STATUS_ALL_RX_TO);
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        TaskBase::delay(1);

        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) &
                 (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR |
                  SYS_STATUS_ALL_RX_TO))) {
        };

        if (status_reg & SYS_STATUS_RXFCG) {
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
            if (frame_len <= FRAME_LEN_MAX) {
                dwt_readrxdata(rx_buffer, frame_len, 0);
                frame_len -= 2;    // del CRC
            }
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
        } else if (status_reg & SYS_STATUS_ALL_RX_TO) {
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO);
            Log.e(TAG,"RX TIMEOUT!");
            return 1;
        } else if (status_reg & SYS_STATUS_ALL_RX_ERR) {
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
            Log.e(TAG,"RX ERROR!");
            return 1;
        }
        // Log.d("RX OK!");
        return 0;
    }

    bool get_recv_data(std::vector<uint8_t>& rx_data) {
        // copy rx_buffer to rx_data
        rx_data.clear();
        memcpy(rx_data.data(), rx_buffer, frame_len);
        frame_len = 0;
        if (rx_data.size() > 0) {
            return true;
        }
        return false;
    }

   private:
    void rst_init() {
        gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_3);
        gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                                GPIO_PIN_3);
    }

    void reset() {
        gpio_bit_reset(GPIOE, GPIO_PIN_3);    // reset pin
        TaskBase::delay(5);                   // hold
        gpio_bit_set(GPIOE, GPIO_PIN_3);
    }

    void port_set_dw1000_fastrate() {
        spi_disable(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.spi_periph);

        spi_parameter_struct spi_init_struct;
        spi_struct_para_init(&spi_init_struct);
        spi_init_struct.device_mode = SPI_MASTER;
        spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
        spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
        spi_init_struct.endian = SPI_ENDIAN_MSB;
        spi_init_struct.nss = SPI_NSS_SOFT;
        spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
        spi_init_struct.prescale = SPI_PSC_2;
        spi_init(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.spi_periph, &spi_init_struct);

        spi_enable(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.spi_periph);
    }

    void spi_config() {
        // 打开所有相关外设时钟
        rcu_periph_clock_enable(
            SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.spi_periph_clock);
        rcu_periph_clock_enable(RCU_GPIOE);

        // 配置 SCK, MISO, MOSI 复用
        gpio_af_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.sclk_port,
                    SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.sclk_func_num,
                    SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.sclk_pin);
        gpio_mode_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.sclk_port, GPIO_MODE_AF,
                      GPIO_PUPD_NONE,
                      SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.sclk_pin);
        gpio_output_options_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.sclk_port,
                                GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                                SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.sclk_pin);

        gpio_af_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.miso_port,
                    SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.miso_func_num,
                    SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.miso_pin);
        gpio_mode_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.miso_port, GPIO_MODE_AF,
                      GPIO_PUPD_NONE,
                      SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.miso_pin);
        gpio_output_options_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.miso_port,
                                GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                                SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.miso_pin);

        gpio_af_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.mosi_port,
                    SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.mosi_func_num,
                    SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.mosi_pin);
        gpio_mode_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.mosi_port, GPIO_MODE_AF,
                      GPIO_PUPD_NONE,
                      SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.mosi_pin);
        gpio_output_options_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.mosi_port,
                                GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                                SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.mosi_pin);

        // NSS 独立配置为普通输出（软件控制）
        gpio_mode_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.nss_port,
                      GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                      SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.nss_pin);
        gpio_output_options_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.nss_port,
                                GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                                SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.nss_pin);
        gpio_bit_set(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.nss_port,
                     SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.nss_pin);

        spi_parameter_struct spi_init_struct;
        spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
        spi_init_struct.device_mode = SPI_MASTER;
        spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
        spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
        spi_init_struct.nss = SPI_NSS_SOFT;
        spi_init_struct.prescale = SPI_PSC_32;    // 这里spi主频为60MHz
        spi_init_struct.endian = SPI_ENDIAN_MSB;
        spi_init(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.spi_periph, &spi_init_struct);

        spi_nss_output_disable(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.spi_periph);
        spi_enable(SPI3_E6MOSI_E5MISO_E2SCLK_E11NSS.spi_periph);
    }
};