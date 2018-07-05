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


void lora_net_proc(LORA_NET *netp, int lora_num)
{
	uint32_t stat;
	int i;
	for(i=0;i<lora_num;i++) {
		stat = SX1276LoRaProcess(&netp[i].loraConfigure);
		switch(stat)
		{
		case RF_RX_DONE:
			if( !netp[i].RX_FLAG ) {
				//SX1276LoRaGetRxPacket(&netp[i].loraConfigure, netp[i].buf, &netp[i].len);
				netp[i].RX_FLAG = 1;
			} else {
				APP_ERROR("pack lost = %d\r\n", i);
			}
			break;
		case RF_TX_DONE:
			SX1276LoRaStartRx(&netp[i].loraConfigure);
			break;
		default:
			//APP_ERROR("stat = %d\r\n", stat);
			break;
		}
	}
}

int lora_net_read_no_block(LORA_NET *netp, uint8_t *buffer)
{
	uint16_t len;
	//memcpy(buffer, netp->buf, netp->len);
	//APP_DEBUG(" test = %d, %d\r\n", netp->RxTimeout, netp->TxTimeout);
	
	SX1276LoRaGetRxPacket(&netp->loraConfigure, buffer, &len);
	netp->RX_FLAG = 0;
	return len;
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

int lora_net_write(LORA_NET *netp, uint8_t *buffer, uint8_t len)
{
	uint32_t stat;
	uint64_t timer = TickCounter;
	SX1276LoRaSetTxPacket(&netp->loraConfigure, buffer, len);
	while( (stat = SX1276LoRaProcess(&netp->loraConfigure) ) != RF_TX_DONE) {
//		if(stat == netp->RF_TX_TIMEOUT) {
//			return -1;
//		}
		if( (TickCounter - timer) > PACK_TIMEOUT) {
			return -1;
		}
	}
	return len;
}

int lora_net_read(LORA_NET *netp, uint8_t *buffer)
{
	uint32_t stat;
	uint16_t len;
	uint64_t timer = TickCounter;
	
	SX1276LoRaStartRx(&netp->loraConfigure);
//	APP_DEBUG(" test = %d, %d\r\n", netp->RxTimeout, netp->TxTimeout);
	while( ( stat = SX1276LoRaProcess(&netp->loraConfigure) ) != RF_RX_DONE) {
//		if(stat == netp->RF_RX_TIMEOUT) {
//			return -1;
//		}
		if( (TickCounter - timer) > PACK_TIMEOUT) {
			return -1;
		}
	}
//	APP_DEBUG(" test = %d, %d\r\n", netp->RxTimeout, netp->TxTimeout);
	SX1276LoRaGetRxPacket(&netp->loraConfigure, buffer, &len);
	return len;
}

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
	int stat;
	//uint64_t timer = *(netp->TickCounter);
//	
	netp->loraConfigure.HoppingFrequencieSeed = msg->HoppingFrequencieSeed;
	netp->loraConfigure.LoRaSettings.SignalBw = msg->SignalBw;
	netp->loraConfigure.LoRaSettings.SpreadingFactor = msg->SpreadingFactor;
	netp->loraConfigure.LoRaSettings.ErrorCoding = msg->ErrorCoding;
	//netp->SX1276Init(&netp->loraConfigure);
	
	//SX1276LoRaSetRFFrequency( &netp->loraConfigure, netp->loraConfigure.HoppingFrequencieSeed );
	SX1276LoRaSetSpreadingFactor( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SpreadingFactor ); // SF6 only operates in implicit header mode.
	SX1276LoRaSetErrorCoding( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.ErrorCoding );
	SX1276LoRaSetSignalBandwidth( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SignalBw );
//	
	netp->pack.Flag_version = FLAG_VER;
	netp->pack.Flag_type = FLAG_TYPE_USER_DATA;
	netp->pack.Flag_direction = FLAG_DIR_DOWN;
	memcpy(netp->pack.Data, buffer, len);
	
	stat = lora_net_write_no_block(netp, (uint8_t *)&netp->pack, len + 1);
	if(stat < 0) {
		APP_ERROR("Send USER_DATA to NODE failed , use %d\r\n", netp->loraConfigure.HoppingFrequencieSeed);
	} else {
		//APP_DEBUG("Send USER_DATA to NODE , use %d \r\n", netp->loraConfigure.HoppingFrequencieSeed);
	}
	
}


int lora_net_Gateway_Network_request(LORA_NET *netp, tTableMsg *msg, uint8_t *gmac)
{
	int n;
	uint32_t timer = TickCounter;
	
	n = lora_net_read_no_block(netp, (uint8_t *)&netp->pack);
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
}


int lora_net_User_data(LORA_NET *netp, uint8_t *buffer, uint8_t len)
{
	int n;
	
	SX1276LoRaSetSpreadingFactor( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SpreadingFactor ); // SF6 only operates in implicit header mode.
	SX1276LoRaSetErrorCoding( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.ErrorCoding );
	SX1276LoRaSetSignalBandwidth( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SignalBw );
	
	n = lora_net_read(netp, (uint8_t *)&netp->pack);
	
	if(n < 0) {
		APP_WARN("Timeout use %d\r\n", netp->loraConfigure.HoppingFrequencieSeed);
	} else {
		
		if(netp->pack.Flag_version == FLAG_VER && netp->pack.Flag_direction == FLAG_DIR_DOWN && netp->pack.Flag_type == FLAG_TYPE_USER_DATA) {
			
			APP_DEBUG("GATEWAY USER DATA = ");
			lora_net_debug_hex( netp->pack.Data, n - 1, 1);
			
			netp->pack.Flag_version = FLAG_VER;
			netp->pack.Flag_type = FLAG_TYPE_USER_DATA;
			netp->pack.Flag_direction = FLAG_DIR_UP;
			
			memcpy( netp->pack.Data, buffer, len);
			lora_net_write(netp, (uint8_t *)&netp->pack, len + 1);
			
			return (len + 1);
		}
		APP_WARN(" Flag_version = %d, Flag_type = %d, Flag_direction = %d, len = %d \r\n", netp->pack.Flag_version, netp->pack.Flag_type, netp->pack.Flag_direction, n);
	}
	return 0;
}


int lora_net_Network_request(LORA_NET *netp, uint8_t *nmac, uint8_t *gmac, tTableMsg *msg)
{
	int stat;
	int len;
	uint64_t timer = TickCounter;
	
	SX1276LoRaSetSpreadingFactor( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SpreadingFactor ); // SF6 only operates in implicit header mode.
	SX1276LoRaSetErrorCoding( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.ErrorCoding );
	SX1276LoRaSetSignalBandwidth( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SignalBw );
	
	netp->pack.Flag_version = FLAG_VER;
	netp->pack.Flag_type = FLAG_TYPE_NETWORK_REQUEST;
	netp->pack.Flag_direction = FLAG_DIR_UP;
	
	memcpy( netp->pack.Data, nmac, 12);
	memcpy( netp->pack.Data + 12, gmac, 12);
	
	stat = lora_net_write(netp, (uint8_t *)&netp->pack, 25);
	if(stat < 0) {
		APP_ERROR("Send NETWORK_REQUEST to GATEWAY failed \r\n");
	} else {
		APP_DEBUG("Send NETWORK_REQUEST to GATEWAY \r\n");
	}
	
	len = lora_net_read(netp, (uint8_t *)&netp->pack);
	if(len < 0) {
		APP_WARN("Timeout use %d\r\n", netp->loraConfigure.HoppingFrequencieSeed);
	} else {
		if(netp->pack.Flag_version == FLAG_VER && netp->pack.Flag_direction == FLAG_DIR_DOWN && netp->pack.Flag_type == FLAG_TYPE_NETWORK_REQUEST && len == 19) {
			if(memcmp( nmac, netp->pack.Data, 12) == 0) { /* give me */
				
				msg->HoppingFrequencieSeed = (netp->pack.Data[12] << 24) | (netp->pack.Data[13] << 16) | (netp->pack.Data[14] << 8) | (netp->pack.Data[15]);
				msg->SpreadingFactor =  (netp->pack.Data[16] & 0xf0) >> 4;
				msg->SignalBw = netp->pack.Data[16] & 0x0f;
				msg->ErrorCoding = netp->pack.Data[17] >> 5;
				

				APP_DEBUG(" Seed = %d \r\n", msg->HoppingFrequencieSeed);
				APP_DEBUG(" Bw = %d \r\n", msg->SignalBw);
				APP_DEBUG(" Sf = %d \r\n", msg->SpreadingFactor);
				APP_DEBUG(" Ec = %d - %02x\r\n", msg->ErrorCoding, netp->pack.Data[17]);
				APP_DEBUG(" TIME = %d ms Rssi: %f\r\n", (int)(TickCounter - timer), SX1276LoRaReadRssi( &netp->loraConfigure) );
				
				return (len - 2);
			}
		}
		APP_WARN(" Flag_version = %d, Flag_type = %d, Flag_direction = %d \r\n", netp->pack.Flag_version, netp->pack.Flag_type, netp->pack.Flag_direction);
	}
	
	
	return 0;
}

int lora_net_Base_station_binding(LORA_NET *netp, uint8_t *nmac, uint8_t *gmac)
{
	int stat;
	int len;
	uint64_t timer = TickCounter;
	
	SX1276LoRaSetSpreadingFactor( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SpreadingFactor ); // SF6 only operates in implicit header mode.
	SX1276LoRaSetErrorCoding( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.ErrorCoding );
	SX1276LoRaSetSignalBandwidth( &netp->loraConfigure, netp->loraConfigure.LoRaSettings.SignalBw );
	
	netp->pack.Flag_version = FLAG_VER;
	netp->pack.Flag_type = FLAG_TYPE_BASE_STATION_BINDING;
	netp->pack.Flag_direction = FLAG_DIR_UP;
	
	memcpy( netp->pack.Data, nmac, 12);
	
	stat = lora_net_write(netp, (uint8_t *)&netp->pack, 13);
	if(stat < 0) {
		APP_ERROR("Send BASE_STATION_BINDING to GATEWAY failed \r\n");
	} else {
		APP_DEBUG("Send BASE_STATION_BINDING to GATEWAY \r\n");
	}
	
	len = lora_net_read(netp, (uint8_t *)&netp->pack);
	if(len < 0) {
		APP_WARN("Timeout use %d\r\n", netp->loraConfigure.HoppingFrequencieSeed);
	} else {
		if(netp->pack.Flag_version == FLAG_VER && netp->pack.Flag_direction == FLAG_DIR_DOWN && netp->pack.Flag_type == FLAG_TYPE_BASE_STATION_BINDING && len == 25) {
			if(memcmp( nmac, netp->pack.Data, 12) == 0) { /* give me */
				memcpy(gmac, netp->pack.Data + 12, 12);
				
				APP_DEBUG("GMAC = ");
				lora_net_debug_hex(gmac, 12, 1);
				APP_DEBUG("TIME = %d ms Rssi: %f\r\n", (int)(TickCounter - timer), SX1276LoRaReadRssi( &netp->loraConfigure) );
				
				return (len);
			}
		}
		APP_WARN(" Flag_version = %d, Flag_type = %d, Flag_direction = %d \r\n", netp->pack.Flag_version, netp->pack.Flag_type, netp->pack.Flag_direction);
	}
	
	
	return 0;
}




