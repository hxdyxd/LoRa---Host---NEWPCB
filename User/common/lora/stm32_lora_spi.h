#ifndef _LORA_SPI_H
#define _LORA_SPI_H

#include "stm32f10x.h"
#include "systick.h"


void stm32_spi_init(void);
void stm32_spi_cs_select(void);
void stm32_spi_cs_deselect(void);
//void write_single_reg(uint8_t addr, uint8_t val);
void set_config(void);

//__inline uint8_t SX1276ReadDio0( void );
//__inline uint8_t SX1276ReadDio1( void );
//__inline uint8_t SX1276ReadDio2( void );
//__inline uint8_t SX1276ReadDio3( void );
//__inline uint8_t SX1276ReadDio4( void );
//__inline uint8_t SX1276ReadDio5( void );
void SX1276InitIo(void);

uint8_t read_single_reg(uint8_t addr);
void SX1276Write( uint8_t addr, uint8_t data );
void SX1276Read( uint8_t addr, uint8_t *data );
void SX1276WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size );
void SX1276ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size );


void stm32_spi_init2(void);
void SX1276InitIo2(void);
uint8_t read_single_reg2(uint8_t addr);
void SX1276Write2( uint8_t addr, uint8_t data );
void SX1276Read2( uint8_t addr, uint8_t *data );
void SX1276WriteBuffer2( uint8_t addr, uint8_t *buffer, uint8_t size );
void SX1276ReadBuffer2( uint8_t addr, uint8_t *buffer, uint8_t size );


void RESET_GPIO_Configuration(void);

#endif
