#include "random_freq.h"
#include "stm32f10x.h"

extern uint16_t timeout_1x1;
extern uint16_t timeout_1x2;

void random_adc_config(void)
{
	ADC_InitTypeDef            ADC_InitStructure;
	GPIO_InitTypeDef           GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
	
	//............................................................
	//Config.........
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	

	ADC_InitStructure.ADC_Mode                = ADC_Mode_Independent;
	// 禁止扫描模式，多通道才要
	ADC_InitStructure.ADC_ScanConvMode        = ENABLE;
	// 连续转换模式,定时器触发不能开启
	ADC_InitStructure.ADC_ContinuousConvMode  = ENABLE;
	// TIM3触发转换
	ADC_InitStructure.ADC_ExternalTrigConv    = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign           = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel        = 1;
	ADC_Init(ADC1, &ADC_InitStructure);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	//...........................................
	//Config...
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_28Cycles5);

	ADC_DMACmd(ADC1, ENABLE);
	
	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

uint8_t random_get_adc_value(void)
{
	uint8_t adc_v;
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET);
	//获取ADC1的值
	adc_v = ADC_GetConversionValue(ADC1);
	//清除转换完成标志
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	return adc_v&1;
}

uint16_t random_get_test(void)
{
	int i;
	int sum = 0;
	for(i=0;i<100;i++){
		if( random_get_adc_value() ) {
			sum++;
		}
	}
	return sum;
}

uint32_t random_get_value(void)
{
	int i;
	uint32_t rand_value = 0;
	for(i=0;i<32;i++){
		rand_value <<= 1;
		rand_value |= random_get_adc_value();
	}
	return rand_value;
}


/*********************v1*************************/
static unsigned long next = 1;

/* RAND_MAX assumed to be 32767 */
int rand_v1(void)
{
    next = next * 1103515245 + 12345;
    return ((unsigned)(next/65536) % 32768);
}

void srand_v1(unsigned seed)
{
    next = seed;
}

#define  SRAND  srand_v1
#define  RAND   rand_v1
/**********************************************/


uint32_t random_getRandomFreq(uint32_t seed, int index)
{
    int i;
    SRAND(seed);
    for(i=0;i<index;i++) {
        RAND();
    }

    uint32_t freq = ( 410000000 + 250000 * (RAND() % (400 - 1)) );
    //printf("%08x[%d] %d\r\n", seed, index, freq);
    return freq;
}

uint32_t random_getRandomTime(uint32_t seed)
{
    int i;
    int min = timeout_1x1;
    int max = timeout_1x2;
    SRAND(seed);
    for(i=0;i<seed%20;i++) {
        RAND();
    }
    return min + (RAND() % (max - min - 1));
}

