/* LORANET GATEWAY 2018 06 27 END */
/* By hxdyxd */

#include "stm32f10x.h"
#include "debug_usart.h"
#include "sx1276.h"
#include "lora_net.h"
#include "string.h"
#include "sx1276-LoRa.h"
#include "type.h"
#include "random_freq.h"
#include "systick.h"
#include "leds.h"
#include "app_debug.h"

#define LORA_MODE_NUM (2)
#define LORA_MAX_NODE_NUM (16)

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
		TableMsg[i].HoppingFrequencieSeed = random_getRandomFreq(TickCounter, i);    // RFFrequency
		TableMsg[i].SpreadingFactor = 9;        // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
		TableMsg[i].SignalBw = 8;               // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
												// 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
		TableMsg[i].ErrorCoding = 4;            // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
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
		/* 等待接收完成 */
		TableMsg[current_id].HoppingFrequencieSeed = random_getRandomTime(TickCounter);
		len = lora_net_Gateway_Network_request(&lora[1], &TableMsg[current_id], gmac);
		if( len == 19) {
			APP_DEBUG("[new] device = %d\r\n", current_id);
			TableMsg[current_id].LastActive = TickCounter;
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
					
					TableMsg[i].online = 0;
					memset(TableMsg[i].nmac, 0, 12);            //macaddr
					gs_online_num--;
				}
			}
			
			//find free index
			current_id = LORA_MAX_NODE_NUM;
			for(i=0;i<LORA_MAX_NODE_NUM;i++) {
				if(TableMsg[i].online == 0) {
					current_id = i;
					break;
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
			
			TableMsg[i].online = 0;
			memset(TableMsg[i].nmac, 0, 12);            //macaddr
			gs_online_num--;
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

int main(void)
{
	LORA_ROUTE_PACK pack;
	int len = 0;
	int i;
	uint8_t *gmac = (uint8_t *)0x1FFFF7E8;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	systick_init();
	leds_init();
    USART1_Config();
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
		//lora[i].len = 0;
		lora[i].RX_FLAG = 0;
		
		lora[i].TickCounter = &TickCounter;

		
		lora_net_init(&lora[i]);
	}
	
	while(1) {
		
		lora_net_proc(lora, LORA_MODE_NUM);
		
		if( lora[0].RX_FLAG ) {	
			//private msg callback
			
			len = lora_net_read_no_block( &lora[0], (uint8_t *)&pack);
			
			TableMsg[current_use_user_id].LastActive = TickCounter;

			APP_DEBUG("NODE %d DAT = ", current_use_user_id);
			lora_net_debug_hex(pack.Data, len - 1, 1);
			APP_DEBUG("TIME = %d ms Rssi: %f\033[0m\r\n",(int)(TickCounter - RXtimer), SX1276LoRaReadRssi( &lora[0].loraConfigure) );
			
			nextOnlineCurrentUserId();
			lora_net_Gateway_User_data(&lora[0], gmac, 12, &TableMsg[current_use_user_id]);
			RXtimer = TickCounter;
		}
		
		if( lora[1].RX_FLAG ) {
			//public msg callback
			
			call_config(gmac);
		}
		
		if( (TickCounter - RXtimer) > PACK_TIMEOUT ) {
			
			//private msg timeout callback
			
			if( gs_online_num ) {
				APP_WARN("NODE %d Timeout use %d\r\n", current_use_user_id, TableMsg[current_use_user_id].HoppingFrequencieSeed);
			
				nextOnlineCurrentUserId();
				lora_net_Gateway_User_data(&lora[0], gmac, 12, &TableMsg[current_use_user_id]);
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
			
			led_rev(0);
		}
		
		
		/* end poll */
	}
}

