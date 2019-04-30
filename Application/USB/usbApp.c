#include "main.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "usbh_diskio_dma.h"
#include "usbh_core.h"
#include "usbh_hid_mouse.h"
#include "usbApp.h"

USBH_HandleTypeDef hUSBHost;
FATFS USBH_fatfs;
char USBDISKPath[4];            /* USB Host logical drive path */
HID_MOUSE_Info_TypeDef* hid_mouse;
uint8_t usbh_hid_mouse_appli_state = APP_NOT_READY;
uint8_t usbh_msc_appli_state = APP_NOT_READY;

FRESULT scan_files (
    char* path        /* Start node to be scanned (***also used as work area***) */
)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                // i = strlen(path);                    // requires a lot memory
                // sprintf(&path[i], "/%s", fno.fname);
                // res = scan_files(path);                    /* Enter the directory */
                // if (res != FR_OK) break;
                // path[i] = 0;
            } else {                                       /* It is a file. */
                sprintf(tmpstr, "%s/%s", path, fno.fname);
                __log(tmpstr);
            }
        }
        f_closedir(&dir);
    }

    return res;
}

/**
 * @brief  User Process
 * @param  phost: Host Handle
 * @param  id: Host Library user message ID
 * @retval None
 */
void USBH_UserProcess(USBH_HandleTypeDef * phost, uint8_t id)
{
 switch (id)
 {
 case HOST_USER_SELECT_CONFIGURATION:
   break;

 case HOST_USER_DISCONNECTION:
   if (f_mount(NULL, USBDISKPath, 0) != FR_OK)
   {
     __log("ERROR : Cannot DeInitialize FatFs!");
   }
   if (USBDISKPath[0] != 0 && FATFS_UnLinkDriver(USBDISKPath) != 0)
   {
     __log("ERROR : Cannot UnLink FatFS Driver!");
   }

   usbh_hid_mouse_appli_state = APP_NOT_READY;
   usbh_msc_appli_state = APP_NOT_READY;

   __log("USB Disconnected");
   break;

 case HOST_USER_CLASS_ACTIVE:

    __log("USB Ready");

    switch (USBH_GetActiveClass(&hUSBHost))
    {
    case USB_MSC_CLASS:

      __log("MSC");

      if(USBDISKPath[0] == 0)
      {
        FATFS_LinkDriver(&USBH_Driver, USBDISKPath);
        sprintf(tmpstr, "USB Linked : %s", USBDISKPath);
        __log(tmpstr);
      }      

      if (f_mount(&USBH_fatfs, USBDISKPath, 1) != FR_OK)
      {
        __log("ERROR : Cannot Initialize FatFs!");
      }
      else
      {
        __log("USB MS Mounted");
        usbh_msc_appli_state = APP_START;
      }

      break;

    case USB_HID_CLASS:

     switch(USBH_HID_GetDeviceType(&hUSBHost))
     {
      case HID_MOUSE :
        USBH_HID_MouseInit(&hUSBHost);

        hid_mouse = USBH_HID_GetMouseInfo(&hUSBHost);

        usbh_hid_mouse_appli_state = APP_START; 

        __log("Mouse");
        break;
      case HID_KEYBOARD :

        __log("Keyboard");
        break;
      case HID_UNKNOWN :

        __log("Unknow Device");
        break;
      } 

      __log("HID");

      break;
    }

   break;

 case HOST_USER_CONNECTION:

   __log("USB Connected");

    sprintf(tmpstr, "Class : %u", USBH_GetActiveClass(&hUSBHost));

    __log(tmpstr);

 break;

 default:
   break;
 }
}

void USBH_APP()
{     
  switch(usbh_hid_mouse_appli_state)
  {
    case APP_START:

      usbh_hid_mouse_appli_state = APP_READY;
      break;
    case APP_READY:

      sprintf(tmpstr, "Mouse : x = %u , y = %u", hid_mouse->x, hid_mouse->y);
      __log(tmpstr);

      break;
  }
  switch(usbh_msc_appli_state)
  {
    case APP_START:
      scan_files(USBDISKPath);

      usbh_msc_appli_state = APP_READY;
      break;
    case APP_READY:

      break;
  }
}
