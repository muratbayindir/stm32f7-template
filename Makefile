# ------------------------------------------------
#
# Generic Makefile (based on gcc)
#
# ------------------------------------------------

######################################
# target
######################################
TARGET = stm32f746g-disco-template


######################################
# building variables
######################################
# debug build?
DEBUG = 0
# optimization
OPT = -O3


#######################################
# paths
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES = Drivers/CMSIS/Device/ST/STM32F7xx/Source/Templates/system_stm32f7xx.c \
			$(wildcard Drivers/STM32F7xx_HAL_Driver/Src/*.c) \
			$(wildcard Drivers/BSP/STM32746G-Discovery/*.c) \
			Drivers/BSP/Components/ft5336/ft5336.c \
			Drivers/BSP/Components/wm8994/wm8994.c \
			$(wildcard Middlewares/Third_Party/FreeRTOS/Source/*.c) \
			Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/port.c \
			Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_5.c \
			Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c \
			Utilities/CPU/cpu_utils.c \
			$(wildcard Middlewares/ST/STemWin/Config/*.c) \
			Middlewares/ST/STemWin/OS/GUI_X_OS.c \
			$(wildcard Middlewares/Third_Party/FatFs/source/*.c) \
			Middlewares/Third_Party/FatFs/source/drivers/sd_diskio_dma_rtos.c \
			Middlewares/Third_Party/FatFs/source/drivers/usbh_diskio_dma.c \
			Middlewares/Third_Party/FatFs/source/option/syscall.c \
			$(wildcard Middlewares/ST/STM32_USB_Host_Library/Core/Src/*.c) \
			$(wildcard Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Src/*.c) \
			$(wildcard Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/*.c) \
			$(wildcard Middlewares/Third_Party/LwIP/src/api/*.c) \
			$(wildcard Middlewares/Third_Party/LwIP/src/core/*.c) \
			$(wildcard Middlewares/Third_Party/LwIP/src/core/ipv4/*.c) \
			$(wildcard Middlewares/Third_Party/LwIP/src/apps/httpd/*.c) \
			$(wildcard Middlewares/Third_Party/LwIP/src/netif/*.c) \
			Middlewares/Third_Party/LwIP/src/system/OS/sys_arch.c \
			$(wildcard Application/Core/*.c) \
			$(wildcard Application/emWin/*.c) \
			$(wildcard Application/SD/*.c) \
			$(wildcard Application/USB/*.c) \
			$(wildcard Application/Audio/*.c) \
			Application/ETH/app_ethernet.c \
			Application/ETH/ethernetif.c \
			Application/ETH/httpserver-netconn.c

			
# 			Middlewares/FatFs/option/unicode.c \

# ASM sources
ASM_SOURCES =  \
Drivers/CMSIS/Device/ST/STM32F7xx/Source/Templates/gcc/startup_stm32f746xx.s


#######################################
# binaries
#######################################
# GCC_PATH = /opt/gcc-arm-none-eabi-7-2018-q2-update/bin
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m7

# fpu
FPU = -mfpu=fpv5-sp-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS = 

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F746xx \
-DUSE_STM32746G_DISCOVERY \
-DARM_MATH_CM7 \
-D__FPU_PRESENT=1 \
-DUSE_USB_HS


# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES =  \
-IDrivers/CMSIS/Device/ST/STM32F7xx/Include \
-IDrivers/CMSIS/Include \
-IDrivers/STM32F7xx_HAL_Driver/Inc \
-IDrivers/BSP/STM32746G-Discovery \
-IDrivers/BSP/Components/Common \
-IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1 \
-IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
-IMiddlewares/Third_Party/FreeRTOS/Source/include \
-IMiddlewares/Third_Party/FatFs/source \
-IMiddlewares/Third_Party/FatFs/source/drivers \
-IMiddlewares/Third_Party/LwIP/src/include \
-IMiddlewares/Third_Party/LwIP/src/system \
-IMiddlewares/ST/STM32_USB_Host_Library/Core/Inc \
-IMiddlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc \
-IMiddlewares/ST/STM32_USB_Host_Library/Class/HID/Inc \
-IMiddlewares/ST/STemWin/inc \
-IMiddlewares/ST/STemWin/Config \
-IUtilities/CPU \
-IApplication/Audio \
-IApplication/Core \
-IApplication/emWin \
-IApplication/ETH \
-IApplication/SD \
-IApplication/USB \

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -std=c99 -fsingle-precision-constant

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32F746NGHx_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys \
Middlewares/ST/STemWin/Lib/libSTemWin_CM7_OS_wc32.a \
Drivers/CMSIS/Lib/GCC/libarm_cortexM7lfdp_math.a
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections -u _printf_float

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@	


#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)
#stm32_programmer
#D:\Programs\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe
flash:	
	D:\\Programs\\STMicroelectronics\\STM32Cube\\STM32CubeProgrammer\\bin\\STM32_Programmer_CLI.exe --connect port=SWD --halt --write '$(BUILD_DIR)/$(TARGET).hex' --verify --start 0x8000000

program:	
	stm32_programmer --connect port=SWD --halt --write '$(BUILD_DIR)/$(TARGET).hex' --verify --start 0x8000000



# *** EOF ***