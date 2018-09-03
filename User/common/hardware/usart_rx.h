#ifndef __USART_RX_H__
#define __USART_RX_H__

#include "stm32f10x.h"

#define USART_RX_MAX_NUM  (255)
#define USART_RX_TIMEOUT_MS  (10)



#define USART_API_READ_PARAMETER1    (0x01)
#define USART_API_READ_PARAMETER2    (0x02)
#define USART_API_READ_MAX_NODE_NUM  (0x03)
#define USART_API_READ_TIME          (0x04)
#define USART_API_READ_STATUS        (0x05)

#define USART_API_WRITE_PARAMETER1   (0x06)
#define USART_API_WRITE_PARAMETER2   (0x07)
#define USART_API_WRITE_MAX_NODE_NUM (0x08)
#define USART_API_WRITE_TIME         (0x09)
#define USART_API_READ_BUSY          (0x0A)
#define USART_API_SEND_USER_DATA     (0x0B)


#define USART_API_SEND_USER_DATA_UCM (0x0C)  //2018 08 25 add



#pragma pack(1)

struct sGatewayUsartStatus{
	uint8_t nmac[12];
	uint8_t fhkey[4];
	uint8_t rssi[2];
	int8_t snr;
	uint8_t diff_lat[2];
};

struct sNodeUsartStatus{
	uint8_t nmac[12];
	uint8_t gmac[12];
	uint8_t status;
	uint8_t diff_lat[2];
};

#pragma pack()

void usart_rx_isr_init(void);
void usart_rx_proc( void (* usart_rx_callback)(void) );



unsigned char *usart_rx_get_buffer_ptr(void);
uint8_t usart_rx_get_length(void);
void usart_rx_release(void);


#endif
