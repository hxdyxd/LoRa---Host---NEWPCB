/* LORANET 2018 06 27 END */
/* By hxdyxd */
#include "lora_net.h"
#include "type.h"



int lora_net_init(LORA_NET *netp)
{
	SX1276LoRaInit(&netp->loraConfigure);
	SX1276LoRaStartRx(&netp->loraConfigure);
	return 0;
}


void lora_net_proc(LORA_NET *netp, int lora_max_num)
{
	uint32_t stat;
	int i;
	for(i=0;i<lora_max_num;i++) {
		stat = SX1276LoRaProcess(&netp[i].loraConfigure);
		switch(stat)
		{
		case RF_RX_DONE:
			SX1276LoRaGetRxPacket( &netp[i].loraConfigure, &netp[i].pack, &netp[i].pack.size );
			netp[i].lora_net_rx_callback( &netp[i] );   //User callback function
			break;
		case RF_TX_DONE:
			SX1276LoRaStartRx( &netp[i].loraConfigure );
			break;
		default:
			//APP_ERROR("stat = %d\r\n", stat);
			break;
		}
	}
}


int lora_net_write_no_block(LORA_NET *netp, uint8_t *buffer, uint8_t len)
{
	//APP_DEBUG(" test = %d, %d\r\n", netp->RxTimeout, netp->TxTimeout);
	if(len > RF_BUFFER_SIZE - 1) {
		len = RF_BUFFER_SIZE - 1;
	}
	SX1276LoRaSetTxPacket(&netp->loraConfigure, buffer, len);
	return len;
}

//int lora_net_write(LORA_NET *netp, uint8_t *buffer, uint8_t len)
//{
//	uint32_t stat;
//	uint64_t timer = TickCounter;
//	SX1276LoRaSetTxPacket(&netp->loraConfigure, buffer, len);
//	while( (stat = SX1276LoRaProcess(&netp->loraConfigure) ) != RF_TX_DONE) {
////		if(stat == netp->RF_TX_TIMEOUT) {
////			return -1;
////		}
//		if( (TickCounter - timer) > PACK_TIMEOUT) {
//			return -1;
//		}
//	}
//	return len;
//}

//int lora_net_read(LORA_NET *netp, uint8_t *buffer)
//{
//	uint32_t stat;
//	uint16_t len;
//	uint64_t timer = TickCounter;
//	
//	SX1276LoRaStartRx(&netp->loraConfigure);
////	APP_DEBUG(" test = %d, %d\r\n", netp->RxTimeout, netp->TxTimeout);
//	while( ( stat = SX1276LoRaProcess(&netp->loraConfigure) ) != RF_RX_DONE) {
////		if(stat == netp->RF_RX_TIMEOUT) {
////			return -1;
////		}
//		if( (TickCounter - timer) > PACK_TIMEOUT) {
//			return -1;
//		}
//	}
////	APP_DEBUG(" test = %d, %d\r\n", netp->RxTimeout, netp->TxTimeout);
//	SX1276LoRaGetRxPacket(&netp->loraConfigure, buffer, &len);
//	return len;
//}

void lora_net_debug_hex(uint8_t *p, uint8_t len, uint8_t lf)
{
	int i;
	for(i=0;i<len;i++) {
		printf("%02x", *p );
		p++;
	}
	if(lf) {
		printf("\r\n");
	}
}

void lora_net_Gateway_User_data(LORA_NET *netp, uint8_t *buffer, uint8_t len, tTableMsg *msg)
{
	netp->loraConfigure.HoppingFrequencieSeed = msg->HoppingFrequencieSeed;
	netp->loraConfigure.LoRaSettings.SignalBw = msg->SignalBw;
	netp->loraConfigure.LoRaSettings.SpreadingFactor = msg->SpreadingFactor;
	netp->loraConfigure.LoRaSettings.ErrorCoding = msg->ErrorCoding;
	
	SX1276LoRaSetSpreadingFactor( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SpreadingFactor ); // SF6 only operates in implicit header mode.
	SX1276LoRaSetErrorCoding( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.ErrorCoding );
	SX1276LoRaSetSignalBandwidth( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SignalBw );
	
	netp->pack.Flag_version = FLAG_VER;
	netp->pack.Flag_type = FLAG_TYPE_USER_DATA;
	netp->pack.Flag_direction = FLAG_DIR_DOWN;
	memcpy(netp->pack.Data, buffer, len);
	
	lora_net_write_no_block(netp, (uint8_t *)&netp->pack, len + 1);
}


int lora_net_Gateway_Network_request(LORA_NET *netp, tTableMsg *msg, uint8_t *gmac)
{
	int n = netp->pack.size;
	uint32_t timer = TickCounter;
	
	if(netp->pack.Flag_version == FLAG_VER && netp->pack.Flag_direction == FLAG_DIR_UP && netp->pack.Flag_type == FLAG_TYPE_NETWORK_REQUEST && n == 25) {
		if(memcmp( gmac, netp->pack.Data + 12, 12) == 0) { /* give me */
			memcpy( msg->nmac, netp->pack.Data, 12);
			
			APP_DEBUG("NMAC = ");
			lora_net_debug_hex(msg->nmac, 12, 1);
			APP_DEBUG("TIME = %d ms Rssi: %f\r\n", (int)(TickCounter - timer), SX1276LoRaReadRssi( &netp->loraConfigure) );
			
			/******************** */
			netp->pack.Flag_version = FLAG_VER;
			netp->pack.Flag_type = FLAG_TYPE_NETWORK_REQUEST;
			netp->pack.Flag_direction = FLAG_DIR_DOWN;
			
			
			memcpy( netp->pack.Data, msg->nmac, 12);
			netp->pack.Data[12] = msg->HoppingFrequencieSeed >> 24;
			netp->pack.Data[13] = msg->HoppingFrequencieSeed >> 16;
			netp->pack.Data[14] = msg->HoppingFrequencieSeed >> 8;
			netp->pack.Data[15] = msg->HoppingFrequencieSeed;
			
			netp->pack.Data[16] = (msg->SpreadingFactor << 4) | msg->SignalBw;
			netp->pack.Data[17] = (msg->ErrorCoding << 5);
			
			lora_net_write_no_block(netp, (uint8_t *)&netp->pack, 19);
			
			APP_DEBUG(" Seed = %d \r\n", msg->HoppingFrequencieSeed );
			APP_DEBUG(" Bw = %d \r\n", msg->SignalBw );
			APP_DEBUG(" Sf = %d \r\n", msg->SpreadingFactor );
			APP_DEBUG(" Ec = %d \r\n", msg->ErrorCoding );
			return (19);
		}
	} else if(netp->pack.Flag_version == FLAG_VER && netp->pack.Flag_direction == FLAG_DIR_UP && netp->pack.Flag_type == FLAG_TYPE_BASE_STATION_BINDING && n == 13) {
		netp->pack.Flag_direction = FLAG_DIR_DOWN;
		memcpy( netp->pack.Data + 12, gmac, 12);
		
		lora_net_write_no_block(netp, (uint8_t *)&netp->pack, 25);
		
		APP_DEBUG("BASE_STATION_BINDING , NMAC = ");
		lora_net_debug_hex( netp->pack.Data, 12, 1);
		APP_DEBUG("TIME = %d ms Rssi: %f\r\n", (int)(TickCounter - timer), SX1276LoRaReadRssi( &netp->loraConfigure) );
		
		return (25);
	}
	APP_WARN("[Public] Flag_version = %d, Flag_type = %d, Flag_direction = %d, len = %d \r\n", netp->pack.Flag_version, netp->pack.Flag_type, netp->pack.Flag_direction, n);
	return 0;
}

void lora_net_Set_Config(LORA_NET *netp, tTableMsg *msg)
{
	netp->loraConfigure.LoRaSettings.SpreadingFactor = msg->SpreadingFactor;        // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
	netp->loraConfigure.LoRaSettings.SignalBw = msg->SignalBw;               // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
										// 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
	netp->loraConfigure.LoRaSettings.ErrorCoding = msg->ErrorCoding;            // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
	
	netp->loraConfigure.HoppingFrequencieSeed = msg->HoppingFrequencieSeed;
	
	
	SX1276LoRaSetSpreadingFactor( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SpreadingFactor ); // SF6 only operates in implicit header mode.
	SX1276LoRaSetErrorCoding( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.ErrorCoding );
	SX1276LoRaSetSignalBandwidth( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SignalBw );
}


/* lora net pack gen */
void lora_net_User_data(LORA_NET *netp, uint8_t *send_buffer, uint8_t len)
{
	
	netp->pack.Flag_version = FLAG_VER;
	netp->pack.Flag_type = FLAG_TYPE_USER_DATA;
	netp->pack.Flag_direction = FLAG_DIR_UP;
	
	memcpy( netp->pack.Data, send_buffer, len);
	lora_net_write_no_block(netp, (uint8_t *)&netp->pack, len + 1);
}

int lora_net_User_data_r(LORA_NET *netp, uint8_t *read_buffer)
{
	int len = netp->pack.size;
	
	if(len < 1) {
		return -1;
	}

	if(netp->pack.Flag_version == FLAG_VER && netp->pack.Flag_direction == FLAG_DIR_DOWN && netp->pack.Flag_type == FLAG_TYPE_USER_DATA) {
		memcpy( read_buffer , netp->pack.Data, len - 1);
		return (len - 1);
	} else {
		APP_WARN(" Flag_version = %d, Flag_type = %d, Flag_direction = %d, len = %d \r\n", netp->pack.Flag_version, netp->pack.Flag_type, netp->pack.Flag_direction, len);
		return -1;
	}
}

void lora_net_Network_request(LORA_NET *netp, uint8_t *nmac, uint8_t *gmac)
{
	netp->pack.Flag_version = FLAG_VER;
	netp->pack.Flag_type = FLAG_TYPE_NETWORK_REQUEST;
	netp->pack.Flag_direction = FLAG_DIR_UP;
	
	memcpy( netp->pack.Data, nmac, 12);
	memcpy( netp->pack.Data + 12, gmac, 12);
	
	lora_net_write_no_block(netp, (uint8_t *)&netp->pack, 25);
}

int lora_net_Network_request_r(LORA_NET *netp, uint8_t *nmac, tTableMsg *privateMsg)
{
	int len = netp->pack.size;
	
	if(len < 1) {
		return -1;
	}
	
	if(netp->pack.Flag_version == FLAG_VER && netp->pack.Flag_direction == FLAG_DIR_DOWN && netp->pack.Flag_type == FLAG_TYPE_NETWORK_REQUEST && len == 19) {
		if(memcmp( nmac, netp->pack.Data, 12) == 0) { /* give me */
			
			privateMsg->HoppingFrequencieSeed = (netp->pack.Data[12] << 24) | (netp->pack.Data[13] << 16) | (netp->pack.Data[14] << 8) | (netp->pack.Data[15]);
			privateMsg->SpreadingFactor =  (netp->pack.Data[16] & 0xf0) >> 4;
			privateMsg->SignalBw = netp->pack.Data[16] & 0x0f;
			privateMsg->ErrorCoding = netp->pack.Data[17] >> 5;
			

			APP_DEBUG(" Seed = %d \r\n", privateMsg->HoppingFrequencieSeed);
			APP_DEBUG(" Bw = %d \r\n", privateMsg->SignalBw);
			APP_DEBUG(" Sf = %d \r\n", privateMsg->SpreadingFactor);
			APP_DEBUG(" Ec = %d \r\n", privateMsg->ErrorCoding);
			
			return 0;
		}
		
		return -1;
	} else {
		APP_WARN(" Flag_version = %d, Flag_type = %d, Flag_direction = %d \r\n", netp->pack.Flag_version, netp->pack.Flag_type, netp->pack.Flag_direction);
		return -1;
	}
}

void lora_net_Base_station_binding(LORA_NET *netp, uint8_t *nmac)
{
	netp->pack.Flag_version = FLAG_VER;
	netp->pack.Flag_type = FLAG_TYPE_BASE_STATION_BINDING;
	netp->pack.Flag_direction = FLAG_DIR_UP;
	
	memcpy( netp->pack.Data, nmac, 12);
	
	lora_net_write_no_block(netp, (uint8_t *)&netp->pack, 13);
}


int lora_net_Base_station_binding_r(LORA_NET *netp, uint8_t *nmac, uint8_t *gmac)
{
	int len = netp->pack.size;
	
	if(len < 1) {
		return -1;
	}
	
			
	if(netp->pack.Flag_version == FLAG_VER && netp->pack.Flag_direction == FLAG_DIR_DOWN && netp->pack.Flag_type == FLAG_TYPE_BASE_STATION_BINDING && len == 25) {
		if(memcmp( nmac, netp->pack.Data, 12) == 0) { /* give me */
			memcpy(gmac, netp->pack.Data + 12, 12);
			
			APP_DEBUG("[ binding ] gmac = ");
			lora_net_debug_hex(gmac, 12, 1);
			
			return 0;
		}
		
		return -1;
	} else {
		APP_WARN(" Flag_version = %d, Flag_type = %d, Flag_direction = %d \r\n", netp->pack.Flag_version, netp->pack.Flag_type, netp->pack.Flag_direction);
		return -1;
	}
}


