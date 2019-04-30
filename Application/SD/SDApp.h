#include "main.h"
#include "ff.h"

#ifndef __SD_APP_H
#define __SD_APP_H

extern uint8_t sdAppliState;
extern uint8_t sdAppliReq;
extern uint8_t sdIsMounted;

extern FIL logFile;
extern FIL testFile;
extern char wstr[];
extern char rstr[];

extern uint8_t retSD;    /* Return value for SD */
extern char SDPath[4];   /* SD logical drive path */
extern FATFS SDFatFS;    /* File system object for SD logical drive */
// extern uint8_t workBuffer[2 * _MAX_SS];

void SD_Task(void const *args);
void SD_Application(void);

#endif /*  __SD_APP_H  */
