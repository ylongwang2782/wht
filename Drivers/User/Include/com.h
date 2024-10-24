#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif

#include "gd32f4xx.h"
#include "gd32f4xx_dma.h"

#ifdef __cplusplus
}
#endif

#define ARRAYNUM(arr_name) (uint32_t)(sizeof(arr_name) / sizeof(*(arr_name)))
#define com_idle_rx_size   256

#define COM0_IRQ_PRE_PRIO  1
#define COM0_IRQ_SUB_PRIO  0
#define COM1_IRQ_PRE_PRIO  1
#define COM1_IRQ_SUB_PRIO  1
#define COM2_IRQ_PRE_PRIO  1
#define COM2_IRQ_SUB_PRIO  2

typedef struct {
    uint32_t baudrate;                // 波特率
    uint32_t gpio_port;               // GPIO端口
    uint32_t tx_pin;                  // 发送引脚
    uint32_t rx_pin;                  // 接收引脚
    uint32_t usart_periph;            // USART外设
    rcu_periph_enum usart_clk;        // USART时钟
    rcu_periph_enum usart_port_clk;   // USART时钟
    uint8_t gpio_af;                  // GPIO复用功能
    rcu_periph_enum rcu_dma_periph;   // DMA发送通道
    uint32_t dma_periph;              // DMA发送通道
    dma_channel_enum dma_tx_channel;  // DMA发送通道
    dma_channel_enum dma_rx_channel;  // DMA接收通道
    uint8_t nvic_irq;                 // NVIC中断号
    uint8_t nvic_irq_pre_priority;    // NVIC中断优先级
    uint8_t nvic_irq_sub_priority;    // NVIC中断子优先级
    uint8_t rx_count;
} SerialConfig;

class Serial {
   public:
    Serial(SerialConfig &config) : config(config) { setup(); }
    void dma_tx(uint8_t *data, uint16_t len);
    void data_send(uint8_t *data, uint16_t len);
    uint8_t rxbuffer[256];

   private:
    const SerialConfig &config;
    void setup();
    void init();
    void dma_tx_config();
    void idle_dma_rx_config();
    uint8_t txbuffer[1];
};

class Serial1 {
   public:
    static uint8_t rx_count;
    uint8_t rxbuffer[com_idle_rx_size];

    // 初始化USART
    void init(uint32_t baudval);

    // 配置DMA发送
    void dma_tx_config();

    // 使用DMA发送数据
    void dma_tx(uint8_t *data, uint16_t len);

    // 直接发送数据
    void data_send(uint8_t *data, uint16_t len);

    // 配置DMA接收及空闲检测
    void idle_dma_rx_config();

   private:
    // 发送和接收缓冲区
    uint8_t txbuffer[1];
};

class Serial2 {
   public:
    static uint8_t rx_count;
    uint8_t rxbuffer[com_idle_rx_size];

    // 初始化USART
    void init(uint32_t baudval);

    // 配置DMA发送
    void dma_tx_config();

    // 使用DMA发送数据
    void dma_tx(uint8_t *data, uint16_t len);

    // 直接发送数据
    void data_send(uint8_t *data, uint16_t len);

    // 配置DMA接收及空闲检测
    void idle_dma_rx_config();

   private:
    // 发送和接收缓冲区
    uint8_t txbuffer[1];
};