/* LORANET GATEWAY 2018 06 27 END */
/* By hxdyxd */

#include "stm32f10x.h"
#include "lora_net.h"
#include "string.h"
#include "sx1276-LoRa.h"
#include "type.h"
#include "random_freq.h"
#include "systick.h"
#include "leds.h"
#include "app_debug.h"
#include "interface_usart.h"
#include "usart_rx.h"

#define LORA_MODE_NUM (2)

#define ONLINE_NUM_DEC(i) do{\
							gs_online_num--;\
							TableMsg[i].online = 0;\
							memset(TableMsg[i].nmac, 0, 12);\
						}while(0)
/*
 * id
*/
uint8_t *gmac = (uint8_t *)0x1FFFF7E8;

/*
 *  model message
*/
LORA_NET lora[LORA_MODE_NUM ];

/*
 *  node message
*/
tTableMsg TableMsg[LORA_MAX_NODE_NUM];
uint8_t gs_online_num = 0;
uint8_t current_id = 0;

/*
 *  user_data
*/
uint8_t current_use_user_id = 0;
uint64_t RXtimer = 0;

/*
* timer
*/
uint64_t Timer1 = 0;
uint64_t Timer2 = 0;

void TableMsgDeinit()
{
	int i;
	for(i=0;i<LORA_MAX_NODE_NUM;i++) {
		TableMsg[i].online = 0;
		TableMsg[i].HoppingFrequencieSeed = random_get_value();    // RFFrequency
		TableMsg[i].SpreadingFactor = 9;        // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
		TableMsg[i].SignalBw = 8;               // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
												// 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
		TableMsg[i].ErrorCoding = 2;            // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
		TableMsg[i].RxPacketRssiValue = 0;
		memset(TableMsg[i].nmac, 0, 12);            //macaddr
	}
}


void call_config(uint8_t *gmac)
{
	int len;
	int i;
	/* 如果ID未分配完 */
	if(gs_online_num < LORA_MAX_NODE_NUM) {
		
		//find free current index
		current_id = LORA_MAX_NODE_NUM;
		for(i=0;i<LORA_MAX_NODE_NUM;i++) {
			if(TableMsg[i].online == 0) {
				current_id = i;
				break;
			}
		}
		
		if(i == LORA_MAX_NODE_NUM) {
			APP_ERROR("[bug]  gs_online_num = %d\r\n", gs_online_num);
		}
		
		/* 等待接收完成 */
		TableMsg[current_id].HoppingFrequencieSeed = random_get_value();
		len = lora_net_Gateway_Network_request(&lora[1], &TableMsg[current_id], gmac);
		if( len == 19) {
			APP_DEBUG("[new] device = %d\r\n", current_id);
			TableMsg[current_id].LastActive = TickCounter;
			TableMsg[current_id].online = 1;
			gs_online_num++;
			
			//same nmac only one
			for(i=0;i<LORA_MAX_NODE_NUM;i++) {
				if(i==current_id) {
					continue;
				}
				
				if(memcmp(TableMsg[current_id].nmac, TableMsg[i].nmac, 12) == 0 && TableMsg[i].online == 1) {
					//refresh
					APP_DEBUG("[refresh]  node = ");
					lora_net_debug_hex(TableMsg[i].nmac, 12, 1);
					
					ONLINE_NUM_DEC(i);
				}
			}
			
		}
	} else {
		// lost
		if( lora[1].RX_FLAG ) {
			lora[1].RX_FLAG = 0;
		}
	}
}

void checkOnline()
{
	int i;
	for(i=0;i<LORA_MAX_NODE_NUM;i++) {
		if( TableMsg[i].online == 1 && (TickCounter - TableMsg[i].LastActive) > TIMEOUT_ONLINE) {
			//offline
			APP_DEBUG("[offline]  node = ");
			lora_net_debug_hex(TableMsg[i].nmac, 12, 1);
			
			ONLINE_NUM_DEC(i);
		}
	}
}

void nextOnlineCurrentUserId()
{
	int i = 0;
	checkOnline();
	if(gs_online_num == 0) {
		return;
	}
	
	for(current_use_user_id++ ;; current_use_user_id++, i++) {
		if(current_use_user_id >= LORA_MAX_NODE_NUM) {
			current_use_user_id = 0;
		}
		
		if( TableMsg[current_use_user_id].online ) {
			return;
		}
		
		if(i > LORA_MAX_NODE_NUM) {
			APP_ERROR("[bug]  gs_online_num = %d\r\n", gs_online_num);
			return;
		}
	}
}


void lora_put_data_callback(void)
{
	uint8_t len;
	/* next user id */
	nextOnlineCurrentUserId();
	
	len = usart_rx_get_length();
	if (len == 0) {
		lora_net_Gateway_User_data(&lora[0], usart_rx_get_buffer_ptr(), 0, &TableMsg[current_use_user_id]);
	} else {
		lora_net_Gateway_User_data(&lora[0], usart_rx_get_buffer_ptr(), len, &TableMsg[current_use_user_id]);
		
		APP_DEBUG("usart dat [%d] = ", len);
		lora_net_debug_hex(usart_rx_get_buffer_ptr(), len, 1);
		
		usart_rx_release();
	}
}


int main(void)
{
	LORA_ROUTE_PACK pack;
	int len = 0;
	int i;
	int led_status = 0;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
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
	
	APP_DEBUG("GATEWAY MODULE , GMAC = ");
	lora_net_debug_hex(gmac, 12, 1);
	
	TableMsgDeinit();
	
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
			
			lora[i].loraConfigure.LoRaSettings.SignalBw = TableMsg[1].SignalBw;
			lora[i].loraConfigure.LoRaSettings.ErrorCoding = TableMsg[1].ErrorCoding;
			lora[i].loraConfigure.LoRaSettings.SpreadingFactor = TableMsg[1].SpreadingFactor;
			
			//memcpy(lora[i].loraConfigure.HoppingFrequencies, PublicHoppingFrequencies, sizeof(PublicHoppingFrequencies));
			lora[i].loraConfigure.HoppingFrequencieSeed = 0;
			lora[i].loraConfigure.random_getRandomFreq = random_getRandomFreq;
			lora[i].loraConfigure.LoRaSettings.FreqHopOn = true;
			
		} else if(i == 1) {
			
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276InitIo = SX1276InitIo2;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276Read = SX1276Read2;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276Write = SX1276Write2;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276ReadBuffer = SX1276ReadBuffer2;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276WriteBuffer = SX1276WriteBuffer2;
			lora[i].loraConfigure.LoraLowLevelFunc.read_single_reg = read_single_reg2;
			
			
			lora[i].loraConfigure.LoRaSettings.RFFrequency = 430000000;    // RFFrequency
			lora[i].loraConfigure.LoRaSettings.SpreadingFactor = 10;        // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
			lora[i].loraConfigure.LoRaSettings.SignalBw = 9;               // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
												// 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
			lora[i].loraConfigure.LoRaSettings.ErrorCoding = 4;            // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
			
			
			lora[i].loraConfigure.HoppingFrequencieSeed = 0;
			lora[i].loraConfigure.random_getRandomFreq = random_getRandomFreq;
			lora[i].loraConfigure.LoRaSettings.FreqHopOn = true;
		}
		


		lora[i].loraConfigure.LoRaSettings.TxPacketTimeout = 1000;
		lora[i].loraConfigure.LoRaSettings.RxPacketTimeout = 1000;
		lora[i].RX_FLAG = 0;


		
		lora_net_init(&lora[i]);
	}
	
	while(1) {
		
		lora_net_proc(lora, LORA_MODE_NUM);
		usart_rx_proc();
		
		
		if( lora[0].RX_FLAG ) {	
			//private msg callback
			
			len = lora_net_read_no_block( &lora[0], (uint8_t *)&pack);
			
			TableMsg[current_use_user_id].LastActive = TickCounter;
			TableMsg[current_use_user_id].RxPacketSnrValue = SX1276LoRaGetPacketSnr( &lora[0].loraConfigure );
			TableMsg[current_use_user_id].RxPacketRssiValue = SX1276LoRaGetPacketRssi( &lora[0].loraConfigure );
			
			if(len - 1 > 0) {
				/* 用户数据格式  */
				/* 源地址（12） + 本机地址（12） + 数据（n） */
				interface_usart_write(TableMsg[current_use_user_id].nmac , 12);
				interface_usart_write(gmac , 12);
				interface_usart_write(pack.Data, len - 1);
				APP_DEBUG("Node %d data = ", current_use_user_id);
				lora_net_debug_hex(pack.Data, len - 1, 1);
			} else {
				APP_DEBUG("Node %d is Live \r\n", current_use_user_id);
			}


			//APP_DEBUG("time = %d ms Rssi: %g Snr = %d dB\033[0m\r\n",(int)(TickCounter - RXtimer), TableMsg[current_use_user_id].RxPacketRssiValue, TableMsg[current_use_user_id].RxPacketSnrValue);
			
			lora_put_data_callback();
			RXtimer = TickCounter;
			
		#ifdef PCB_V2
			led_rev(0);  //red
		#endif
		}
		
		if( lora[1].RX_FLAG ) {
			//public msg callback
			
			call_config(gmac);
		}
		
		if( (TickCounter - RXtimer) > PACK_TIMEOUT ) {
			
			//private msg timeout callback
			
			if( gs_online_num ) {
				APP_WARN("NODE %d Timeout use %d\r\n", current_use_user_id, TableMsg[current_use_user_id].HoppingFrequencieSeed);
			
				lora_put_data_callback();
				RXtimer = TickCounter;
			}
			
		}
		
		if( (TickCounter - Timer1) > 5000 ) {
			Timer1 = TickCounter;
			//5s callback
			
			APP_DEBUG("ONLINE NUM \t = \t %d \r\n", gs_online_num);
		}
		
		if( (TickCounter - Timer2) > 200 ) {
			Timer2 = TickCounter;
			//200ms callback
			
			
			if(gs_online_num == 0) {
				led_status = 0;
			} else if( led_status && i<=0 ) {
				i = 15;
				led_status = 0;
			} else if( led_status==0 && i<=0 ) {
				i = gs_online_num * 2;
				led_status = 1;
			}
			if( i>=0 ) {
				i--;
			}
			
			if(led_status) {
				
		#ifdef PCB_V2
				led_rev(1);  //blue
		#else
				led_rev(2);
		#endif
				
			} else {
				
		#ifdef PCB_V2
				led_off(1);  //blue low = off
		#else
				led_on(2);  //high = off
		#endif	
				
			}
		}
		
		
		/* end poll */
	}
}

