/* Global Configuration 
 *  By hxdyxd 
 * 2018 07 05 CREAT
*/

#ifndef _TYPE_H
#define _TYPE_H

#include <stdint.h>

/* configuration */

//   #define PCB_V1  (1)
#define PCB_V2  (1)
//   #define PCB_V3  (1)


#define MAGIC_CONFIG  (0x78549261)


#define LORA_MAX_NODE_NUM (16)

#define TIMEOUT_ONLINE (60000)   //60s

#define PACK_TIMEOUT   (1200)   //1s



/* global define */

#define DEVICE_BUSY     (0)
#define DEVICE_NO_BUSY  (1)

#define NODE_STATUS_ONLINE      (0)
#define NODE_STATUS_OFFLINE     (1)
#define NODE_STATUS_NOBINDED    (2)

#define GATEWAY_TIMER_STATUS                  (0)
#define GATEWAY_TIMER_LEDS                    (1)
#define GATEWAY_TIMER_PRIVATE_MSG_TIMEOUT     (2)


/* global typedef */

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
	int8_t RxPacketSnrValue;
}tTableMsg;


typedef struct sConfig
{
	uint32_t isconfig;
	uint8_t gmac[12];
}tConfig;




#endif
