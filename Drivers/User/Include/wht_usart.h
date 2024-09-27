#include <stdio.h>

#include "gd32f4xx.h"
#include "systick.h"

void wht_usart_init(void);
int wht_usart_send(uint8_t *data, uint16_t len);
int wht_usart_recv(uint8_t *data, uint16_t len);