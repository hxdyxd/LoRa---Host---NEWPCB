#include "interface_usart.h"
#include <stdio.h>

#define USART_NUM     (2)

struct sGpio
{
	uint32_t RCC_APB2Periph;
	uint16_t GPIO_Pin;
	GPIO_TypeDef* GPIOx;
};

struct sUsart
{
	uint32_t RCC_APBPeriph;
	uint32_t USART_BaudRate;
	USART_TypeDef* USARTx;
	FunctionalState DMAReq_Tx;
	FunctionalState DMAReq_Rx;
};

struct sUsartCfg
{
	struct sUsart tUsart;
	struct sGpio tGpioTx;
	struct sGpio tGpioRx;
	uint64_t TxTimer;
};

struct sUsartCfg USART_CFG[USART_NUM] = {
	{
		.tUsart = {
			.RCC_APBPeriph = RCC_APB2Periph_USART1,
			.USART_BaudRate = 115200,
			.USARTx = USART1,
			.DMAReq_Tx = DISABLE,
			.DMAReq_Rx = DISABLE,
		},
		.tGpioTx = {
			.RCC_APB2Periph = RCC_APB2Periph_GPIOA,
			.GPIOx = GPIOA,
			.GPIO_Pin = GPIO_Pin_9,
		},
		.tGpioRx = {
			.RCC_APB2Periph = RCC_APB2Periph_GPIOA,
			.GPIOx = GPIOA,
			.GPIO_Pin = GPIO_Pin_10,
		},
	},
	{
		.tUsart = {
			.RCC_APBPeriph = RCC_APB1Periph_USART2,
			.USART_BaudRate = 115200,
			.USARTx = USART2,
			.DMAReq_Tx = ENABLE,
			.DMAReq_Rx = DISABLE,
		},
		.tGpioTx = {
			.RCC_APB2Periph = RCC_APB2Periph_GPIOA,
			.GPIOx = GPIOA,
			.GPIO_Pin = GPIO_Pin_2,
		},
		.tGpioRx = {
			.RCC_APB2Periph = RCC_APB2Periph_GPIOA,
			.GPIOx = GPIOA,
			.GPIO_Pin = GPIO_Pin_3,
		},
	},
};


void interface_usart_init(void)
{
	int i;
	GPIO_InitTypeDef GPIO_init;
	USART_InitTypeDef USART_init;
	
	for(i=0; i<USART_NUM; i++) {
	
		RCC_APB2PeriphClockCmd(USART_CFG[i].tGpioRx.RCC_APB2Periph, ENABLE);
		RCC_APB2PeriphClockCmd(USART_CFG[i].tGpioTx.RCC_APB2Periph, ENABLE);
		
		switch(USART_CFG[i].tUsart.RCC_APBPeriph){
		case RCC_APB2Periph_USART1:
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
			break;
		case RCC_APB1Periph_USART2:
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
			break;
		default:
			while(1);
		}

		
		GPIO_init.GPIO_Pin = USART_CFG[i].tGpioTx.GPIO_Pin;  //TX
		GPIO_init.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_init.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init( USART_CFG[i].tGpioTx.GPIOx , &GPIO_init);
		
		GPIO_init.GPIO_Pin = USART_CFG[i].tGpioRx.GPIO_Pin;  //RX
		GPIO_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init( USART_CFG[i].tGpioRx.GPIOx , &GPIO_init);
		
		USART_init.USART_BaudRate = USART_CFG[i].tUsart.USART_BaudRate;
		USART_init.USART_WordLength = USART_WordLength_8b;
		USART_init.USART_StopBits = USART_StopBits_1;
		USART_init.USART_Parity = 	USART_Parity_No;
		USART_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		USART_Init( USART_CFG[i].tUsart.USARTx , &USART_init);

		USART_Cmd( USART_CFG[i].tUsart.USARTx , ENABLE);
		USART_ClearFlag( USART_CFG[i].tUsart.USARTx , USART_FLAG_TC);
		USART_DMACmd( USART_CFG[i].tUsart.USARTx , USART_DMAReq_Tx, USART_CFG[i].tUsart.DMAReq_Tx );
		USART_DMACmd( USART_CFG[i].tUsart.USARTx , USART_DMAReq_Rx, USART_CFG[i].tUsart.DMAReq_Rx );
		USART_CFG[i].TxTimer = 0;
	}
}


void interface_usart_putchar( int id, unsigned char ch)
{
	USART_SendData( USART_CFG[id].tUsart.USARTx , ch);
	while(USART_GetFlagStatus( USART_CFG[id].tUsart.USARTx , USART_FLAG_TC) != SET);   //传输完成标志
}

void interface_usart_write(const unsigned char *p, int len)
{
	while(len) {
		interface_usart_putchar(1, *p);
		p++;
		len--;
	}
	USART_CFG[1].TxTimer = TickCounter;
}


void interface_usart_write_wait(void)
{
	while( TickCounter - USART_CFG[1].TxTimer < USART_TX_TIMEOUT_MS);
}


int fputc(int ch, FILE *f)
{
	interface_usart_putchar(0, (unsigned char)ch);
	return ch;
}

int fgetc(FILE *f)
{
	while(USART_GetFlagStatus(USART_CFG[0].tUsart.USARTx, USART_FLAG_RXNE) != SET);   //接收数据寄存器不为空标志
	return (int)USART_ReceiveData(USART_CFG[0].tUsart.USARTx);
}



