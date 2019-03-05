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
C_SOURCES = $(wildcard Drivers/CMSIS/Device/STM32F746/*.c) \
			$(wildcard Drivers/STM32F7xx_HAL_Driver/Src/*.c) \
			$(wildcard Drivers/BSP/STM32746G-Discovery/*.c) \
			Drivers/BSP/Components/ft5336/ft5336.c \
			Drivers/BSP/Components/wm8994/wm8994.c \
			$(wildcard Middlewares/FreeRTOS/*.c) \
			Middlewares/FreeRTOS/portable/GCC/ARM_CM7/port.c \
			Middlewares/FreeRTOS/portable/MemMang/heap_5.c \
			Middlewares/FreeRTOS/CMSIS_RTOS/cmsis_os.c \
			Middlewares/STemWin/OS/GUI_X_OS.c \
			Utilities/CPU/cpu_utils.c \
			$(wildcard Middlewares/FatFs/*.c) \
			Middlewares/FatFs/drivers/sd_diskio_dma_rtos.c \
			Middlewares/FatFs/option/syscall.c \
			Middlewares/FatFs/option/unicode.c \
			$(wildcard Middlewares/LwIP/api/*.c) \
			$(wildcard Middlewares/LwIP/core/*.c) \
			$(wildcard Middlewares/LwIP/core/ipv4/*.c) \
			$(wildcard Middlewares/LwIP/apps/httpd/*.c) \
			$(wildcard Middlewares/LwIP/netif/*.c) \
			Middlewares/LwIP/system/OS/sys_arch.c \
			$(wildcard App/Core/*.c) \
			$(wildcard App/emWin/*.c) \
			$(wildcard App/FatFs/*.c) \
			App/ETH/app_ethernet.c \
			App/ETH/ethernetif.c \
			App/ETH/httpserver-netconn.c

# ASM sources
ASM_SOURCES =  \
Drivers/CMSIS/Device/STM32F746/startup_stm32f746xx.s


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
-D__FPU_PRESENT=1


# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES =  \
-IDrivers/STM32F7xx_HAL_Driver/Inc \
-IDrivers/STM32F7xx_HAL_Driver/Inc/Legacy \
-IDrivers/CMSIS/Device/STM32F746 \
-IDrivers/CMSIS/Include \
-IDrivers/BSP/STM32746G-Discovery \
-IDrivers/BSP/Components/Common \
-IMiddlewares/FreeRTOS/portable/GCC/ARM_CM7 \
-IMiddlewares/FreeRTOS/CMSIS_RTOS \
-IMiddlewares/FreeRTOS/include \
-IMiddlewares/FatFs \
-IMiddlewares/STemWin/inc \
-IUtilities/CPU \
-IApp/Core \
-IApp/emWin \
-IApp/FatFs \
-IMiddlewares/FatFs/drivers \
-IApp/FreeRTOS \
-IApp/ETH \
-IMiddlewares/LwIP/include \
-IMiddlewares/LwIP/system

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
Middlewares/STemWin/Lib/STemWin_CM7_OS_wc32_ARGB.a \
Drivers/CMSIS/Lib/libarm_cortexM7lfdp_math.a
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections -u _printf_float

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


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

program:	
	stm32_programmer --connect port=SWD --halt --write '$(BUILD_DIR)/$(TARGET).hex' --verify --start 0x8000000



# *** EOF ***