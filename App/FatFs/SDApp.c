#include "main.h" 
#include <math.h>
#include "cmsis_os.h"
#include "main_frame.h"
#include "sd_diskio_dma_rtos.h"
#include "SDApp.h"
#include "ff.h"

extern MULTIEDIT_HANDLE editLog;

uint8_t sdAppliState;
uint8_t sdAppliReq;
uint8_t sdIsMounted;

FIL logFile;
FIL testFile;
char wstr[] = "Hello World";
char rstr[50];

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
// uint8_t workBuffer[2 * _MAX_SS];

char path[100];

#define __setSDFilePath(x)\
sprintf(path, "%s%s", SDPath, x);

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
  uint32_t i;

  switch(sdAppliState)
  {
    case SD_APP_MOUNT:

      if (sdIsMounted == 0)
      {
        retSD = f_mount(&SDFatFS, SDPath, 1);
        if (retSD != FR_OK)
        {
          sdIsMounted = 0;
          // sdAppliState = SD_APP_ERR;

          f_mount(0, SDPath, 1);
          FATFS_UnLinkDriver(SDPath);
          FATFS_LinkDriver(&SD_Driver, SDPath);
          
          break;
        }
        sdAppliState = SD_APP_START;
        sdIsMounted = 1;
        __log("Mounted");
      }
      else
      {
        f_mount(0, SDPath, 1);
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

          __setSDFilePath("log.txt");
          f_unlink(path);
          retSD = f_open(&logFile, path, FA_CREATE_NEW | FA_WRITE | FA_CREATE_ALWAYS);

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

          __setSDFilePath("1.TXT");
          retSD = f_open(&testFile, path, FA_OPEN_ALWAYS | FA_READ);

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

          __setSDFilePath("log.txt");
          retSD = f_open(&logFile, path, FA_OPEN_ALWAYS | FA_READ);

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
      f_mount(0, SDPath, 1);
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