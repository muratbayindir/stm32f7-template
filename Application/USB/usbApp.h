#include "main.h"
#include "ff.h"

#ifndef __USB_APP
#define __USB_APP


extern USBH_HandleTypeDef hUSBHost;
extern FATFS USBH_fatfs;
extern char USBDISKPath[];
extern char path[];
extern uint8_t usbh_msc_appli_state;

#define __setUSBMSCFilePath(x)\
sprintf(path, "%s%s", USBDISKPath, x);

FRESULT scan_files ( char* path );
void USBH_UserProcess(USBH_HandleTypeDef * phost, uint8_t id);
void USBH_APP();

#endif
