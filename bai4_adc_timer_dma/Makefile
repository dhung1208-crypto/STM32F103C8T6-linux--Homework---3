.RECIPEPREFIX := >

TARGET = bai4_adc_uart_dma
BUILD_DIR = build

CUBE = ../stm32-libs/STM32CubeF1

C_SOURCES = \
core/src/main.c \
core/src/stm32f1xx_it.c \
core/src/system_stm32f1xx.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c\
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_spi.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc_ex.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c \
$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_i2c.c

ASM_SOURCES = \
startup/startup_stm32f103xb.s

PREFIX = arm-none-eabi-

CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

CPU = -mcpu=cortex-m3
MCU = $(CPU) -mthumb

C_DEFS = \
-DUSE_HAL_DRIVER \
-DSTM32F103xB

C_INCLUDES = \
-Icore/inc \
-I$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Inc \
-I$(CUBE)/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy \
-I$(CUBE)/Drivers/CMSIS/Device/ST/STM32F1xx/Include \
-I$(CUBE)/Drivers/CMSIS/Include

CFLAGS = \
$(MCU) \
$(C_DEFS) \
$(C_INCLUDES) \
-O0 \
-g3 \
-Wall \
-ffunction-sections \
-fdata-sections \
-MMD \
-MP

ASFLAGS = \
$(MCU) \
-g3

LDSCRIPT = STM32F103C8TX_FLASH.ld

LDFLAGS = \
$(MCU) \
-T$(LDSCRIPT) \
-specs=nano.specs \
-specs=nosys.specs \
-Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
-Wl,--gc-sections

LIBS = -lc -lm

OBJECTS = \
$(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o))) \
$(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

vpath %.c core/src
vpath %.c $(CUBE)/Drivers/STM32F1xx_HAL_Driver/Src
vpath %.s startup

all: $(BUILD_DIR)/$(TARGET).elf \
     $(BUILD_DIR)/$(TARGET).hex \
     $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
>$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
>$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
>$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
>$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
>$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
>$(CP) -O binary -S $< $@

$(BUILD_DIR):
>mkdir -p $(BUILD_DIR)

flash: $(BUILD_DIR)/$(TARGET).elf
>openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

clean:
>rm -rf $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean flash
