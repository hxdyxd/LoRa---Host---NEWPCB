#ifndef  _DEBUG_USART_H_
#define  _DEBUG_USART_H_

#include "stm32f10x.h"
#include <stdio.h>

void USART1_Config(void);
void put_hex(uint8_t *str, int len, char end);

#endif
