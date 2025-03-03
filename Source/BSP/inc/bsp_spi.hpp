#ifndef BSP_SPI_H
#define BSP_SPI_H
#include <cstdint>
#include <vector>

#include "FreeRTOS.h"
#include "bsp_gpio.hpp"
#include "gd32f4xx.h"
#include "task.h"
typedef struct {
    uint32_t spi_periph;
    rcu_periph_enum spi_periph_clock;

    uint32_t mosi_port;
    uint32_t mosi_pin;
    int32_t mosi_func_num;

    uint32_t miso_port;
    uint32_t miso_pin;
    int32_t miso_func_num;

    uint32_t sclk_port;
    uint32_t sclk_pin;
    int32_t sclk_func_num;

} SPI_CONFIG;

typedef struct {
    uint32_t nss_port;
    uint32_t nss_pin;
} NSS_IO;

class SpiMaster {
   private:
    SPI_CONFIG __cfg;
    std::vector<GPIO> __nss;
    GPIO __mosi;
    GPIO __miso;
    GPIO __sclk;
    std::vector<GPIO> __nss_init(std::vector<NSS_IO> nss_io) {
        std::vector<GPIO> nss;
        for (auto i : nss_io) {
            nss.push_back(GPIO((GPIO::Port)i.nss_port, (GPIO::Pin)i.nss_pin,
                               GPIO::Mode::OUTPUT, GPIO::PullUpDown::NONE,
                               GPIO::OType::PP, GPIO::Speed::SPEED_50MHZ));
            nss.back().bit_set();
        }
        return nss;
    }

   public:
    std::vector<uint8_t> rx_buffer;
    /**
     * param[in]  prescale: SPI_PSC_n (n=2,4,8,16,32,64,128,256)

    **/
    SpiMaster(SPI_CONFIG cfg, std::vector<NSS_IO> nss_io, uint32_t prescale,
              uint32_t clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE,
              uint32_t frame_size = SPI_FRAMESIZE_8BIT,
              uint32_t endian = SPI_ENDIAN_MSB)
        : __cfg(cfg),
          __nss(__nss_init(nss_io)),
          __mosi((GPIO::Port)__cfg.mosi_port, (GPIO::Pin)__cfg.mosi_pin,
                 GPIO::Mode::AF, GPIO::PullUpDown::NONE, GPIO::OType::PP,
                 GPIO::Speed::SPEED_50MHZ),
          __miso((GPIO::Port)__cfg.miso_port, (GPIO::Pin)__cfg.miso_pin,
                 GPIO::Mode::AF, GPIO::PullUpDown::NONE, GPIO::OType::PP,
                 GPIO::Speed::SPEED_50MHZ),
          __sclk((GPIO::Port)__cfg.sclk_port, (GPIO::Pin)__cfg.sclk_pin,
                 GPIO::Mode::AF, GPIO::PullUpDown::NONE, GPIO::OType::PP,
                 GPIO::Speed::SPEED_50MHZ) {
        __mosi.af_set(__cfg.mosi_func_num);
        __miso.af_set(__cfg.miso_func_num);
        __sclk.af_set(__cfg.sclk_func_num);

        spi_parameter_struct spi_init_struct;
        rcu_periph_clock_enable(cfg.spi_periph_clock);
        spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
        spi_init_struct.device_mode = SPI_MASTER;
        spi_init_struct.frame_size = frame_size;
        spi_init_struct.clock_polarity_phase = clock_polarity_phase;
        spi_init_struct.nss = SPI_NSS_SOFT;
        spi_init_struct.prescale = prescale;
        spi_init_struct.endian = endian;
        spi_init(cfg.spi_periph, &spi_init_struct);
        spi_enable(cfg.spi_periph);
        // spi_init();
    }
    ~SpiMaster() {};
    void enable() { spi_enable(__cfg.spi_periph); }
    void disable() { spi_disable(__cfg.spi_periph); }

    void nss_high(uint8_t nss_index = 0) { __nss[nss_index].bit_set(); }
    void nss_low(uint8_t nss_index = 0) { __nss[nss_index].bit_reset(); }

    bool send(std::vector<uint8_t> tx_data, uint16_t timeout_ms = 1000,
              uint8_t nss_index = 0) {
        uint32_t timeout_tick = pdMS_TO_TICKS(timeout_ms);
        uint32_t txcount = 0;
        nss_low(nss_index);
        uint32_t tickstart = xTaskGetTickCount();
        while (txcount < tx_data.size()) {
            if (SET == spi_i2s_flag_get(__cfg.spi_periph, SPI_FLAG_TBE)) {
                txcount++;
                spi_i2s_data_transmit(__cfg.spi_periph, tx_data[txcount]);
            }

            if (xTaskGetTickCount() - tickstart > timeout_tick) {
                nss_high(nss_index);
                return false;
            }
        }
        nss_high(nss_index);
        return true;
    }

    bool send_recv(std::vector<uint8_t> tx_data, uint32_t rx_len,
                   uint16_t timeout_ms = 1000, uint8_t nss_index = 0) {
        uint32_t tx_size = tx_data.size() > rx_len ? tx_data.size() : rx_len;
        uint32_t txcount = 0;
        uint32_t rxcount = 0;
        uint8_t txallowed = 1;
        uint32_t timeout_tick = pdMS_TO_TICKS(timeout_ms);
        rx_buffer.clear();
        if (rx_len > rx_buffer.capacity()) {
            rx_buffer.reserve(rx_len);
        }

        nss_low(nss_index);
        uint32_t tickstart = xTaskGetTickCount();
        while ((txcount < tx_size) || (rxcount < rx_len)) {
            if (txallowed || (txcount >= rxcount)) {
                txallowed = 0;
                if (txcount < tx_data.size()) {
                    spi_i2s_data_transmit(__cfg.spi_periph, tx_data[txcount]);
                } else {
                    spi_i2s_data_transmit(__cfg.spi_periph, 0xFF);
                }
                txcount++;
            }
            if (RESET != spi_i2s_flag_get(__cfg.spi_periph, SPI_FLAG_RBNE) &&
                rxcount < rx_len) {
                rx_buffer.push_back(spi_i2s_data_receive(__cfg.spi_periph));
                rxcount++;
                txallowed = 1;
            }

            if (xTaskGetTickCount() - tickstart > timeout_tick) {
                nss_high(nss_index);
                return false;
            }
        }
        nss_high(nss_index);
        return true;
    }
};

extern const SPI_CONFIG SPI1_PC1MOSI_PC2MISO_PB10SCLK;

#endif