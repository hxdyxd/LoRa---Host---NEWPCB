#include "flash.h"

#define FLASH_PAGE_SIZE         ((uint32_t)FLASH_Page_Size)   /* FLASH Page Size */
#define FLASH_USER_START_ADDR   ((uint32_t)FLASH_Start_Addr)   /* Start @ of user Flash area */

/*******************************************************************************
* 名称: xfs_sava_cfg
* 功能: 保存配置信息
* 形参: 
* 返回: 
* 说明: 
*******************************************************************************/
uint32_t xfs_sava_cfg(const unsigned char *cfg, uint32_t len)
{
	uint32_t Address = FLASH_USER_START_ADDR;
	uint32_t end =  FLASH_USER_START_ADDR + len;
	uint32_t *p = (uint32_t*)cfg;
	
	FLASH_Unlock();
  /* Erase the user Flash area
    (area defined by FLASH_USER_START_ADDR) ***********/

  /* Clear pending flags (if any) */  
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); 

  /* Erase the FLASH pages */

    if (FLASH_ErasePage(FLASH_USER_START_ADDR)!= FLASH_COMPLETE)
    {
     /* Error occurred while sector erase. 
         User can add here some code to deal with this error  */
		FLASH_Lock(); 
		return 0;   
    }

  /* Program the user Flash area word by word
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

  while (Address < end)
  {
    if (FLASH_ProgramWord(Address, *p) == FLASH_COMPLETE)
    {
      Address = Address + 4;
	  p++;
    }
    else
    { 
      /* Error occurred while writing data in Flash memory. 
         User can add here some code to deal with this error */
      FLASH_Lock(); 
	   return 0;  
    }
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  FLASH_Lock(); 
  return 1;
}

