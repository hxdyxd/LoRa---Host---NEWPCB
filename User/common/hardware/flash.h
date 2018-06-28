#ifndef __FLASH_H__
#define __FLASH_H__

#include "stm32f10x.h"
#include "stdio.h"

#define FLASH_Page_Size    ((uint32_t) 0x00000400) //FLASH页的大小
//#define Index_Start_Addr   ((uint32_t) 0x08005000) //数据存储标志起始地址
//#define Index_End_Addr     ((uint32_t) 0x08005400) //数据存储标志结束地址
#define FLASH_Start_Addr   ((uint32_t) 0x0800FC00) //数据存储起始地址  1K
#define FLASH_End_Addr     ((uint32_t) 0x08010000) //数据存储结束地址
#define DATA_32            ((uint32_t) 0x12345678) //要编写的数据

uint32_t xfs_sava_cfg(const unsigned char *cfg, uint32_t len);

#endif

