#include "stm32_lora_spi.h"
#include "app_debug.h"
#include "type.h"

void Delay10(__IO uint32_t i)
{
	while(i--);
}

/**
  * @brief  使能SPI时钟
  * @retval None
  */
static void SPI_RCC_Configuration(void)
{
#ifdef PCB_V2
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  //B12
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
#else
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
#endif
}

/**
  * @brief  配置指定SPI的引脚
  * @retval None
  */
static void SPI_GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
	
#ifdef PCB_V2
	//SPI2_IO
    GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    //EN
	
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_SetBits(GPIOB, GPIO_Pin_12);
	APP_DEBUG("lora[0] use NSS = PB12\r\n");
#else
	//SPI1_IO
    GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    //EN
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
	APP_DEBUG("lora[0] use NSS = PA4\r\n");
#endif
}

///**
//  * @brief  配置指定引脚
//  * @retval None
//  */
//static void DIO_GPIO_Configuration(void)
//{
//    GPIO_InitTypeDef GPIO_InitStruct;
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
//	//DIO
//    GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_5 | 
//								GPIO_Pin_4 | GPIO_Pin_3 | GPIO_Pin_2 | GPIO_Pin_1;
//    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//    GPIO_Init(GPIOB, &GPIO_InitStruct);
//	
//	GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_9;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
//	GPIO_Init(GPIOB, &GPIO_InitStruct);
//}

/**
  * @brief  配置指定引脚
  * @retval None
  */
void RESET_GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	//DIO
    GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
	GPIO_SetBits(GPIOA, GPIO_Pin_12);
	Delay10(100*72);
	GPIO_ResetBits(GPIOA, GPIO_Pin_12);
	Delay10(1000*72);
	GPIO_SetBits(GPIOA, GPIO_Pin_12);
	Delay10(6000*72);
}

void stm32_spi_init(void)
{
    SPI_InitTypeDef SPI_InitStruct;
 
    SPI_RCC_Configuration();
 
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStruct.SPI_Direction= SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStruct.SPI_CRCPolynomial = 7;
	
#ifdef PCB_V2
	SPI_Init(SPI2, &SPI_InitStruct);
    SPI_SSOutputCmd(SPI2, ENABLE);
    SPI_Cmd(SPI2, ENABLE);
	APP_DEBUG("lora[0] use SPI2\r\n");
#else
    SPI_Init(SPI1, &SPI_InitStruct);
    SPI_SSOutputCmd(SPI1, ENABLE);
    SPI_Cmd(SPI1, ENABLE);
	APP_DEBUG("lora[0] use SPI1\r\n");
#endif
	
	SPI_GPIO_Configuration();
}

/**
  * @brief  片选信号输出低电平
  * @retval None
  */
void stm32_spi_cs_select(void)
{
#ifdef PCB_V2
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);
#else
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
#endif
}

/**
  * @brief  片选信号输出高电平
  * @retval None
  */
void stm32_spi_cs_deselect(void)
{
#ifdef PCB_V2
    GPIO_SetBits(GPIOB, GPIO_Pin_12);
#else
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
#endif
}

uint8_t SpiInOut(uint8_t outData)
{
    /* Send SPIy data */
#ifdef PCB_V2
    SPI_I2S_SendData(SPI2, outData);
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI2);
#else
    SPI_I2S_SendData(SPI1, outData);
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI1);	
#endif
}


//lora

void write_single_reg(uint8_t addr, uint8_t val)
{
	stm32_spi_cs_select();
	SpiInOut(addr | 0x80);
	SpiInOut(val);
	stm32_spi_cs_deselect();
}

uint8_t read_single_reg(uint8_t addr)
{
	uint8_t val;
	stm32_spi_cs_select();
	SpiInOut(addr & 0x7f);
	val = SpiInOut(0);
	stm32_spi_cs_deselect();
	return val;
}

void SX1276Write( uint8_t addr, uint8_t data )
{
    SX1276WriteBuffer( addr, &data, 1 );
}

void SX1276Read( uint8_t addr, uint8_t *data )
{
    SX1276ReadBuffer( addr, data, 1 );
}

void SX1276WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    stm32_spi_cs_select();

    SpiInOut( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        SpiInOut( buffer[i] );
    }

    //NSS = 1;
    stm32_spi_cs_deselect();
}

void SX1276ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    stm32_spi_cs_select();

    SpiInOut( addr & 0x7F );

    for( i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( 0 );
    }

    //NSS = 1;
    stm32_spi_cs_deselect();
}
/*****************222222222222222222222222222***********************/


/**
  * @brief  使能SPI时钟
  * @retval None
  */
static void SPI_RCC_Configuration2(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
}

/**
  * @brief  配置指定SPI的引脚
  * @retval None
  */
static void SPI_GPIO_Configuration2(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
	//SPI1_IO
    GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    //EN
#ifdef PCB_V2
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_SetBits(GPIOB, GPIO_Pin_2);
	APP_DEBUG("lora[1] use NSS = PB2\r\n");
#else
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_SetBits(GPIOB, GPIO_Pin_12);
	APP_DEBUG("lora[1] use NSS = PB12\r\n");
#endif	
}

/**
  * @brief  配置指定引脚
  * @retval None
  */
void RESET_GPIO_Configuration2(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	//DIO
    GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_11;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
	GPIO_SetBits(GPIOA, GPIO_Pin_11);
	Delay10(1090*72);
	GPIO_ResetBits(GPIOA, GPIO_Pin_11);
	Delay10(1090*72);
	GPIO_SetBits(GPIOA, GPIO_Pin_11);
	Delay10(6090*72);
}

void stm32_spi_init2(void)
{
    SPI_InitTypeDef SPI_InitStruct;
 
    SPI_RCC_Configuration2();
 
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStruct.SPI_Direction= SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStruct.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStruct);
 
    SPI_SSOutputCmd(SPI2, ENABLE);
    SPI_Cmd(SPI2, ENABLE);
	APP_DEBUG("lora[1] use SPI2\r\n");
	
	SPI_GPIO_Configuration2();
}

/**
  * @brief  片选信号输出低电平
  * @retval None
  */
void stm32_spi_cs_select2(void)
{
	//printf("PA2 = Low\r\n");
#ifdef PCB_V2
	GPIO_ResetBits(GPIOB, GPIO_Pin_2);
#else
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);
#endif
}

/**
  * @brief  片选信号输出高电平
  * @retval None
  */
void stm32_spi_cs_deselect2(void)
{
#ifdef PCB_V2
	GPIO_SetBits(GPIOB, GPIO_Pin_2);
#else
    GPIO_SetBits(GPIOB, GPIO_Pin_12);
#endif
}

uint8_t SpiInOut2(uint8_t outData)
{
    /* Send SPIy data */
    SPI_I2S_SendData(SPI2, outData);
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI2);
}

//lora

void write_single_reg2(uint8_t addr, uint8_t val)
{
  stm32_spi_cs_select2();
  SpiInOut2(addr | 0x80);
  SpiInOut2(val);
  stm32_spi_cs_deselect2();
}

uint8_t read_single_reg2(uint8_t addr)
{
  uint8_t val;
  stm32_spi_cs_select2();
  SpiInOut2(addr & 0x7f);
  val = SpiInOut2(0);
  stm32_spi_cs_deselect2();
  return val;
}

void SX1276Write2( uint8_t addr, uint8_t data )
{
    SX1276WriteBuffer2( addr, &data, 1 );
}

void SX1276Read2( uint8_t addr, uint8_t *data )
{
    SX1276ReadBuffer2( addr, data, 1 );
}

void SX1276WriteBuffer2( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    stm32_spi_cs_select2();

    SpiInOut2( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        SpiInOut2( buffer[i] );
    }

    //NSS = 1;
    stm32_spi_cs_deselect2();
}

void SX1276ReadBuffer2( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    stm32_spi_cs_select2();

    SpiInOut2( addr & 0x7F );

    for( i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut2( 0 );
    }

    //NSS = 1;
    stm32_spi_cs_deselect2();
}
/*****************333333333333333333333333***********************/



//__inline uint8_t SX1276ReadDio0( void )
//{
//	printf("SX1276ReadDio0\r\n");
//    return 0;
//}

//__inline uint8_t SX1276ReadDio1( void )
//{
//	printf("SX1276ReadDio1\r\n");
//    return 0;
//}

//__inline uint8_t SX1276ReadDio2( void )
//{
//	printf("SX1276ReadDio2\r\n");
//    return 0;
//}

//__inline uint8_t SX1276ReadDio3( void )
//{
//	printf("SX1276ReadDio3\r\n");
//    return 0;
//}

//__inline uint8_t SX1276ReadDio4( void )
//{
//	printf("SX1276ReadDio4\r\n");
//    return 0;
//}

//__inline uint8_t SX1276ReadDio5( void )
//{
//	printf("SX1276ReadDio5\r\n");
//    return 0;
//}

//__inline void SX1276WriteRxTx( uint8_t txEnable )
//{
//	printf("SX1276WriteRxTx\r\n");
//}

void SX1276InitIo(void)
{
	RESET_GPIO_Configuration();
	stm32_spi_init();
	//DIO_GPIO_Configuration();
	
}

void SX1276InitIo2(void)
{
	RESET_GPIO_Configuration2();
	stm32_spi_init2();
	//DIO_GPIO_Configuration();
	
}
