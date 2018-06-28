#include "random_freq.h"

uint32_t random_getRandomFreq(uint32_t seed, int index)
{
	int i;
	srand(seed);
	for(i=0;i<index;i++) {
		rand();
	}

	return( 410000000 + 250000 * (rand() % (200 - 1)) );
}

uint32_t random_getRandomTime(uint32_t seed)
{
	int i;
	int min = 4000;
	int max = 14000;
	srand(seed);
	for(i=0;i<seed%20;i++) {
		rand();
	}
	return min + (rand() % (max - min - 1));
}
