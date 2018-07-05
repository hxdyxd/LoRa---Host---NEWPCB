/* LORANET NODE 2018 06 27 END */
/* By hxdyxd */

#include "stm32f10x.h"
#include "string.h"
#include "sx1276-LoRa.h"
#include "lora_net.h"
#include "type.h"
#include "random_freq.h"
#include "systick.h"
#include "flash.h"
#include "leds.h"
#include "keys.h"
#include "app_debug.h"
#include "interface_usart.h"
#include "usart_rx.h"

#define LORA_MODE_NUM (1)

#define NODE_STATUS_ONLINE      (0)
#define NODE_STATUS_OFFLINE     (1)
#define NODE_STATUS_NOBINDED    (2)


/*
 *  model message
*/
LORA_NET lora[LORA_MODE_NUM + 1];
uint8_t node_stats = NODE_STATUS_OFFLINE;
tTableMsg privateMsg, publicMsg;

/*
* timer
*/
uint64_t Timer1 = 0;
uint64_t Timer2 = 0;

void Delay_ms(__IO uint32_t i)
{
	uint64_t time = TickCounter;
	
	printf("Delay_ms = %d\r\n", i);
	
	while( (TickCounter - time) < i);
}

int main(void)
{
	//uint8_t buf[255];
	int len = 0, usart_len = 0;
	int i;
	uint8_t *nmac = (uint8_t *)0x1FFFF7E8;
	
	tConfig temp_config;
	tConfig *flash_config = (tConfig *)FLASH_Start_Addr;
	
	uint64_t timeout = TickCounter;
	uint64_t random_delay_timer = TickCounter;
	uint16_t random_delay_timeout = 4000;
	
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	//clear config
	//xfs_sava_cfg( (uint8_t *)&temp_config, sizeof(tConfig) );
	
	systick_init();
	leds_init();
	
#ifdef PCB_V2
	led_on(0);
	led_off(1);
#endif
	
    interface_usart_init();
	usart_rx_isr_init();
	random_adc_config();
	APP_DEBUG("USART1_Config\r\n");
	APP_DEBUG("Build , %s %s \r\n", __DATE__, __TIME__);
	
	APP_DEBUG("NODE MODULE , NMAC = ");
	lora_net_debug_hex(nmac, 12, 1);
	
	publicMsg.HoppingFrequencieSeed = 0;
	publicMsg.SpreadingFactor = 10;        // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
	publicMsg.SignalBw = 9;               // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
										// 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
	publicMsg.ErrorCoding = 4;            // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
	
	for(i=0;i<LORA_MODE_NUM;i++) {
		/* Fill Default Value */
		SX1276LoRaDeinit( &lora[i].loraConfigure );
		
		/* Configure Low Level Function */
		if(i == 0) {
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276InitIo = SX1276InitIo;
			
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276Read = SX1276Read;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276Write = SX1276Write;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276ReadBuffer = SX1276ReadBuffer;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276WriteBuffer = SX1276WriteBuffer;
			lora[i].loraConfigure.LoraLowLevelFunc.read_single_reg = read_single_reg;
			
			lora[i].loraConfigure.random_getRandomFreq = random_getRandomFreq;
			lora[i].loraConfigure.LoRaSettings.FreqHopOn = true;
		
			lora_net_Set_Config(&lora[i],  &publicMsg);
			
		}
		
		lora[i].TxTimeout = 1000;
		lora[i].RxTimeout = 2000;
		lora[i].loraConfigure.LoRaSettings.TxPacketTimeout = 1000;
		lora[i].loraConfigure.LoRaSettings.RxPacketTimeout = 2000;
		
		
		//lora[i].TickCounter = &TickCounter;
		
		lora_net_init(&lora[i]);
	}
	
	if(flash_config->isconfig == MAGIC_CONFIG) {
		node_stats = NODE_STATUS_OFFLINE;
		APP_DEBUG("NODE MODULE , GMAC = ");
		lora_net_debug_hex(flash_config->gmac, 12, 1);
	} else {
		node_stats = NODE_STATUS_NOBINDED;
		APP_DEBUG("NODE MODULE , NO BIND GATEWAY \r\n");
	}
	
	random_delay_timeout = random_getRandomTime( random_get_value() );
	
	while(1) {
		
		usart_rx_proc();
		
		switch(node_stats)
		{
		case NODE_STATUS_ONLINE:
			//User_data
			lora_net_Set_Config(&lora[0],  &privateMsg);
		
			
			usart_len = usart_rx_get_length();
			if (usart_len == 0) {
				len = lora_net_User_data(&lora[0], usart_rx_get_buffer_ptr(), 0 );
			} else {
				len = lora_net_User_data(&lora[0], usart_rx_get_buffer_ptr(), usart_len );
				
				APP_DEBUG("usart dat [%d] = ", usart_len);
				lora_net_debug_hex(usart_rx_get_buffer_ptr(), usart_len, 1);
		
				usart_rx_release();
			}
			
			if(len > 0) {
				timeout = TickCounter;
		#ifdef PCB_V2
				led_rev(0);
		#else
				led_rev(2);
		#endif
			}
			
			if(TickCounter - timeout > TIMEOUT_ONLINE) {
				APP_WARN("[offline] node lost \r\n");
				node_stats = NODE_STATUS_OFFLINE;
			}
			
			break;
		case NODE_STATUS_OFFLINE:
			//Network_request
		
			/* Dalay */
			if(TickCounter - random_delay_timer < random_delay_timeout) {
				break;
			}
			random_delay_timer = TickCounter;
			random_delay_timeout = random_getRandomTime( random_get_value() );
			printf("Delay_ms = %d\r\n", random_delay_timeout);
			/* Dalay end */
			
			//Delay_ms( random_getRandomTime( random_get_value() ) );
		
			lora_net_Set_Config(&lora[0],  &publicMsg);
			len = lora_net_Network_request(&lora[0], nmac, flash_config->gmac, &privateMsg);
			if(len  > 0) {
				timeout = TickCounter;
				node_stats = NODE_STATUS_ONLINE;
			}
			break;
		case NODE_STATUS_NOBINDED:
			/* 获取自身ID */
			
			/* Dalay */
			if(TickCounter - random_delay_timer < random_delay_timeout) {
				break;
			}
			random_delay_timer = TickCounter;
			random_delay_timeout = random_getRandomTime( random_get_value() );
			printf("Delay_ms = %d\r\n", random_delay_timeout);
			/* Dalay end */
		
			//Delay_ms( random_getRandomTime( random_get_value() ) );
		
			lora_net_Set_Config(&lora[0],  &publicMsg);
			len = lora_net_Base_station_binding(&lora[0], nmac, temp_config.gmac);
			if(len > 0) {
				temp_config.isconfig = MAGIC_CONFIG;
				xfs_sava_cfg( (uint8_t *)&temp_config, sizeof(tConfig) );
				
				if(memcmp(&temp_config, flash_config, sizeof(tConfig)) == 0) {
					lora_net_debug_hex(flash_config->gmac, 12, 1);
					node_stats = NODE_STATUS_OFFLINE;
				}
			}
			break;
		default:
			break;
		}
		
		if( (TickCounter - Timer2) > 200 ) {
			//200ms callback
			Timer2 = TickCounter;
		#ifdef PCB_V2
			if(node_stats == NODE_STATUS_OFFLINE) {
				//offline
				led_on(0);  //red on
				led_off(1);  //blue off
			} else if(node_stats == NODE_STATUS_ONLINE) {
				//online
				led_on(1);
			} else {
				led_on(0);  //red on
				led_rev(1);
			}
		#endif
		}
		
		if(node_stats != NODE_STATUS_NOBINDED &&
	#ifdef PCB_V2
		key_read(0) == Bit_RESET
	#else
		key_read(1) == Bit_RESET
	#endif
		) {
			//press
			//clear config
			temp_config.isconfig = 0x00000000;
			xfs_sava_cfg( (uint8_t *)&temp_config, sizeof(tConfig) );
			node_stats = NODE_STATUS_NOBINDED;
		}
		
		
	}
}
