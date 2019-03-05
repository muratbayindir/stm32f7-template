/**
  ******************************************************************************
  * @file    main.h 
  * @author  
  * @brief   Header for main.c file
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif
   
/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "stm32f7xx_it.h"
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_audio.h"
#include "stm32746g_discovery_camera.h"
#include "stm32746g_discovery_eeprom.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_qspi.h"
#include "stm32746g_discovery_sd.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_ts.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "GUI.h"
#include "DIALOG.h"


enum SDAppliState
{
  SD_APP_START=0,
  SD_APP_NO_SD,
  SD_APP_CPL,
  SD_APP_MOUNT,
  SD_APP_READY,
  SD_APP_ERR,
  SD_APP_FINISH_ME
};

#define __log(x) \
MULTIEDIT_AddText(editLog, x); \
MULTIEDIT_AddText(editLog, "\n");

// #define USE_DHCP

/*Static IP ADDRESS*/
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   1
#define IP_ADDR3   10
   
/*NETMASK*/
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

/*Gateway Address*/
#define GW_ADDR0   192
#define GW_ADDR1   168
#define GW_ADDR2   1
#define GW_ADDR3   12


/* Exported types ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */ 

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
