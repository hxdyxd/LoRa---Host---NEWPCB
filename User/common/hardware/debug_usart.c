#include "debug_usart.h"

void USART1_Config(void)
{
	GPIO_InitTypeDef GPIO_init;
	USART_InitTypeDef USART_init;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_init.GPIO_Pin = GPIO_Pin_9;  //TX
	GPIO_init.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_init);
	
	GPIO_init.GPIO_Pin = GPIO_Pin_10;  //RX
	GPIO_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_init);
	
	USART_init.USART_BaudRate = 115200;
	USART_init.USART_WordLength = USART_WordLength_8b;
	USART_init.USART_StopBits = USART_StopBits_1;
	USART_init.USART_Parity = 	USART_Parity_No;
	USART_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_init);
		

	USART_Cmd(USART1, ENABLE);
	USART_ClearFlag(USART1, USART_FLAG_TC);
}

void put_hex(uint8_t *str, int len, char end)
{
	int i;
	for(i = 0; i < len; i++) {
		printf("%02x ", str[i]);
	}
	if(end) {
		printf("\r\n");
	}
}

int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t)ch);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET);   //传输完成标志
	return ch;
}

int fgetc(FILE *f)
{
	while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != SET);   //接收数据寄存器不为空标志
	return (int)USART_ReceiveData(USART1);
}
