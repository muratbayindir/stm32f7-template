#ifndef __MAIN_FRAME
#define __MAIN_FRAME

#include "DIALOG.h"

extern WM_HWIN frameMain;
extern MULTIEDIT_HANDLE editLog;
extern TEXT_Handle textStatus;
extern TEXT_Handle textAudioStatus;

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define ID_MAIN_FRAMEWIN_0 (GUI_ID_USER + 0x00)
#define ID_MAIN_BUTTON_0 (GUI_ID_USER + 0x01)
#define ID_MAIN_BUTTON_1 (GUI_ID_USER + 0x02)


#define ID_MAIN_EDITLOG         12861
#define ID_MAIN_TEXTSTATUS      12862
#define ID_MAIN_TEXTAUDIOSTATUS 12863


void CreateFramewin(void);

#endif
