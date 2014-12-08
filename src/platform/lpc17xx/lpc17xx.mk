GCC_ARM_ON_PATH = $(shell command -v arm-none-eabi-gcc >/dev/null; echo $$?)

ifneq ($(GCC_ARM_ON_PATH),0)
GCC_BIN = ../dependencies/gcc-arm-embedded/bin/
endif

ifndef JTAG_INTERFACE
	JTAG_INTERFACE = olimex-arm-usb-ocd-custom
endif

OPENOCD_CONF_BASE = ../conf/openocd
CMSIS_PATH = $(LIBS_PATH)/CDL/CMSISv2p00_LPC17xx
DRIVER_PATH = $(LIBS_PATH)/CDL/LPC17xxLib
INCLUDE_PATHS += -I$(LIBS_PATH)/nxpUSBlib/Drivers \
				-I$(DRIVER_PATH)/inc -I$(LIBS_PATH)/BSP -I$(CMSIS_PATH)/inc
ifeq ($(BOOTLOADER), 1)
LINKER_SCRIPT = platform/lpc17xx/LPC17xx-bootloader.ld
else
LINKER_SCRIPT = platform/lpc17xx/LPC17xx-baremetal.ld
endif

CC = $(GCC_BIN)arm-none-eabi-gcc
CXX = $(GCC_BIN)arm-none-eabi-g++
ASFLAGS = -c -mcpu=cortex-m3 -mthumb --defsym RAM_MODE=0
SUPRESSED_ERRORS = -Wno-aggressive-loop-optimizations -Wno-char-subscripts \
				   -Wno-unused-but-set-variable
CPPFLAGS = -c -fno-common -fmessage-length=0 -Wall -fno-exceptions \
		   -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections \
		   $(SUPRESSED_ERRORS) -Werror -DTOOLCHAIN_GCC_ARM \
		   -DUSB_DEVICE_ONLY -D__LPC17XX__ -DBOARD=9 $(CC_SYMBOLS)
CFLAGS += $(CFLAGS_STD)
CXXFLAGS += $(CXXFLAGS_STD)

ifeq ($(PLATFORM), BLUEBOARD)
CPPFLAGS += -DBLUEBOARD
else
CPPFLAGS += -DFORDBOARD
endif

AS = $(GCC_BIN)arm-none-eabi-as
LD = $(GCC_BIN)arm-none-eabi-g++
LDFLAGS = -mcpu=cortex-m3 -mthumb \
		  -Wl,--gc-sections,-Map=$(OBJDIR)/$(BASE_TARGET).map -Llpc17xx
LD_SYS_LIBS = -lstdc++ -lsupc++ -lm -lc -lgcc

OBJCOPY = $(GCC_BIN)arm-none-eabi-objcopy

ARM_C_SRCS = $(CROSSPLATFORM_C_SRCS) $(wildcard platform/lpc17xx/*.c)
LIB_C_SRCS += $(wildcard $(LIBS_PATH)/nxpUSBlib/Drivers/USB/Core/*.c)
LIB_C_SRCS += $(wildcard $(LIBS_PATH)/nxpUSBlib/Drivers/USB/Core/LPC/*.c)
LIB_C_SRCS += $(wildcard $(LIBS_PATH)/nxpUSBlib/Drivers/USB/Core/LPC/HAL/LPC17XX/*.c)
LIB_C_SRCS += $(wildcard $(LIBS_PATH)/nxpUSBlib/Drivers/USB/Core/LPC/DCD/LPC17XX/*.c)
LIB_C_SRCS += $(wildcard $(LIBS_PATH)/nxpUSBlib/Drivers/USB/Core/LPC/DCD/LPC17XX/*.c)
LIB_C_SRCS += $(wildcard $(LIBS_PATH)/BSP/*.c)
LIB_C_SRCS += $(wildcard $(LIBS_PATH)/BSP/LPCXpressoBase_RevB/*.c)
LIB_C_SRCS += $(CMSIS_PATH)/src/core_cm3.c
LIB_C_SRCS += $(CMSIS_PATH)/src/system_LPC17xx.c
LIB_C_SRCS += $(wildcard $(DRIVER_PATH)/src/*.c)
ARM_CPP_SRCS = $(CROSSPLATFORM_CPP_SRCS) $(wildcard platform/lpc17xx/*.cpp)
ARM_OBJ_FILES = $(ARM_C_SRCS:.c=.o) $(ARM_CPP_SRCS:.cpp=.o) $(LIB_C_SRCS:.c=.o)
OBJECTS = $(patsubst %,$(OBJDIR)/%,$(ARM_OBJ_FILES))

TARGET_BIN = $(OBJDIR)/$(TARGET).bin
TARGET_ELF = $(OBJDIR)/$(TARGET).elf

ifeq ($(DEBUG), 1)
CPPFLAGS += -g -ggdb
else
CPPFLAGS += -Os -Wno-uninitialized
endif

BSP_EXISTS = $(shell test -e $(LIBS_PATH)/BSP/bsp.h; echo $$?)
CDL_EXISTS = $(shell test -e $(LIBS_PATH)/CDL/README.mkd; echo $$?)
USBLIB_EXISTS = $(shell test -e $(LIBS_PATH)/nxpUSBlib/README.mkd; echo $$?)
ifneq ($(BSP_EXISTS),0)
$(error BSP dependency is missing - did you run "git submodule init && git submodule update"?)
endif

ifneq ($(CDL_EXISTS),0)
$(error CDL dependency is missing - did you run "git submodule init && git submodule update"?)
endif

ifneq ($(USBLIB_EXISTS),0)
$(error nxpUSBlib dependency is missing - did you run "git submodule init && git submodule update"?)
endif

all: $(TARGET_BIN)

flash: custom_all
	@echo "Flashing $(PLATFORM) via JTAG with OpenOCD..."
	openocd -s $(OPENOCD_CONF_BASE) -c 'set FIRMWARE_PATH $(TARGET_BIN)' -f $(PLATFORM).cfg -f $(BASE_TARGET).cfg -f interface/$(JTAG_INTERFACE).cfg -f flash.cfg
	@echo "$(GREEN)Flashed $(PLATFORM) successfully.$(COLOR_RESET)"

gdb:
	@openocd -f $(OPENOCD_CONF_BASE)/gdb.cfg

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<

$(OBJECTS): .firmware_options

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE_PATHS) -o $@ $<

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TARGET_ELF): $(OBJECTS)
	$(LD) $(LDFLAGS) -T$(LINKER_SCRIPT) -o $@ $^ $(LD_SYS_LIBS)

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

ispflash: all
	@lpc21isp -bin $(TARGET_BIN) $(UART_PORT) 115200 1474

clean::
	rm -rf $(OBJDIR)
