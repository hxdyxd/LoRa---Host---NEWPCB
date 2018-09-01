#include "keys.h"

#define  KEY_TIMEOUT   (50)
#define  KEY_NUM        (3)

struct sGpio
{
	uint32_t RCC_APB2Periph;
	uint16_t GPIO_Pin;
	GPIO_TypeDef* GPIOx;
};

struct sGpio KEY_GPIO[KEY_NUM] = {
	{
		.RCC_APB2Periph = RCC_APB2Periph_GPIOB,
		.GPIOx = GPIOB,
		.GPIO_Pin = GPIO_Pin_0,
	},
	{
		.RCC_APB2Periph = RCC_APB2Periph_GPIOB,
		.GPIOx = GPIOB,
		.GPIO_Pin = GPIO_Pin_9,
	},
	{
		.RCC_APB2Periph = RCC_APB2Periph_GPIOC,
		.GPIOx = GPIOC,
		.GPIO_Pin = GPIO_Pin_13,
	},	
};


/**
  * @brief  ≈‰÷√÷∏∂®“˝Ω≈
  * @retval None
  */
static void KEY_GPIO_Configuration(void)
{
	int i;
    GPIO_InitTypeDef GPIO_InitStruct;
	
	for(i = 0; i < KEY_NUM; i++) {
		RCC_APB2PeriphClockCmd(KEY_GPIO[i].RCC_APB2Periph, ENABLE);
		//DIO
		GPIO_InitStruct.GPIO_Pin =  KEY_GPIO[i].GPIO_Pin;
		//GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_Init(KEY_GPIO[i].GPIOx, &GPIO_InitStruct);
	}
}

void keys_init(void)
{
	KEY_GPIO_Configuration();
}

uint8_t key_read(int id)
{
	return GPIO_ReadInputDataBit(KEY_GPIO[id].GPIOx, KEY_GPIO[id].GPIO_Pin);
}

void keys_proc( void (* key_func)(int8_t key_id) )
{
	static int8_t last_key_val = -1;
	static int8_t key_val = -1;
	static uint64_t timer = 0;
	uint8_t i, j;
	uint8_t key_find = 0;
	
	for(i=0; i<KEY_NUM; i++) {
		if(key_read(i) == Bit_RESET) {
			last_key_val = key_val;
			key_val = i;
			key_find = 1;
			if(last_key_val != key_val) {
				timer = TickCounter;
			}
			break;
		}
	}
	
	if(!key_find) {
		key_val = -1;
	}
	
	static int8_t last_key_val_to_func = -1;
	
	if( TickCounter - timer > KEY_TIMEOUT ) {
		
		if(key_val != -1 && last_key_val_to_func != key_val) {
			key_func(key_val);
		}
		
		last_key_val_to_func = key_val;
	}
}



