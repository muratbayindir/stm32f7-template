/* Includes ------------------------------------------------------------------*/
#include "waveplayer.h"
#include "main.h"

static FIL wavFile;

void AUDIO_PLAYER_Start(const char * fileName)
{
  f_open(wavFile);
}

void AUDIO_PLAYER_Process(void);
void AUDIO_PLAYER_Stop(void);

/*****************************  END OF FILE  *****************************/
