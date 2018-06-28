#ifndef __SYSSTICK_H__
#define __SYSSTICK_H__

#include <stm32f10x.h>

extern uint64_t TickCounter;
void systick_init(void);

#endif
