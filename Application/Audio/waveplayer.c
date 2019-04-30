/* Includes ------------------------------------------------------------------*/
#include "waveplayer.h"
#include "main.h"
#include "ff.h"
#include "GUI.h"

static AUDIO_OUT_BufferTypeDef  BufferCtl;
static __IO uint32_t uwVolume = 70;
static char playingFileName[50];

WAVE_FormatTypeDef WaveFormat;
FIL WavFile;
AUDIO_PLAYBACK_StateTypeDef AudioState;

static AUDIO_ErrorTypeDef GetFileInfo(const char * fileName, WAVE_FormatTypeDef *info);
static uint8_t PlayerInit(uint32_t AudioFreq);

/**
  * @brief  Initializes Audio Interface.
  * @param  None
  * @retval Audio error
  */
AUDIO_ErrorTypeDef AUDIO_PLAYER_Init(void)
{
	if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_AUTO, uwVolume, I2S_AUDIOFREQ_48K) == 0)
	{
		return AUDIO_ERROR_NONE;
	}
	else
	{
		return AUDIO_ERROR_IO;
	}
}

/**
  * @brief  Starts Audio streaming.    
  * @param  idx: File index
  * @retval Audio error
  */ 
AUDIO_ErrorTypeDef AUDIO_PLAYER_Start(const char * fileName)
{
	uint32_t bytesread;
  int i;

	f_close(&WavFile);

	GetFileInfo(fileName, &WaveFormat);

  for (i = strlen(fileName) - 1; i > 0; --i)
  {
    if (fileName[i] == '\\' || fileName[i] == '/')
      break;
  }

  strcpy(playingFileName, fileName + i + 1);

  /*Adjust the Audio frequency */
  PlayerInit(WaveFormat.SampleRate); 

  BufferCtl.state = BUFFER_OFFSET_NONE;

  f_open(&WavFile, fileName, FA_OPEN_EXISTING | FA_READ);

  /* Get Data from USB Flash Disk */
  f_lseek(&WavFile, 0);

  /* Fill whole buffer at first time */
  if(f_read(&WavFile, 
            &BufferCtl.buff[0], 
            AUDIO_OUT_BUFFER_SIZE, 
            (void *)&bytesread) == FR_OK)
  {
    AudioState = AUDIO_STATE_PLAY;
    if(bytesread != 0)
    {
      BSP_AUDIO_OUT_Play((uint16_t*)&BufferCtl.buff[0], AUDIO_OUT_BUFFER_SIZE);
      BufferCtl.fptr = bytesread;
      return AUDIO_ERROR_NONE;
    }
  }
	return AUDIO_ERROR_IO;
}

/**
  * @brief  Manages Audio process. 
  * @param  None
  * @retval Audio error
  */
AUDIO_ErrorTypeDef AUDIO_PLAYER_Process(void)
{
  uint32_t bytesread, elapsed_time;
  AUDIO_ErrorTypeDef audio_error = AUDIO_ERROR_NONE;
  static uint32_t prev_elapsed_time = 0xFFFFFFFF;
  
  switch(AudioState)
  {
  case AUDIO_STATE_PLAY:

    if(BufferCtl.fptr >= WaveFormat.FileSize)
    {
  		BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
      f_close(&WavFile);
  		AudioState = AUDIO_STATE_NEXT;
    }
    
    if(BufferCtl.state == BUFFER_OFFSET_HALF)
    {
      if(f_read(&WavFile, 
                &BufferCtl.buff[0], 
                AUDIO_OUT_BUFFER_SIZE/2, 
                (void *)&bytesread) != FR_OK)
      { 
        BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW); 
        return AUDIO_ERROR_IO;     
      } 
		BufferCtl.state = BUFFER_OFFSET_NONE;
		BufferCtl.fptr += bytesread; 
    }
    
    if(BufferCtl.state == BUFFER_OFFSET_FULL)
    {
      if(f_read(&WavFile, 
                &BufferCtl.buff[AUDIO_OUT_BUFFER_SIZE /2], 
                AUDIO_OUT_BUFFER_SIZE/2, 
                (void *)&bytesread) != FR_OK)
      { 
        BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW); 
        return AUDIO_ERROR_IO;       
      } 
 
      BufferCtl.state = BUFFER_OFFSET_NONE;
      BufferCtl.fptr += bytesread; 
    }
    
    /* Display elapsed time */
    elapsed_time = BufferCtl.fptr / WaveFormat.ByteRate; 

    // sprintf(tmpstr, "ET : %u", elapsed_time);
    // __statAudio(tmpstr);

    if(prev_elapsed_time != elapsed_time)
    {
      prev_elapsed_time = elapsed_time;
      sprintf((char *)tmpstr, "Playing : %s [%02d:%02d]", playingFileName, (int)(elapsed_time /60), (int)(elapsed_time%60));
      __statAudio(tmpstr);
    }
    break;
    
  case AUDIO_STATE_STOP:
    BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
    AudioState = AUDIO_STATE_IDLE; 
    audio_error = AUDIO_ERROR_IO;
    break;
    
  case AUDIO_STATE_NEXT:

    // BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);

    break;    
    
  case AUDIO_STATE_PREVIOUS:

    // BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);

    break;   
    
  case AUDIO_STATE_PAUSE:
    
	__statAudio("Paused");

    BSP_AUDIO_OUT_Pause();
    AudioState = AUDIO_STATE_WAIT;
    break;
    
  case AUDIO_STATE_RESUME:
    __statAudio("Resumed");
    BSP_AUDIO_OUT_Resume();
    AudioState = AUDIO_STATE_PLAY;
    break;
    
  case AUDIO_STATE_VOLUME_UP: 
    if( uwVolume <= 90)
    {
      uwVolume += 10;
    }
    sprintf((char *)tmpstr,  "Volume : %lu ", uwVolume);
    __statAudio(tmpstr);
    AudioState = AUDIO_STATE_PLAY;
    break;
    
  case AUDIO_STATE_VOLUME_DOWN:    
    if( uwVolume >= 10)
    {
      uwVolume -= 10;
    }
    sprintf((char *)tmpstr,  "Volume : %lu ", uwVolume);
    __statAudio(tmpstr);
    AudioState = AUDIO_STATE_PLAY;
    break;
    
  case AUDIO_STATE_WAIT:
  case AUDIO_STATE_IDLE:
  case AUDIO_STATE_INIT:    
  default:

    break;
  }
  return audio_error;
}


/**
  * @brief  Calculates the remaining file size and new position of the pointer.
  * @param  None
  * @retval None
  */
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
  if(AudioState == AUDIO_STATE_PLAY)
  {
    BufferCtl.state = BUFFER_OFFSET_FULL;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(audioTaskHandle, &xHigherPriorityTaskWoken);
  }
}

/**
  * @brief  Manages the DMA Half Transfer complete interrupt.
  * @param  None
  * @retval None
  */
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{ 
  if(AudioState == AUDIO_STATE_PLAY)
  {
    BufferCtl.state = BUFFER_OFFSET_HALF;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(audioTaskHandle, &xHigherPriorityTaskWoken);
  }
}

void AudioTask(const void * args)
{
  AUDIO_PLAYER_Init();

  while(1)
  {    
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200));

    AUDIO_PLAYER_Process();

    vTaskDelay(0);
  }

}


/*******************************************************************************
                            Static Functions
*******************************************************************************/

/**
  * @brief  Gets the file info.
  * @param  file_idx: File index
  * @param  info: Pointer to WAV file info
  * @retval Audio error
  */
static AUDIO_ErrorTypeDef GetFileInfo(const char * fileName, WAVE_FormatTypeDef *info)
{
  uint32_t bytesread;
  uint32_t duration;
  
  if(f_open(&WavFile, fileName, FA_OPEN_EXISTING | FA_READ) == FR_OK) 
  {
    /* Fill the buffer to Send */
    if(f_read(&WavFile, info, sizeof(WaveFormat), (void *)&bytesread) == FR_OK)
    {
      sprintf((char *)tmpstr, "Playing file : %s",
              fileName);
      __log(tmpstr);
       
      sprintf((char *)tmpstr,  "Sample rate : %d Hz", (int)(info->SampleRate));
      __log(tmpstr);
      
      sprintf((char *)tmpstr,  "Channels number : %d", info->NbrChannels);
      __log(tmpstr);
      
      duration = info->FileSize / info->ByteRate; 
      sprintf((char *)tmpstr, "File Size : %d KB [%02d:%02d]", (int)(info->FileSize/1024), (int)(duration/60), (int)(duration%60));
      __log(tmpstr);
  
      sprintf((char *)tmpstr,  "Volume : %lu", uwVolume);
      __log(tmpstr);
      return AUDIO_ERROR_NONE;
    }
    f_close(&WavFile);
  }
  return AUDIO_ERROR_IO;
}

/**
  * @brief  Initializes the Wave player.
  * @param  AudioFreq: Audio sampling frequency
  * @retval None
  */
static uint8_t PlayerInit(uint32_t AudioFreq)
{ 
  /* Initialize the Audio codec and all related peripherals (I2S, I2C, IOExpander, IOs...) */  
  if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, uwVolume, AudioFreq) != 0)
  {
    return 1;
  }
  else
  {
    BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
    return 0;
  } 
}

/*****************************  END OF FILE  *****************************/
