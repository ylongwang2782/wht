#include <stdio.h>

#include "gd32f4xx.h"
#include "systick.h"

#define COMn               3U

#define WHT_COM0           USART0
#define WHT_COM0_CLK       RCU_USART0
#define WHT_COM0_GPIO_PORT GPIOA
#define WHT_COM0_GPIO_CLK  RCU_GPIOA
#define WHT_COM0_AF        GPIO_AF_7
#define WHT_COM0_TX_PIN    GPIO_PIN_9
#define WHT_COM0_RX_PIN    GPIO_PIN_10

#define WHT_COM1           USART1
#define WHT_COM1_CLK       RCU_USART1
#define WHT_COM1_GPIO_PORT GPIOD
#define WHT_COM1_GPIO_CLK  RCU_GPIOD
#define WHT_COM1_AF        GPIO_AF_7
#define WHT_COM1_TX_PIN    GPIO_PIN_5
#define WHT_COM1_RX_PIN    GPIO_PIN_6

#define WHT_COM2           USART2
#define WHT_COM2_CLK       RCU_USART2
#define WHT_COM2_GPIO_PORT GPIOB
#define WHT_COM2_GPIO_CLK  RCU_GPIOB
#define WHT_COM2_AF        GPIO_AF_7
#define WHT_COM2_TX_PIN    GPIO_PIN_10
#define WHT_COM2_RX_PIN    GPIO_PIN_11

enum {
    COM0 = 0,
    COM1,
    COM2,
};

void wht_com_init(uint32_t com);
void wht_com_send(uint32_t usart_periph, uint8_t *data, uint16_t len);
void wht_usart_init(void);
int wht_usart_send(uint8_t *data, uint16_t len);
int wht_usart_recv(uint8_t *data, uint16_t len);

void wht_com0_idle_dma_rx_config(uint32_t com, void (*callback)(void));
void wht_com0_dma_tx_config(void);
void wht_com0_dma_tx(uint8_t *data, uint16_t len);

void wht_com1_idle_dma_rx_config(uint32_t com, void (*callback)(void));
void wht_com1_dma_tx_config(void);
void wht_com1_dma_tx(uint8_t *data, uint16_t len);

void wht_com2_idle_dma_rx_config(uint32_t com, void (*callback)(void));
void wht_com2_dma_tx_config(void);
void wht_com2_dma_tx(uint8_t *data, uint16_t len);