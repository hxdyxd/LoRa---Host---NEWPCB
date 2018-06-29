#ifndef __RANDOM_FREQ_H__
#define __RANDOM_FREQ_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void random_adc_config(void);
uint8_t random_get_adc_value(void);
uint16_t random_get_test(void);
uint32_t random_get_value(void);


uint32_t random_getRandomFreq(uint32_t seed, int index);
uint32_t random_getRandomTime(uint32_t seed);

#endif
