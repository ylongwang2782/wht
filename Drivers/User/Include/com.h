#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif

#include "gd32f4xx.h"
#include "gd32f4xx_dma.h"

#ifdef __cplusplus
}
#endif

#define ARRAYNUM(arr_name)  (uint32_t)(sizeof(arr_name) / sizeof(*(arr_name)))
#define USART0_DATA_ADDRESS ((uint32_t) & USART_DATA(USART0))
#define USART1_DATA_ADDRESS ((uint32_t) & USART_DATA(USART1))
#define USART2_DATA_ADDRESS ((uint32_t) & USART_DATA(USART2))
#define com_idle_rx_size    256

#define COM0_IRQ_PRE_PRIO   1
#define COM0_IRQ_SUB_PRIO   0
#define COM1_IRQ_PRE_PRIO   1
#define COM1_IRQ_SUB_PRIO   1
#define COM2_IRQ_PRE_PRIO   1
#define COM2_IRQ_SUB_PRIO   2

class UsartCom {
   public:
    uint8_t &rx_count;
    uint8_t rxbuffer[com_idle_rx_size];

    // 构造函数
    UsartCom(uint8_t &idle_dma_rx_count);

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

    void idle_dma_rx_check(void);

   private:
    // 发送和接收缓冲区
    uint8_t txbuffer[1];
};