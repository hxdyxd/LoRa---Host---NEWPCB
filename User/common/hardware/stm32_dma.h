#ifndef __STM32_DMA_H__
#define __STM32_DMA_H__

#include "stm32f10x.h"


void stm32_dma_init(void);
int stm32_dma_usart2_write(uint8_t *p, uint32_t len);

#endif
