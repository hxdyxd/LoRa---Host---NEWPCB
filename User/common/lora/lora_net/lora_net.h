/* LORANET GATEWAY 2018 06 27 END */
/* By hxdyxd */

#ifndef _LORA_NET_H
#define _LORA_NET_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "sx1276-LoRa.h"
#include "sx1276-LoRaMisc.h"
#include "type.h"
#include "app_debug.h"

#pragma pack(1)

typedef struct {
	uint8_t Flag_version:4;
	uint8_t Flag_type:3;
	uint8_t Flag_direction:1;
	uint8_t Data[RF_BUFFER_SIZE];
	uint16_t size;
}LORA_ROUTE_PACK;

#pragma pack()


typedef struct sLORA_NET {
	void (* lora_net_rx_callback)(struct sLORA_NET *netp);
	tLora loraConfigure;
	LORA_ROUTE_PACK pack;
}LORA_NET;


#define FLAG_VER          (0)

#define FLAG_TYPE_BASE_STATION_BINDING       (0)
#define FLAG_TYPE_NETWORK_REQUEST            (1)
#define FLAG_TYPE_USER_DATA                  (2)
#define FLAG_TYPE_FAST_DATA                  (3)

#define FLAG_DIR_UP       (1)
#define FLAG_DIR_DOWN     (0)


int lora_net_init(LORA_NET *netp);

void lora_net_proc(LORA_NET *netp, int lora_max_num);

int lora_net_read_no_block(LORA_NET *netp, uint8_t *buffer);

int lora_net_write(LORA_NET *netp, uint8_t *buffer, uint8_t len);
int lora_net_read(LORA_NET *netp, uint8_t *buffer);

void lora_net_debug_hex(uint8_t *p, uint8_t len, uint8_t lf);

void lora_net_Set_Config(LORA_NET *netp, tTableMsg *msg);

//gateway
void lora_net_Gateway_User_data(LORA_NET *netp, uint8_t *buffer, uint8_t len);  //fix in 2018 07 13
int lora_net_Gateway_User_data_r(LORA_NET *netp, uint8_t *buffer);  //fix in 2018 07 13
int lora_net_Gateway_Network_request(LORA_NET *netp, tTableMsg *msg, uint8_t *gmac);

//node
void lora_net_User_data(LORA_NET *netp, uint8_t *send_buffer, uint8_t len);  //fix in 2018 07 13
void lora_net_Network_request(LORA_NET *netp, uint8_t *nmac, uint8_t *gmac);   //fix in 2018 07 13
void lora_net_Base_station_binding(LORA_NET *netp, uint8_t *nmac);   //fix in 2018 07 13

int lora_net_User_data_r(LORA_NET *netp, uint8_t *read_buffer);  //add in 2018 07 13
int lora_net_Network_request_r(LORA_NET *netp, uint8_t *nmac, tTableMsg *privateMsg);  //add in 2018 07 13
int lora_net_Base_station_binding_r(LORA_NET *netp, uint8_t *nmac, uint8_t *gmac);

#endif
