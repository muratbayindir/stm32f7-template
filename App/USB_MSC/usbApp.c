// #include "main.h"
// #include "ff.h"
// #include "usbh_diskio_dma.h"

// USBH_HandleTypeDef hUSBHost;
// FATFS USBH_fatfs;
// char USBDISKPath[4];            /* USB Host logical drive path */

// extern MULTIEDIT_HANDLE editLog;

// /**
//   * @brief  User Process
//   * @param  phost: Host Handle
//   * @param  id: Host Library user message ID
//   * @retval None
//   */
// void USBH_UserProcess(USBH_HandleTypeDef * phost, uint8_t id)
// {
//   switch (id)
//   {
//   case HOST_USER_SELECT_CONFIGURATION:
//     break;

//   case HOST_USER_DISCONNECTION:
//     if (f_mount(NULL, "", 0) != FR_OK)
//     {
//       __log("ERROR : Cannot DeInitialize FatFs!");
//     }
//     if (FATFS_UnLinkDriver(USBDISKPath) != 0)
//     {
//       __log("ERROR : Cannot UnLink FatFS Driver!");
//     }

//     __log("USB Disconnected");

//     break;

//   case HOST_USER_CLASS_ACTIVE:

//     break;

//   case HOST_USER_CONNECTION:
//     if (FATFS_LinkDriver(&USBH_Driver, USBDISKPath) == 0)
//     {
//       if (f_mount(&USBH_fatfs, "", 0) != FR_OK)
//       {
//         __log("ERROR : Cannot Initialize FatFs!");
//       }
//     }

//     __log("USB Connected");

//   break;

//   default:
//     break;
//   }
// }