/**
  ******************************************************************************
  * @file    main.c
  * @author  
  * @brief   main.c file
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h" 
#include <math.h>
#include "cmsis_os.h"
#include "main_frame.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio_dma_rtos.h"
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "app_ethernet.h"
#include "httpserver-netconn.h"

static void SystemClock_Config(void);
void Bk_Task(void const *args);
void GUI_Task(void const *args);
void SD_Task(void const *args);
static void MPU_Config(void);
static void ReadOutProtection();
static void Netif_Config(void);
static void CPU_CACHE_Enable(void);
void TouchUpdate(void);

#define RTOS_DTM_SECTION_SIZE 40 * 1024
#define BK_TASK_STACK_SIZE 1024 * 2
#define GUI_TASK_STACK_SIZE 1024 * 2
#define SD_TASK_STACK_SIZE 1024

TaskHandle_t bkTaskHandle, guiTaskHandle, sdTaskHandle;
TaskStatus_t xTaskStatus, xTaskStatus1, xTaskStatus2;

uint8_t ucHeap[ RTOS_DTM_SECTION_SIZE ] __attribute__((section(".DTCMSection")));
uint8_t ucHeap2[ configTOTAL_HEAP_SIZE - RTOS_DTM_SECTION_SIZE ];

const HeapRegion_t xHeapRegions[] ={
{ ucHeap, RTOS_DTM_SECTION_SIZE },
{ ucHeap2, configTOTAL_HEAP_SIZE - RTOS_DTM_SECTION_SIZE },
{ NULL,               0         }  /* Marks the end of the array. */
};

uint8_t *sdramData;
extern LTDC_HandleTypeDef hltdc;  

__IO uint32_t uwVolume = 70;

uint8_t retSD;    /* Return value for SD */
char SDPath[4] = "0:/";   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
// uint8_t workBuffer[2 * _MAX_SS];

FIL logFile;
FIL testFile;
char wstr[] = "Hello World";
char rstr[50];
char tmpstr[50];

uint16_t audioBuffer[1024];

struct netif gnetif; /* network interface structure */

WM_HWIN frameMain;
MULTIEDIT_HANDLE editLog;
TEXT_Handle textStatus;

#define ID_MAIN_EDITLOG     12861
#define ID_MAIN_TEXTSTATUS  12862

uint8_t sdAppliState;
uint8_t sdAppliReq;
uint8_t sdIsMounted;

/**
* @brief  Main program
* @param  None
* @retval int
*/
int main(void)
{   
  /* Configure the MPU attributes as Write Through */
  MPU_Config();

  CPU_CACHE_Enable();

  /* Invalidate I-Cache : ICIALLU register */
  // SCB_InvalidateICache();

  // /* Enable branch prediction */
  // SCB->CCR |= (1 <<18);
  // __DSB();

  // /* Invalidate I-Cache : ICIALLU register */
  // SCB_InvalidateICache();

  // /* Enable I-Cache */
  // SCB_EnableICache();

  // SCB_InvalidateDCache();
  // SCB_EnableDCache();

  /* STM32F7xx HAL library initialization:
  - Configure the Flash ART accelerator on ITCM interface
  - Configure the Systick to generate an interrupt each 1 msec
  - Set NVIC Group Priority to 4
  - Global MSP (MCU Support Package) initialization
  */
  HAL_Init();

  __HAL_RCC_CRC_CLK_ENABLE();
  
  /* Configure the system clock @ 200 Mhz */
  SystemClock_Config();

  vPortDefineHeapRegions(xHeapRegions);

  // ReadOutProtection();

  xTaskCreate(Bk_Task, "Bk Task", BK_TASK_STACK_SIZE, 
            (void *) NULL, tskIDLE_PRIORITY, 
            &bkTaskHandle);

  /* Start scheduler */
  vTaskStartScheduler();
	
  while(1);
}

void Bk_Task(void const *args) 
{
  /* Initialize the SDRAM */
  BSP_SDRAM_Init();

  sdramData = SDRAM_DEVICE_ADDR;
  
  /* Initialize the Touch screen */
  BSP_TS_Init(420, 272);

  /* Enable CRC to Unlock GUI */
  __HAL_RCC_CRC_CLK_ENABLE();
  
  /* Enable Back up SRAM */
  __HAL_RCC_BKPSRAM_CLK_ENABLE();

  BSP_LED_Init(LED1);

  // BSP_LCD_Init();
  // BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  // BSP_LCD_SelectLayer(0);
  // BSP_LCD_DisplayOn();
  // BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
  // BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  // BSP_LCD_Clear(LCD_COLOR_BLACK);
  // BSP_LCD_DisplayStringAtLine(2, " Hello World !!!");

  xTaskCreate(GUI_Task, "GUI Task", GUI_TASK_STACK_SIZE, 
            (void *) NULL, tskIDLE_PRIORITY, 
            &guiTaskHandle);

  BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_AUTO, uwVolume, I2S_AUDIOFREQ_48K);

  memset(audioBuffer, 0, 1024);

  for (int i = 1; i < 1024; ++i)
  {
    audioBuffer[i] = sin(i * 3.14159f / 2048.0f) * 5000;
  }

  BSP_AUDIO_OUT_Play(audioBuffer, 1024 * sizeof(uint16_t));

  sdAppliState = SD_APP_NO_SD;
  sdIsMounted = 0;
  sdAppliReq = 0;

  FATFS_LinkDriver(&SD_Driver, SDPath);

  BSP_SD_IsDetected();
  vTaskDelay(100);

  xTaskCreate(SD_Task, "SD Task", SD_TASK_STACK_SIZE, 
            (void *) NULL, 1, 
            &sdTaskHandle);

  while (1) 
  {
    BSP_LED_Toggle(LED1);

    vTaskDelay(30);
  }
}

void GUI_Task(void const *args) 
{
  char tmpstr[50];
  uint32_t i;
  uint32_t* lcdData;
  lcdData = LCD_FB_START_ADDRESS;

   // Initialize emWin GUI 
  GUI_Init();  

  HAL_LTDC_SetAlpha(&hltdc, 200, 0);

  // GUI_SetFont(&GUI_Font32_1);

  // GUI_SetColor(GUI_WHITE);
  // GUI_SetBkColor(GUI_BLACK);
   // Clear Display 
  // GUI_Clear();

  frameMain = CreateFramewin();

  editLog = MULTIEDIT_CreateEx(100, 120, 200, 100, frameMain,
   WM_CF_SHOW, MULTIEDIT_CF_AUTOSCROLLBAR_V, ID_MAIN_EDITLOG, 50, "");

  textStatus = TEXT_CreateEx(20, 252, 300, 20, frameMain, 
    WM_CF_SHOW, 0, ID_MAIN_TEXTSTATUS, "");

  GUI_Exec();
  
  /* Create tcp_ip stack thread */ // 169.254.85.210
  tcpip_init(NULL, NULL);

  /* Initialize the LwIP stack */
  Netif_Config();
  
  /* Initialize webserver demo */
  http_server_netconn_init();
  
  /* Notify user about the network interface config */
  User_notification(&gnetif);
  
#ifdef USE_DHCP
  /* Start DHCPClient */
  osThreadDef(DHCP, DHCP_thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate (osThread(DHCP), &gnetif);
#endif

  TickType_t fpsTimeCounter = xTaskGetTickCount();
  TickType_t now;
  uint16_t frameCounter = 0;

  while (1) 
  {
    TouchUpdate();
    GUI_Exec();

    // for (i = 0; i < LCD_GetXSize() * LCD_GetYSize() / 2; ++i)
    //   lcdData[i] &= 0xFF80FF80;

    frameCounter++;
    now = xTaskGetTickCount();
    if(now - fpsTimeCounter > 1000 * portTICK_RATE_MS)
    {
      vTaskGetInfo(guiTaskHandle, &xTaskStatus, pdTRUE, eInvalid);
      vTaskGetInfo(bkTaskHandle, &xTaskStatus1, pdTRUE, eInvalid);
      vTaskGetInfo(sdTaskHandle, &xTaskStatus2, pdTRUE, eInvalid);

      sprintf(tmpstr, "FPS : %5.1f - G:%u-B:%u-S:%u", (float) frameCounter / 
        (now - fpsTimeCounter) * (1000 * portTICK_RATE_MS), xTaskStatus.usStackHighWaterMark, 
        xTaskStatus1.usStackHighWaterMark, xTaskStatus2.usStackHighWaterMark);
      TEXT_SetText(textStatus, tmpstr);
      frameCounter = 0;
      fpsTimeCounter = xTaskGetTickCount();
    }

    vTaskDelay(50);
  }
}


/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
static void Netif_Config(void)
{ 
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
 
#ifdef USE_DHCP
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
#else
  IP_ADDR4(&ipaddr,IP_ADDR0,IP_ADDR1,IP_ADDR2,IP_ADDR3);
  IP_ADDR4(&netmask,NETMASK_ADDR0,NETMASK_ADDR1,NETMASK_ADDR2,NETMASK_ADDR3);
  IP_ADDR4(&gw,GW_ADDR0,GW_ADDR1,GW_ADDR2,GW_ADDR3);
#endif /* USE_DHCP */
  
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
  
  /*  Registers the default network interface. */
  netif_set_default(&gnetif);
  
  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called.*/
    netif_set_up(&gnetif);
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }
}

void SD_Task(void const *args)
{  
  __IO uint8_t lastState, state, counter = 0;
  uint8_t firstTime = 0;

  while(1)
  {
    SD_Application();

    state = BSP_SD_IsDetected();

    if (lastState != state)
    {
      counter = 0;
    }

    if (sdAppliState == SD_APP_NO_SD && counter > 4 && state == 1)
    {
      sdAppliState = SD_APP_MOUNT;
      counter = 250;
      firstTime = 1;
      __log("SD Card Inserted");
    }

    if (sdAppliState == SD_APP_MOUNT && sdAppliState == SD_APP_READY && 
      sdAppliState == SD_APP_START && counter > 4 && state == 0)
    {
      sdAppliState = SD_APP_ERR;
    }

    if(state == 0)
    {
      if (lastState == state && counter != 250)
        counter++;

      if (counter > 4 && counter != 250)
      {
        sdAppliState = SD_APP_NO_SD;
        counter = 250;
        if (firstTime == 0)
        {
          __log("No SD Found");
          firstTime = 1;
        }
        else
        {
          __log("SD Card Ejected");
        }
      }
    }
    else
    {
      if (lastState == state && counter != 250)
        counter++;
      if (counter > 4 && counter != 250)
      {
        sdAppliState = SD_APP_MOUNT;
        counter = 250;
        firstTime = 1;
        __log("SD Card Inserted");
      }
    }

    if (sdAppliReq != 0)
    {
      if (state == 1)
      {
        sdAppliState = SD_APP_START;
        sdAppliReq = 0;
      }
    }

    lastState = state;

    vTaskDelay(50);
  }
}

void SD_Application()
{
  int i;

  switch(sdAppliState)
  {
    case SD_APP_MOUNT:

      if (sdIsMounted == 0)
      {
        retSD = f_mount(&SDFatFS, "0:/", 1);
        sdAppliState = SD_APP_START;
        sdIsMounted = 1;
        __log("Mounted");
      }
      else
      {
        f_mount(0, "0:/", 1);
        sdIsMounted = 0;
      }

      break;

    case SD_APP_START:

      if (BSP_SD_IsDetected())
      {
        if (sdIsMounted == 1)
        {          
          // __log("SD Formatting");

          // retSD = f_mkfs("", FM_ANY, 0, workBuffer, sizeof(workBuffer));

          // if(retSD == FR_OK)
          // {
          //   __log("SD Formatted Succesfully");
          // }
          // else
          // {
          //   sdAppliState = SD_APP_ERR;
          //   sprintf(rstr, "SD Format Error : %u", retSD);
          //   __log(rstr);
          //   return;
          // }

          f_unlink("0:/log.txt");
          retSD = f_open(&logFile, "0:/log.txt", FA_CREATE_NEW | FA_WRITE | FA_CREATE_ALWAYS);

          if(retSD == FR_OK)
          {
            f_write(&logFile, wstr, sizeof(wstr), &i);
            f_close(&logFile);   
          }       
          else
          {
            sprintf(rstr, "log.txt write error : %u", retSD);
            __log(rstr); 
            f_close(&logFile); 
            sdAppliState = SD_APP_ERR;
            return;
          }

          retSD = f_open(&testFile, "0:/1.TXT", FA_OPEN_ALWAYS | FA_READ);

          if(retSD == FR_OK)
          {
            memset(rstr, ' ', 50);
            f_read(&testFile, rstr, 50, &i);  
          }
          else
          {
            sprintf(rstr, "1.TXT read error : %u", retSD);
          }
          __log(rstr); 
          f_close(&testFile);    

          retSD = f_open(&logFile, "0:/log.txt", FA_OPEN_ALWAYS | FA_READ);

          if(retSD == FR_OK)
          {
            memset(rstr, ' ', 50);
            f_read(&logFile, rstr, 50, &i);     
          }
          else
          {
            sprintf(rstr, "log.txt read error : %u", retSD);
          }
          __log(rstr);   
          f_close(&logFile);  

          sdAppliState = SD_APP_READY;
        }
        else
        {
          sdIsMounted = 0;
          sdAppliState = SD_APP_MOUNT;
          __log("Not Mounted");
        }
      }
      else
      {
        sdIsMounted = 0;
        sdAppliState = SD_APP_NO_SD;
        __log("No SD Found");
      }

      break;
    case SD_APP_NO_SD:

      break;
    case SD_APP_ERR:

      __log("SD Card Error");
      f_mount(0, "0:/", 1);
      FATFS_UnLinkDriver(SDPath);
      FATFS_LinkDriver(&SD_Driver, SDPath);
      sdAppliState = SD_APP_NO_SD;
      sdIsMounted = 0;

      break;
    case SD_APP_READY:

      // if (BSP_SD_IsDetected())
      // {
      //   f_mount(0, "", 1);
      // }

      break;
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 432;  
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
  
  /* Activate the OverDrive to reach the 216 MHz Frequency */  
  ret = HAL_PWREx_EnableOverDrive();
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; 
  
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }  
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}


/**
  * @brief  Configure the MPU attributes .
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();
  
  /* Configure the MPU as Normal Non Cacheable for Ethernet Buffers in the SRAM2 */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x2004C000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  
  /* Configure the MPU as Device for Ethernet Descriptors in the SRAM2 */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x2004C000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512B;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

/*
@Note It is recommended to enable the cache and maintain its coherence, but depending on the use case
      It is also possible to configure the MPU as "Write through", to guarantee the write access coherence.
      In that case, the MPU must be configured as Cacheable/Bufferable/Not Shareable.
      Even though the user must manage the cache coherence for read accesses.
      Please refer to the AN4838 “Managing memory protection unit (MPU) in STM32 MCUs”
      Please refer to the AN4839 “Level 1 cache on STM32F7 Series”
*/
  //Cacheable/Bufferable/Not Shareable.

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x20010000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);


  
  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static void ReadOutProtection()
{
  FLASH_OBProgramInitTypeDef OB_Init;

  HAL_FLASHEx_OBGetConfig(&OB_Init);

  if(OB_Init.RDPLevel != OB_RDP_LEVEL_1)
  {
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock(); 
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR);
    OB_Init.OptionType = OPTIONBYTE_RDP;
    OB_Init.RDPLevel = OB_RDP_LEVEL_1;
    HAL_FLASHEx_OBProgram(&OB_Init);
    HAL_FLASH_OB_Launch();
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
  }
}


/**
  * @brief  Read the coordinate of the point touched and assign their
  *         value to the variables u32_TSXCoordinate and u32_TSYCoordinate
  * @param  None
  * @retval None
  */
void TouchUpdate(void)
{
  static GUI_PID_STATE TS_State = {0, 0, 0, 0};
  __IO TS_StateTypeDef  ts;
  uint16_t xDiff, yDiff;  

  BSP_TS_GetState((TS_StateTypeDef *)&ts);

  if((ts.touchX[0] >= LCD_GetXSize()) ||(ts.touchY[0] >= LCD_GetYSize()) ) 
  {
    ts.touchX[0] = 0;
    ts.touchY[0] = 0;
    ts.touchDetected =0;
  }

  xDiff = (TS_State.x > ts.touchX[0]) ? (TS_State.x - ts.touchX[0]) : (ts.touchX[0] - TS_State.x);
  yDiff = (TS_State.y > ts.touchY[0]) ? (TS_State.y - ts.touchY[0]) : (ts.touchY[0] - TS_State.y);
  
  
  if((TS_State.Pressed != ts.touchDetected ) ||
     (xDiff > 30 )||
      (yDiff > 30))
  {
    TS_State.Pressed = ts.touchDetected;
    if(ts.touchDetected) 
    {
      TS_State.x = ts.touchX[0];
      TS_State.y = ts.touchY[0];
      GUI_TOUCH_StoreStateEx(&TS_State);
    }
    else
    {
      GUI_TOUCH_StoreStateEx(&TS_State);
      TS_State.x = 0;
      TS_State.y = 0;
    }
  }
}

/*********************************  END OF FILE  ******************************************/
