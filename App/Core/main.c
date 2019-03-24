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
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "app_ethernet.h"
#include "httpserver-netconn.h"
#include "SDApp.h"
#include "sd_diskio_dma_rtos.h"
#include "usbh_diskio_dma.h"
#include "waveplayer.h"

static void SystemClock_Config(void);
void Bk_Task(void const *args);
void Main_Task(void const *args);
static void MPU_Config(void);
static void ReadOutProtection(void);
static void Netif_Config(void);
static void CPU_CACHE_Enable(void);
void TouchUpdate(void);

static void USBH_UserProcess(USBH_HandleTypeDef * phost, uint8_t id);

#define RTOS_DTM_SECTION_SIZE 34 * 1024
#define BK_TASK_STACK_SIZE 400
#define MAIN_TASK_STACK_SIZE 750
#define SD_TASK_STACK_SIZE 300

TaskHandle_t bkTaskHandle, guiTaskHandle, sdTaskHandle, audioTaskHandle;
TaskStatus_t xTaskStatus, xTaskStatus1, xTaskStatus2;

#if defined ( __CC_ARM   )
uint8_t ucHeap[ RTOS_DTM_SECTION_SIZE ] __attribute__((at(0x20000000)));
uint8_t ucHeap2[ configTOTAL_HEAP_SIZE - RTOS_DTM_SECTION_SIZE ]  __attribute__((at(0x20010000)));
#elif defined ( __GNUC__ ) /*!< GNU Compiler */
uint8_t ucHeap[ RTOS_DTM_SECTION_SIZE ] __attribute__((section(".DTCMSection")));
uint8_t ucHeap2[ configTOTAL_HEAP_SIZE - RTOS_DTM_SECTION_SIZE ] __attribute__((section(".RAMSection")));
#endif

const HeapRegion_t xHeapRegions[] ={
{ ucHeap, RTOS_DTM_SECTION_SIZE },
{ ucHeap2, configTOTAL_HEAP_SIZE - RTOS_DTM_SECTION_SIZE },
{ NULL,               0         }  /* Marks the end of the array. */
};

volatile uint32_t *sdramData;
volatile uint32_t* lcdData;
extern LTDC_HandleTypeDef hltdc;

struct netif gnetif; /* network interface structure */

WM_HWIN frameMain;
MULTIEDIT_HANDLE editLog;
TEXT_Handle textStatus;
TEXT_Handle textAudioStatus;

char tmpstr[50];

#define ID_MAIN_EDITLOG         12861
#define ID_MAIN_TEXTSTATUS      12862
#define ID_MAIN_TEXTAUDIOSTATUS 12863

USBH_HandleTypeDef hUSBHost;
FATFS USBH_fatfs;
char USBDISKPath[4];            /* USB Host logical drive path */

extern uint8_t sdAppliState;
extern uint8_t sdAppliReq;
extern uint8_t sdIsMounted;

extern FIL logFile;
extern FIL testFile;
extern char wstr[];
extern char rstr[50];

extern uint8_t retSD;    /* Return value for SD */
extern char SDPath[4];   /* SD logical drive path */
extern FATFS SDFatFS;    /* File system object for SD logical drive */
// extern uint8_t workBuffer[2 * _MAX_SS];

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

//  ReadOutProtection();

  xTaskCreate((TaskFunction_t) Bk_Task, "Bk Task", BK_TASK_STACK_SIZE, 
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

  sdramData = (uint32_t *) SDRAM_DEVICE_ADDR;
  
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

  xTaskCreate((TaskFunction_t) Main_Task, "Main Task", MAIN_TASK_STACK_SIZE, 
            (void *) NULL, tskIDLE_PRIORITY, 
            &guiTaskHandle);

 /* Init Host Library */
  USBH_Init(&hUSBHost, USBH_UserProcess, 0);

  /* Add Supported Class */
  USBH_RegisterClass(&hUSBHost, USBH_MSC_CLASS);

  /* Start Host Process */
  USBH_Start(&hUSBHost);

  sdAppliState = SD_APP_NO_SD;
  sdIsMounted = 0;
  sdAppliReq = 0;

  FATFS_LinkDriver(&SD_Driver, SDPath);

  xTaskCreate((TaskFunction_t) SD_Task, "SD Task", SD_TASK_STACK_SIZE, 
            (void *) NULL, 1, 
            &sdTaskHandle);

  BSP_SD_IsDetected();
  vTaskDelay(100);

  while (1) 
  {
    BSP_LED_Toggle(LED1);

    AUDIO_PLAYER_Process();

    vTaskDelay(50);
  }
}

void Main_Task(void const *args) 
{
  lcdData = (uint32_t *) LCD_FB_START_ADDRESS;

   // Initialize emWin GUI 
  GUI_Init();  

  // HAL_LTDC_SetAlpha(&hltdc, 200, 0);

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

  textAudioStatus = TEXT_CreateEx(200, 252, 300, 20, frameMain, 
    WM_CF_SHOW, 0, ID_MAIN_TEXTAUDIOSTATUS, "");

  GUI_Exec();
  
  /* Create tcp_ip stack thread */
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

  xTaskCreate((TaskFunction_t) AudioTask, "Audio Task", configMINIMAL_STACK_SIZE * 2, 
            (void *) NULL, 8, 
            &audioTaskHandle);

  AUDIO_PLAYER_Start("0:/1.wav");

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
      // vTaskGetInfo(guiTaskHandle, &xTaskStatus, pdTRUE, eInvalid);
      // vTaskGetInfo(bkTaskHandle, &xTaskStatus1, pdTRUE, eInvalid);
      // vTaskGetInfo(sdTaskHandle, &xTaskStatus2, pdTRUE, eInvalid);

      // sprintf(tmpstr, "FPS : %5.1f - M:%u-B:%u-S:%u", (float) frameCounter / 
      //   (now - fpsTimeCounter) * (1000 * portTICK_RATE_MS), xTaskStatus.usStackHighWaterMark, 
      //   xTaskStatus1.usStackHighWaterMark, xTaskStatus2.usStackHighWaterMark );

      sprintf(tmpstr, "FPS : %.1f   ", (float) frameCounter / 
        (now - fpsTimeCounter) * (1000 * portTICK_RATE_MS));

      TEXT_SetText(textStatus, tmpstr);
      frameCounter = 0;
      fpsTimeCounter = xTaskGetTickCount();
    }

    vTaskDelay(50);
  }
}


/**
  * @brief  User Process
  * @param  phost: Host Handle
  * @param  id: Host Library user message ID
  * @retval None
  */
static void USBH_UserProcess(USBH_HandleTypeDef * phost, uint8_t id)
{
  switch (id)
  {
  case HOST_USER_SELECT_CONFIGURATION:
    break;

  case HOST_USER_DISCONNECTION:

    if (f_mount(NULL, USBDISKPath, 0) != FR_OK)
    {
      __log("ERROR : Cannot DeInitialize FatFs! \n");
    }
    if (FATFS_UnLinkDriver(USBDISKPath) != 0)
    {
      __log("ERROR : Cannot UnLink FatFS Driver! \n");
    }
    __log("USB Disconnected");
    break;

  case HOST_USER_CLASS_ACTIVE:

    break;

  case HOST_USER_CONNECTION:
    if (FATFS_LinkDriver(&USBH_Driver, USBDISKPath) == 0)
    {
      if (f_mount(&USBH_fatfs, USBDISKPath, 0) != FR_OK)
      {
        __log("ERROR : Cannot Initialize FatFs! \n");
      }
    }
    __log("USB Connected");
  break;

  default:
    break;
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


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 200000000
  *            HCLK(Hz)                       = 200000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 400
  *            PLL_P                          = 2
  *            PLLSAI_N                       = 384
  *            PLLSAI_P                       = 8
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
// void SystemClock_Config(void)
// {
//   RCC_ClkInitTypeDef RCC_ClkInitStruct;
//   RCC_OscInitTypeDef RCC_OscInitStruct;
//   RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

//   /* Enable HSE Oscillator and activate PLL with HSE as source */
//   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
//   RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//   RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
//   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//   RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
//   RCC_OscInitStruct.PLL.PLLM = 25;
//   RCC_OscInitStruct.PLL.PLLN = 400;
//   RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
//   RCC_OscInitStruct.PLL.PLLQ = 8;

//   if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//   {
//     while(1) { ; }
//   }

//   /* Activate the OverDrive to reach the 200 Mhz Frequency */
//   if (HAL_PWREx_EnableOverDrive() != HAL_OK)
//   {
//     while(1) { ; }
//   }

//   /* Select PLLSAI output as USB clock source */
//   PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
//   PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
//   PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
//   PeriphClkInitStruct.PLLSAI.PLLSAIQ = 4;
//   PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV4;

//   if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
//   {
//     while(1) { ; }
//   }

//   /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
//    * clocks dividers */
//   RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
//                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
//   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//   RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//   RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
//   RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

//   if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
//   {
//     while(1) { ; }
//   }
// }


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
  /* Invalidate I-Cache : ICIALLU register */
  SCB_InvalidateICache();

  /* Enable branch prediction */
  SCB->CCR |= (1 <<18);
  __DSB();

  /* Invalidate I-Cache : ICIALLU register */
  SCB_InvalidateICache();

  /* Enable I-Cache */
  SCB_EnableICache();

  SCB_InvalidateDCache();
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

  /* Configure the MPU attributes for SDRAM as WT */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = SDRAM_DEVICE_ADDR;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  
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

static void ReadOutProtection(void)
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
