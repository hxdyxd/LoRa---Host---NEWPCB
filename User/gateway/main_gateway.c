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
#include "stm32_dma.h"
#include "soft_timer.h"

#define LORA_MODEL_NUM (2)

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
LORA_NET lora[LORA_MODEL_NUM ];
tTableMsg privateMsg = {
	.HoppingFrequencieSeed = 1,  //RANDOM
	.SpreadingFactor = 9,        // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
	.SignalBw = 8,               // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
										// 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
	.ErrorCoding = 3,            // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
};						
tTableMsg publicMsg = {
	.HoppingFrequencieSeed = 0,
	.SpreadingFactor = 10,        // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
	.SignalBw = 9,               // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
										// 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
	.ErrorCoding = 4,            // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
};						

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

/*
* write
*/
uint8_t gs_write_buf[256];
uint8_t gs_usart_write_buf[1024];
uint8_t gs_write_len = 0;
uint8_t GATEWAY_BUST_FLAG = 1;   //1: no busy

void TableMsgDeinit(void)
{
	int i;
	for(i=0;i<LORA_MAX_NODE_NUM;i++) {
		TableMsg[i].online = 0;
		TableMsg[i].HoppingFrequencieSeed = random_get_value();    // RFFrequency
		TableMsg[i].SpreadingFactor = privateMsg.SpreadingFactor;  // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
		TableMsg[i].SignalBw = privateMsg.SignalBw;                // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
												                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
		TableMsg[i].ErrorCoding = privateMsg.ErrorCoding;          // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
		TableMsg[i].RxPacketRssiValue = 0;
		TableMsg[i].RxPacketSnrValue = 0;
		memset(TableMsg[i].nmac, 0, 12);            //macaddr
	}
}


void checkOnline(void)
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

void nextOnlineCurrentUserId(void)
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
	
	if ( GATEWAY_BUST_FLAG == DEVICE_NO_BUSY ) {
		lora_net_Set_Config(&lora[0],  &TableMsg[current_use_user_id]);
		lora_net_Gateway_User_data(&lora[0], gs_write_buf, 0);
	} else {
		int i;
		
		for(i=0;i<LORA_MAX_NODE_NUM;i++) {
			if( memcmp(gs_write_buf, TableMsg[i].nmac, 12 ) == 0 && TableMsg[i].online ) {
				lora_net_Set_Config(&lora[0],  &TableMsg[i]);
				lora_net_Gateway_User_data(&lora[0], gs_write_buf + 12, gs_write_len - 12);
				
				APP_DEBUG("send to %d node [%d] = ", i, gs_write_len - 12);
				lora_net_debug_hex(gs_write_buf + 12, gs_write_len - 12, 1);
				
				GATEWAY_BUST_FLAG = DEVICE_NO_BUSY;
				return;
			}
		}
		
		APP_DEBUG("[offline] Can't send, usart dat [%d] = ", len);
		
		GATEWAY_BUST_FLAG = DEVICE_NO_BUSY;
	}
}

void usart_rx_callback(void)
{
	uint8_t cmd = 0;
	uint32_t fhkey;
	struct sGatewayUsartStatus usartStatus;
	uint8_t len;
	
	int i;
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
		
		gs_usart_write_buf[1] = (privateMsg.SpreadingFactor << 4) | privateMsg.SignalBw;
		gs_usart_write_buf[2] = (privateMsg.ErrorCoding << 5);
	
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
		/* CMD(1) + [ NMAC（12） + FHKEY(4) + Rssi(2) + SNR(1) ] + [ ... ] ... */
		cmd = USART_API_READ_STATUS | 0x80;
	
		interface_usart_write_wait();
		interface_usart_write( &cmd, 1);

		for(i=0;i<LORA_MAX_NODE_NUM;i++) {
			if( TableMsg[i].online ) {
				memcpy(usartStatus.nmac, TableMsg[i].nmac, 12);
				
				fhkey = TableMsg[i].HoppingFrequencieSeed;
				usartStatus.fhkey[0] = fhkey >> 24;
				usartStatus.fhkey[1] = (fhkey >> 16) & 0xff;
				usartStatus.fhkey[2] = (fhkey >> 8)  & 0xff;
				usartStatus.fhkey[3] = fhkey & 0xff;

				int16_t rssi =  TableMsg[i].RxPacketRssiValue;
				usartStatus.rssi[0] = (*((uint16_t *)&rssi) >> 8) & 0xff;
				usartStatus.rssi[1] = *((uint16_t *)&rssi) & 0xff;
				
				usartStatus.snr = (int8_t)TableMsg[i].RxPacketSnrValue;
				
				uint16_t diff_lat = (TickCounter - TableMsg[i].LastActive);
				usartStatus.diff_lat[0] = (diff_lat >> 8) & 0xff;
				usartStatus.diff_lat[1] = diff_lat & 0xff;
				
				interface_usart_write( usartStatus.nmac, sizeof(struct sGatewayUsartStatus) );
			}
		}
		usart_rx_release();
		
		APP_DEBUG("usart read status, online num = %d\r\n", gs_online_num);
		break;
	
	case USART_API_READ_BUSY:
		/* CMD(1) + STATUS（1） */
		cmd = USART_API_READ_BUSY | 0x80;
	
		interface_usart_write_wait();
		interface_usart_write( &cmd, 1);
		interface_usart_write( &GATEWAY_BUST_FLAG, 1);
		usart_rx_release();
	
		APP_DEBUG("usart busy = %d\r\n", GATEWAY_BUST_FLAG);
		break;
	
	case USART_API_SEND_USER_DATA:
		len = usart_rx_get_length();
		if( len >= 13 && GATEWAY_BUST_FLAG == 1 ) {
			gs_write_len = len - 1;
			memcpy(gs_write_buf, usart_rx_get_buffer_ptr() + 1, gs_write_len);
			GATEWAY_BUST_FLAG = 0;
		} else {
			APP_DEBUG("[ busy ] usart data [%d] = \r\n", len - 1);
		}
		
		usart_rx_release();
		break;
	default:
		
		break;
	}
}

void private_write_timeout_callback(void)
{
	if( gs_online_num ) {
		APP_WARN("Node %d Timeout use %d\r\n", current_use_user_id, TableMsg[current_use_user_id].HoppingFrequencieSeed);
	
		lora_put_data_callback();
		RXtimer = TickCounter;
	}
}


void private_message_callback(struct sLORA_NET *netp)
{
	int len = 0;
	
	len = lora_net_Gateway_User_data_r(netp, gs_usart_write_buf + 25);
	
	if(len >= 0) {
		
		TableMsg[current_use_user_id].LastActive = TickCounter;
		TableMsg[current_use_user_id].RxPacketSnrValue = SX1276LoRaGetPacketSnr( &netp->loraConfigure );
		TableMsg[current_use_user_id].RxPacketRssiValue = SX1276LoRaGetPacketRssi( &netp->loraConfigure );
		
		if(len > 0) {
			/* 用户数据格式  */
			/* CMD(1) + 源地址（12） + 本机地址（12） + 数据（n） */
			gs_usart_write_buf[0] = USART_API_SEND_USER_DATA | 0x80;
			memcpy(gs_usart_write_buf + 1, TableMsg[current_use_user_id].nmac , 12);
			memcpy(gs_usart_write_buf + 13, gmac , 12);
			stm32_dma_usart2_write(gs_usart_write_buf, len + 25);
			
			APP_DEBUG("Node %d data = ", current_use_user_id);
			lora_net_debug_hex(netp->pack.Data, len - 1, 1);
		} else {
			APP_DEBUG("Node %d is Live \r\n", current_use_user_id);
		}
		
		//APP_DEBUG("time = %d ms Rssi: %g Snr = %d dB\033[0m\r\n",(int)(TickCounter - RXtimer), TableMsg[current_use_user_id].RxPacketRssiValue, TableMsg[current_use_user_id].RxPacketSnrValue);
		
		lora_put_data_callback();
		soft_timer_create(GATEWAY_TIMER_PRIVATE_MSG_TIMEOUT, 1, 1, private_write_timeout_callback, PACK_TIMEOUT);
		RXtimer = TickCounter;
		
	#ifdef PCB_V2
		led_rev(0);  //red
	#endif
	}

}


void public_message_callback(struct sLORA_NET *netp)
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
		len = lora_net_Gateway_Network_request(netp, &TableMsg[current_id], gmac);
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
	}
	/* end */
}

void led_status_callback(void)
{
	static int led_status = 0;
	static int i;
	
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


void online_num_tips_callback(void)
{
	APP_DEBUG("ONLINE NUM \t = \t %d \r\n", gs_online_num);
}

int main(void)
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
	
	APP_DEBUG("GATEWAY MODULE , GMAC = ");
	lora_net_debug_hex(gmac, 12, 1);
	
	TableMsgDeinit();
	
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
			lora[i].lora_net_rx_callback = private_message_callback;
			
		} else {
			
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276InitIo = SX1276InitIo2;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276Read = SX1276Read2;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276Write = SX1276Write2;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276ReadBuffer = SX1276ReadBuffer2;
			lora[i].loraConfigure.LoraLowLevelFunc.SX1276WriteBuffer = SX1276WriteBuffer2;
			lora[i].loraConfigure.LoraLowLevelFunc.read_single_reg = read_single_reg2;
			
			lora[i].loraConfigure.random_getRandomFreq = random_getRandomFreq;
			lora[i].lora_net_rx_callback = public_message_callback;
		}
		
		lora_net_init(&lora[i]);
	}
	
	lora_net_Set_Config(&lora[0],  &privateMsg);
	lora_net_Set_Config(&lora[1],  &publicMsg);
	
	//定时器配置
	soft_timer_init();
	
	/* always running  */
	soft_timer_create(GATEWAY_TIMER_STATUS, 1, 1, online_num_tips_callback, 15000);  //15s
	soft_timer_create(GATEWAY_TIMER_LEDS, 1, 1, led_status_callback, 200);  //200ms
	
	/* always running, 单节点查询超时，接收到私有信道数据后重置 */
	soft_timer_create(GATEWAY_TIMER_PRIVATE_MSG_TIMEOUT, 1, 1, private_write_timeout_callback, PACK_TIMEOUT);
	
	while(1) {
		
		lora_net_proc(lora, LORA_MODEL_NUM);   //LORA接收数据处理
		usart_rx_proc(usart_rx_callback);     //串口接收数据处理
		soft_timer_proc();                    //定时器处理
		
		/* end poll */
	}
}

