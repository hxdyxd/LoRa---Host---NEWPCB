
#ifndef __KEYS_H__
#define __KEYS_H__

#include "stm32f10x.h"
#include <systick.h>

void keys_init(void);
uint8_t key_read(int id);
void keys_proc( void (* key_func)(int8_t key_id) );

#endif

