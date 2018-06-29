#ifndef _TYPE_H
#define _TYPE_H

#include <stdint.h>

typedef struct sTableMsg
{
	uint8_t online;
	uint8_t nmac[12];
	uint32_t HoppingFrequencieSeed;
	uint8_t SignalBw;
	uint8_t SpreadingFactor;
	uint8_t ErrorCoding;
	uint64_t LastActive;
	double RxPacketRssiValue;
}tTableMsg;


typedef struct sConfig
{
	uint32_t isconfig;
	uint8_t gmac[12];
}tConfig;	

#define MAGIC_CONFIG  (0x78549261)

#define TIMEOUT_ONLINE (60000)   //60s

#define PACK_TIMEOUT   (1000)   //1s


#endif
