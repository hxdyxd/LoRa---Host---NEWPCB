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
#include "stm32_dma.h"
#include "soft_timer.h"

#define LORA_MODEL_NUM (1)


/*
 *  model message
*/
LORA_NET lora[LORA_MODEL_NUM];
uint8_t node_stats = NODE_STATUS_OFFLINE;
tTableMsg privateMsg = {
	.HoppingFrequencieSeed = 1,  //RANDOM
	.SpreadingFactor = 10,        // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
	.SignalBw = 8,               // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
										// 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
	.ErrorCoding = 4,            // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
};						
tTableMsg publicMsg = {
	.HoppingFrequencieSeed = 0,
	.SpreadingFactor = 10,        // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
	.SignalBw = 8,               // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
										// 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
	.ErrorCoding = 4,            // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
};

/*
* timer
*/
uint64_t timeout = 0;  //online -> offline

/*
* addr
*/
uint8_t *nmac = (uint8_t *)0x1FFFF7E8;
tConfig *flash_config = (tConfig *)FLASH_Start_Addr;
tConfig temp_config;

/*
* write
*/
uint8_t gs_write_buf[256];
uint8_t gs_usart_write_buf[1024];
uint8_t gs_write_len = 0;
uint8_t NODE_BUST_FLAG = DEVICE_NO_BUSY;   //1: no busy
uint8_t gs_write_user_data_mode = USER_DATA_MODE_UCM;


void random_message_timeout_callback(void);


void usart_rx_callback(void)
{
	struct sNodeUsartStatus usartStatus;
	uint8_t len;
	
	uint8_t *usart_buffer = usart_rx_get_buffer_ptr();
	switch( usart_buffer[0] )
	{
	case USART_API_READ_PARAMETER1:
		/* CMD(1) + FHKEY(4) + SF(0.4) + SB(0.4) + EC(0.5) */
		gs_usart_write_buf[0] = USART_API_READ_PARAMETER1 | 0x80;
		
		gs_usart_write_buf[1] = publicMsg.HoppingFrequencieSeed >> 24;
		gs_usart_write_buf[2] = publicMsg.HoppingFrequencieSeed >> 16;
		gs_usart_write_buf[3] = publicMsg.HoppingFrequencieSeed >> 8;
		gs_usart_write_buf[4] = publicMsg.HoppingFrequencieSeed;
		
		gs_usart_write_buf[5] = (publicMsg.SpreadingFactor << 4) | publicMsg.SignalBw;
		gs_usart_write_buf[6] = (publicMsg.ErrorCoding << 5);
	
		stm32_dma_usart2_write(gs_usart_write_buf, 7 );
		usart_rx_release();
		
		APP_DEBUG("usart read parameter1 : \r\n");
		APP_DEBUG(" Seed = %d \r\n", publicMsg.HoppingFrequencieSeed );
		APP_DEBUG(" Bw = %d \r\n", publicMsg.SignalBw );
		APP_DEBUG(" Sf = %d \r\n", publicMsg.SpreadingFactor );
		APP_DEBUG(" Ec = %d \r\n", publicMsg.ErrorCoding );
		break;
	
	case USART_API_READ_PARAMETER2:
		/* CMD(1) + SF(0.4) + SB(0.4) + EC(0.5) */
		gs_usart_write_buf[0] = USART_API_READ_PARAMETER2 | 0x80;
		
		if(node_stats == NODE_STATUS_ONLINE) {
			gs_usart_write_buf[1] = (privateMsg.SpreadingFactor << 4) | privateMsg.SignalBw;
			gs_usart_write_buf[2] = (privateMsg.ErrorCoding << 5);
		} else {
			gs_usart_write_buf[1] = gs_usart_write_buf[2] = 0;
		}
	
		stm32_dma_usart2_write(gs_usart_write_buf, 3 );
		usart_rx_release();
		
		APP_DEBUG("usart read parameter2 : \r\n");
		APP_DEBUG(" Bw = %d \r\n", privateMsg.SignalBw );
		APP_DEBUG(" Sf = %d \r\n", privateMsg.SpreadingFactor );
		APP_DEBUG(" Ec = %d \r\n", privateMsg.ErrorCoding );
		break;
		
	case USART_API_READ_MAX_NODE_NUM:
		/* CMD(1) + NUMBER(1) */
		gs_usart_write_buf[0] = USART_API_READ_MAX_NODE_NUM | 0x80;
		
		gs_usart_write_buf[1] = LORA_MAX_NODE_NUM;
	
		stm32_dma_usart2_write(gs_usart_write_buf, 2 );
		usart_rx_release();
		
		APP_DEBUG("usart read max node number = %d\r\n", LORA_MAX_NODE_NUM);
		break;
	
	case USART_API_READ_TIME:
		/* CMD(1) + RANDOM LOW(2) + RANDOM HIGH(2) + TIMEOUT(2) + ONLINE TIMEOUT(2) */
		gs_usart_write_buf[0] = USART_API_READ_TIME | 0x80;
		
		gs_usart_write_buf[1] = 0;
		gs_usart_write_buf[2] = 0;
		gs_usart_write_buf[3] = 0;
		gs_usart_write_buf[4] = 0;
	
		gs_usart_write_buf[5] = 0;
		gs_usart_write_buf[6] = 0;
	
		gs_usart_write_buf[7] = 0;
		gs_usart_write_buf[8] = 0;
	
		stm32_dma_usart2_write(gs_usart_write_buf, 9 );
		usart_rx_release();
	
		APP_DEBUG("usart read time %d-%d , %d , %d\r\n", 0, 0, 0, 0);
		break;
		
	case USART_API_READ_STATUS:
		/* CMD(1) + NMAC(12) + GMAC(12) + STATUS(1) + DIFF LAT(2) ... */
		gs_usart_write_buf[0] = USART_API_READ_STATUS | 0x80;
	
		memcpy( usartStatus.nmac, nmac, 12);
		memcpy( usartStatus.gmac, flash_config->gmac, 12);
		usartStatus.status = node_stats;
	
		uint16_t diff_lat;
		diff_lat = (TickCounter - timeout);
		usartStatus.diff_lat[0] = (diff_lat >> 8) & 0xff;
		usartStatus.diff_lat[1] = diff_lat & 0xff;
	
		memcpy( gs_usart_write_buf + 1, usartStatus.nmac, sizeof(struct sNodeUsartStatus) );
		//TX
		stm32_dma_usart2_write(gs_usart_write_buf,  sizeof(struct sNodeUsartStatus) + 1);
		usart_rx_release();
		
		APP_DEBUG("usart read status, status = %d\r\n", node_stats);
		break;
	
	case USART_API_READ_BUSY:
		/* CMD(1) + STATUS（1） */
		gs_usart_write_buf[0] = USART_API_READ_BUSY | 0x80;
		gs_usart_write_buf[1] = NODE_BUST_FLAG;
		//TX
		stm32_dma_usart2_write(gs_usart_write_buf,  2);
		usart_rx_release();
	
		APP_DEBUG("usart busy = %d\r\n", NODE_BUST_FLAG);
		break;
	
	case USART_API_SEND_USER_DATA:
		len = usart_rx_get_length();
		if( len >= 13 && NODE_BUST_FLAG == DEVICE_NO_BUSY ) {
			gs_write_len = len - 1;
			memcpy(gs_write_buf, usart_rx_get_buffer_ptr() + 1, gs_write_len);
			NODE_BUST_FLAG = DEVICE_BUSY;
			gs_write_user_data_mode = USER_DATA_MODE_DEFAULT;
		} else {
			APP_DEBUG("[ busy ] usart data [%d] = \r\n", len - 1);
		}
		
		usart_rx_release();
		break;
		
	case USART_API_SEND_USER_DATA_UCM:
		len = usart_rx_get_length();
		if( len > 2 && NODE_BUST_FLAG == DEVICE_NO_BUSY ) {
			gs_write_len = len - 1;
			memcpy(gs_write_buf, usart_rx_get_buffer_ptr() + 1, gs_write_len);
			NODE_BUST_FLAG = DEVICE_BUSY;
			gs_write_user_data_mode = USER_DATA_MODE_UCM;
		} else {
			APP_DEBUG("[ busy ] usart ucm data [%d] = \r\n", len - 1);
		}
		
		usart_rx_release();
		break;		
	
	default:
		
		break;
	}
}


void led_status_callback(void)
{
	//200ms callback
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


void user_key_proc(int8_t key_id)
{
	if(node_stats != NODE_STATUS_NOBINDED) {
		//press
		//clear config
		temp_config.isconfig = 0x00000000;
		xfs_sava_cfg( (uint8_t *)&temp_config, sizeof(tConfig) );
		node_stats = NODE_STATUS_NOBINDED;
		
		//delay random time
		soft_timer_create(NODE_TIMER_RANDOM_MSG_TIMEOUT, 1, 1, random_message_timeout_callback, random_getRandomTime(random_get_value()) );
	}
}


void random_message_timeout_callback(void)
{
	switch(node_stats)
	{
	case NODE_STATUS_ONLINE:
		//offline timeout
	
		APP_WARN("[offline] node lost \r\n");
		node_stats = NODE_STATUS_OFFLINE;
	
		//use publicMsg
		lora_net_Set_Config(&lora[0],  &publicMsg);
	
		//delay random time
		soft_timer_create(NODE_TIMER_RANDOM_MSG_TIMEOUT, 1, 1, random_message_timeout_callback, random_getRandomTime(random_get_value()) );
		break;
	case NODE_STATUS_OFFLINE:
		//join Network request
	
		lora_net_Set_Config(&lora[0],  &publicMsg);
		lora_net_Network_request(&lora[0], nmac, flash_config->gmac);
		APP_DEBUG("[Send] join Network request\r\n");
		
		//refresh timer, delay random time
		soft_timer_create(NODE_TIMER_RANDOM_MSG_TIMEOUT, 1, 1, random_message_timeout_callback, random_getRandomTime(random_get_value()) );
		break;
	case NODE_STATUS_NOBINDED:
		/* 获取自身ID */
		
		lora_net_Set_Config(&lora[0],  &publicMsg);
		lora_net_Base_station_binding(&lora[0], nmac);
		APP_DEBUG("[Send] Base station bindin request\r\n");
		
		//refresh timer, delay random time
		soft_timer_create(NODE_TIMER_RANDOM_MSG_TIMEOUT, 1, 1, random_message_timeout_callback, random_getRandomTime(random_get_value()) );
		break;
	default:
		APP_ERROR("[bug] node_stats failed\r\n");
		break;
	}
}


void lora_message_callback(struct sLORA_NET *netp)
{
	int len = 0;
	
	switch(node_stats)
	{
	case NODE_STATUS_ONLINE:
		
	
		switch(gs_write_user_data_mode)
		{
		case USER_DATA_MODE_UCM:
			/* CMD(1) + UC（USER_CODE_LEN） + 数据（n） */
			len = lora_net_User_data_r(netp, gs_usart_write_buf + 1);
			break;
		
		case USER_DATA_MODE_DEFAULT:
			/* CMD(1) + 源地址（12） + 本机地址（12） + 数据（n） */
			len = lora_net_User_data_r(netp, gs_usart_write_buf + 25);
			break;
		
		default:
			break;
		}
		//len = lora_net_User_data_r(netp, gs_usart_write_buf + 25);
		
		if(len >= 0) {
			if(len > 0) {
				
				switch(gs_write_user_data_mode)
				{
				case USER_DATA_MODE_UCM:
					/* CMD(1) + UC（USER_CODE_LEN） + 数据（n） */
					APP_DEBUG("Gateway ucm user data = ");
					lora_net_debug_hex( gs_usart_write_buf + 1, len, 1);
					
					/* 用户数据格式  */
					/* CMD(1) + 源地址（12） + 本机地址（12） + 数据（n） */
					gs_usart_write_buf[0] = USART_API_SEND_USER_DATA_UCM | 0x80;
					//TX
					stm32_dma_usart2_write(gs_usart_write_buf,  len + 1);
					break;
				
				case USER_DATA_MODE_DEFAULT:
					/* CMD(1) + 源地址（12） + 本机地址（12） + 数据（n） */
					APP_DEBUG("Gateway user data = ");
					lora_net_debug_hex( gs_usart_write_buf + 25, len, 1);
					
					/* 用户数据格式  */
					/* CMD(1) + 源地址（12） + 本机地址（12） + 数据（n） */
					gs_usart_write_buf[0] = USART_API_SEND_USER_DATA | 0x80;
					memcpy( &gs_usart_write_buf[1], flash_config->gmac, 12);
					memcpy( &gs_usart_write_buf[13], nmac, 12);
					//TX
					stm32_dma_usart2_write(gs_usart_write_buf,  len + 25);
					break;
				
				default:
					break;
				}			
				
			} else {
				APP_DEBUG("Gateway is live \r\n");
			}
			
			if ( NODE_BUST_FLAG == DEVICE_NO_BUSY ) {
				lora_net_User_data(netp, gs_write_buf, 0);
			} else {
				
				switch(gs_write_user_data_mode)
				{
				case USER_DATA_MODE_UCM:
					/* CMD(1) + UC（USER_CODE_LEN） + 数据（n） */
					lora_net_User_data(netp, gs_write_buf, gs_write_len);
				
					APP_DEBUG("ucm send to gateway [%d] = ", gs_write_len);
					lora_net_debug_hex(gs_write_buf, gs_write_len, 1);
					break;
				
				case USER_DATA_MODE_DEFAULT:
					/* CMD(1) + 源地址（12） + 本机地址（12） + 数据（n） */
					lora_net_User_data(netp, gs_write_buf + 12, gs_write_len - 12);
				
					APP_DEBUG("send to gateway [%d] = ", gs_write_len - 12);
					lora_net_debug_hex(gs_write_buf + 12, gs_write_len - 12, 1);
					break;
				
				default:
					break;
				}
				
				
		
				NODE_BUST_FLAG = DEVICE_NO_BUSY;
			}
				
			timeout = TickCounter;
			//refresh timer, delay timeout
			soft_timer_create(NODE_TIMER_RANDOM_MSG_TIMEOUT, 1, 1, random_message_timeout_callback, TIMEOUT_ONLINE);
	#ifdef PCB_V2
			led_rev(0);
	#else
			led_rev(2);
	#endif
		}
	
	
		break;
	case NODE_STATUS_OFFLINE:
		
		len = lora_net_Network_request_r(netp, nmac, &privateMsg);

		if( len >= 0 ) {
			timeout = TickCounter;
			node_stats = NODE_STATUS_ONLINE;
			
			//use privateMsg
			lora_net_Set_Config(netp,  &privateMsg);
			lora_net_User_data(netp, gs_write_buf, 0);
			
			//delay timeout
			soft_timer_create(NODE_TIMER_RANDOM_MSG_TIMEOUT, 1, 1, random_message_timeout_callback, TIMEOUT_ONLINE);
		}

	
		break;
	case NODE_STATUS_NOBINDED:
		/* 获取自身ID */
	
		len = lora_net_Base_station_binding_r(netp, nmac, temp_config.gmac);
		
		if( len >= 0 ) {
			
			memcpy(temp_config.gmac, netp->pack.Data + 12, 12);
			temp_config.isconfig = MAGIC_CONFIG;
			xfs_sava_cfg( (uint8_t *)&temp_config, sizeof(tConfig) );
			
			if(memcmp(&temp_config, flash_config, sizeof(tConfig)) == 0) {
				//check is save  ^_^  .
				APP_DEBUG("[ flash ] save gmac = ");
				lora_net_debug_hex(flash_config->gmac, 12, 1);
				node_stats = NODE_STATUS_OFFLINE;
			}
			
			timeout = TickCounter;
			
			//refresh timer, delay timeout
			soft_timer_create(NODE_TIMER_RANDOM_MSG_TIMEOUT, 1, 1, random_message_timeout_callback, random_getRandomTime(random_get_value()) );
		}
		
		
		break;
	default:
		APP_ERROR("[bug] node_stats failed\r\n");
		break;
	}
}


#ifdef PROJECT_COMMON
	int main_node_core(void)
#else
	int main(void)
#endif
{
	int i;
	
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	systick_init();
	leds_init();
	
#ifdef PCB_V2
	led_on(0);
	led_off(1);
#endif
	
    interface_usart_init();
	usart_rx_isr_init();
	stm32_dma_init();
	
	random_adc_config();
	APP_DEBUG("USART1_Config\r\n");
	APP_DEBUG("Build , %s %s \r\n", __DATE__, __TIME__);
	
	APP_DEBUG("NODE MODULE , NMAC = ");
	lora_net_debug_hex(nmac, 12, 1);
	
	for(i=0;i<LORA_MODEL_NUM;i++) {
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
			lora[i].lora_net_rx_callback = lora_message_callback;
		}
		
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
	
	soft_timer_init();
	
	soft_timer_create(NODE_TIMER_LEDS, 1, 1, led_status_callback, 200);  //200ms
	soft_timer_create(NODE_TIMER_RANDOM_MSG_TIMEOUT, 1, 1, random_message_timeout_callback, random_getRandomTime(random_get_value()) );  //random ms
	
	
	while(1) {
		
		lora_net_proc(lora, LORA_MODEL_NUM);    //LORA接收数据处理
		usart_rx_proc(usart_rx_callback);      //串口接收数据处理
		keys_proc(user_key_proc);
		soft_timer_proc();                     //定时器处理
		
		/* end poll */
		
	}
}
