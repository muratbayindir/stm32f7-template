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
#include "usbh_core.h"
#include "usbh_msc.h"

enum AppliState
{
  APP_ERR=0,
  APP_START,
  APP_READY,
  APP_NO_SD,
  APP_COMPLETED,
  APP_MOUNT,
  APP_NOT_READY
};


extern TaskHandle_t audioTaskHandle;
extern TEXT_Handle textAudioStatus;

extern char tmpstr[];
extern MULTIEDIT_HANDLE editLog;

#define __log(x) \
MULTIEDIT_AddText(editLog, x); \
MULTIEDIT_AddText(editLog, "\n");

#define __statAudio(x) \
TEXT_SetText(textAudioStatus, x);

#define USE_DHCP

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

// mac 02:00:00:00:01:00
// ip 144.122.107.132
// mer mac 10:7b:44:20:fb:ad


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
