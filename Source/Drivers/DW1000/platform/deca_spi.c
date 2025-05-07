/*! ----------------------------------------------------------------------------
 * @file    deca_spi.c
 * @brief   SPI access functions
 *
 * @attention
 *
 * Copyright 2015 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */

#include "deca_spi.h"
#include "deca_device_api.h"
#include "gd32f4xx.h"

extern spi_parameter_struct spi_init_struct;   
#define DW1000_SPI_Handle SPI1

//初始化在main中执行，此处留空
int openspi(/*SPI_TypeDef* SPIx*/)
{
    return 0;
}

int closespi(void)
{
    return 0;
} 

/*
等待总线空闲后拉低片选，向从机写数据时不但需要发送，同时也需要读取，去读一下寄存器就行，无需进行处理
先发送header，再发送body，均是spi_i2s_data_transmit与spi_i2s_data_receive一起用
全部操作结束后再拉高片选，不要读完一个就拉回去
*/
#pragma GCC optimize ("O3")
int writetospi(uint16_t headerLength,
               const uint8_t *headerBuffer,
               uint32_t bodyLength,
               const uint8_t *bodyBuffer)
{
    decaIrqStatus_t  stat ;
    stat = decamutexon() ;

    while(spi_i2s_flag_get(SPI1, SPI_FLAG_TRANS) == SET); 
    gpio_bit_reset(GPIOB, GPIO_PIN_12); 

    for(uint16_t i = 0; i < headerLength; i++) {
        while(spi_i2s_flag_get(SPI1, SPI_FLAG_TBE) == RESET); 
        spi_i2s_data_transmit(SPI1, headerBuffer[i]);
        while(spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE) == RESET); 
        (void)spi_i2s_data_receive(SPI1); 
    }
    for(uint32_t i = 0; i < bodyLength; i++) 
	{
        while(spi_i2s_flag_get(SPI1, SPI_FLAG_TBE) == RESET);
        spi_i2s_data_transmit(SPI1, bodyBuffer[i]);
        while(spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE) == RESET);
        (void)spi_i2s_data_receive(SPI1);
    }
    gpio_bit_set(GPIOB, GPIO_PIN_12); 
	decamutexoff(stat) ;
    return 0;
} 

/*
读数据，向从机发送00就行，后续还会调整
*/
#pragma GCC optimize ("O3")
int readfromspi(uint16_t headerLength,
               const uint8_t *headerBuffer,
               uint32_t readlength,
               uint8_t *readBuffer)
{
   decaIrqStatus_t  stat ;
   stat = decamutexon();

   while(spi_i2s_flag_get(SPI1, SPI_FLAG_TRANS) == SET); 
   gpio_bit_reset(GPIOB, GPIO_PIN_12); 

   for(uint16_t i = 0; i < headerLength; i++) 
   {
        while(spi_i2s_flag_get(SPI1, SPI_FLAG_TBE) == RESET); 
        spi_i2s_data_transmit(SPI1, headerBuffer[i]);
        while(spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE) == RESET); 
        spi_i2s_data_receive(SPI1); 
   }
   while(readlength-- > 0)
   {
       while(spi_i2s_flag_get(SPI1, SPI_FLAG_TBE) == RESET);
       spi_i2s_data_transmit(SPI1, 0x00);
       while(spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE) == RESET);
       *readBuffer++ = spi_i2s_data_receive(SPI1);
   }
   gpio_bit_set(GPIOB, GPIO_PIN_12); 
   decamutexoff(stat) ;
   return 0;
} 

