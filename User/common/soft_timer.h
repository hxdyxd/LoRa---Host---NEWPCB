#ifndef _SOFT_TIMER_H
#define _SOFT_TIMER_H

#define MAX_TIMER 10

#include <stdint.h>
#include <systick.h>


#define SOFT_TIMER_GET_TICK_COUNT()  (TickCounter)

typedef struct {
	char is_circle;
	char on;
	uint32_t timeout;
	void (* timer_cb)(void);
	uint64_t count;
} SOFT_TIMER;


void soft_timer_ms_tick(void);
void soft_timer_init(void);
char soft_timer_create(char id, char on, char is_circle, void (* timer_cb)(void), uint32_t timeout);
char soft_timer_delete(char id);
void soft_timer_proc(void);



#endif
