#include "stm32_dma.h"

#define DMA_NUM     (1)

struct sDmaCfg
{
	uint32_t RCC_AHBPeriph;
	DMA_Channel_TypeDef* DMAy_Channelx;
	uint32_t DMA_PeripheralBaseAddr;
	uint32_t DMA_DIR;
	FunctionalState DMA_Cmd_State;
	//ISR
	FunctionalState DMA_IT_TC_State;
	uint8_t NVIC_IRQChannel;
	uint8_t DMA_TC_FLAG;
};

struct sDmaCfg DMA_CFG[DMA_NUM] = {
	{
		#define DMA_IRQHandler_Index0   DMA1_Channel7_IRQHandler
		#define DMA_FLAG_TC_Index0      DMA1_FLAG_TC7
		.RCC_AHBPeriph = RCC_AHBPeriph_DMA1,
		.DMAy_Channelx = DMA1_Channel7,   //USART2_TX
		.DMA_PeripheralBaseAddr = (uint32_t)(&(USART2->DR)),  //USART2
		.DMA_DIR = DMA_DIR_PeripheralDST,            //Memory to  Periphera
		.DMA_Cmd_State = DISABLE,
		.DMA_TC_FLAG =1,
		//ISR
		.DMA_IT_TC_State = ENABLE,
		.NVIC_IRQChannel = DMA1_Channel7_IRQn,
	},
	
};


void stm32_dma_init(void)
{
	int i;
	DMA_InitTypeDef            DMA_InitStructure;
    NVIC_InitTypeDef           NVIC_InitStructure;
	
	for(i=0; i<DMA_NUM; i++) {
		RCC_AHBPeriphClockCmd(DMA_CFG[i].RCC_AHBPeriph , ENABLE);
		
		DMA_DeInit( DMA_CFG[i].DMAy_Channelx );
		DMA_InitStructure.DMA_PeripheralBaseAddr = DMA_CFG[i].DMA_PeripheralBaseAddr;

		DMA_InitStructure.DMA_MemoryBaseAddr = 0;

		DMA_InitStructure.DMA_DIR = DMA_CFG[i].DMA_DIR;
		DMA_InitStructure.DMA_BufferSize = 0;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
		DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
		DMA_Init( DMA_CFG[i].DMAy_Channelx , &DMA_InitStructure);
		
		DMA_ITConfig( DMA_CFG[i].DMAy_Channelx, DMA_IT_TC, DMA_CFG[i].DMA_IT_TC_State);
		
		NVIC_InitStructure.NVIC_IRQChannel = DMA_CFG[i].NVIC_IRQChannel;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = DMA_CFG[i].DMA_IT_TC_State;
		NVIC_Init(&NVIC_InitStructure);
		
		DMA_Cmd( DMA_CFG[i].DMAy_Channelx , DMA_CFG[i].DMA_Cmd_State);
	}
}


int stm32_dma_usart2_write(uint8_t *p, uint32_t len)
{
    while(! DMA_CFG[0].DMA_TC_FLAG );
    DMA_CFG[0].DMAy_Channelx->CNDTR = len;
    DMA_CFG[0].DMAy_Channelx->CMAR = (uint32_t)p;
    DMA_Cmd( DMA_CFG[0].DMAy_Channelx, ENABLE);
    DMA_CFG[0].DMA_TC_FLAG = 0;
}


void DMA_IRQHandler_Index0(void)
{
    if(DMA_GetITStatus(DMA_FLAG_TC_Index0)) {
        DMA_ClearFlag(DMA_FLAG_TC_Index0);
        DMA_Cmd(DMA_CFG[0].DMAy_Channelx, DISABLE);
        DMA_CFG[0].DMA_TC_FLAG = 1;
    }
}


