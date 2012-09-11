GCC_BIN =
PROJECT = cantranslator
CMSIS_PATH = ./libs/CDL/CMSISv2p00_LPC17xx
DRIVER_PATH = ./libs/CDL/LPC17xxLib
SYS_OBJECTS =
INCLUDE_PATHS = -I. -I./libs/cJSON -I./libs/nxpUSBlib/Drivers \
				-I$(DRIVER_PATH)/inc -I./libs/BSP -I$(CMSIS_PATH)/inc
LIBRARY_PATHS =
LIBRARIES =
LINKER_SCRIPT = lpc1768/LPC1768.ld
OBJDIR  	  = build


###############################################################################

CC = $(GCC_BIN)arm-none-eabi-gcc
CPP = $(GCC_BIN)arm-none-eabi-g++
AS_FLAGS = -c -mcpu=cortex-m3 -mthumb --defsym RAM_MODE=0
CC_FLAGS = -c -fno-common -fmessage-length=0 -Wall -fno-exceptions \
		   -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections \
		   -Wno-char-subscripts -Wno-unused-but-set-variable -Werror
ONLY_C_FLAGS = -std=gnu99
ONLY_CPP_FLAGS = -std=gnu++0x
# TODO Build a BSP for the blueboard
CC_SYMBOLS = -DTARGET_LPC1768 -DTOOLCHAIN_GCC_ARM \
			 -DUSB_DEVICE_ONLY -D__LPC17XX__ -DBOARD=9


AS = $(GCC_BIN)arm-none-eabi-as

LD = $(GCC_BIN)arm-none-eabi-g++
LD_FLAGS = -mcpu=cortex-m3 -mthumb -Wl,--gc-sections
LD_SYS_LIBS = -lstdc++ -lsupc++ -lm -lc -lgcc

OBJCOPY = $(GCC_BIN)arm-none-eabi-objcopy

LOCAL_C_SRCS    = $(wildcard *.c)
LOCAL_C_SRCS    += $(wildcard libs/nxpUSBlib/Drivers/USB/Core/*.c)
LOCAL_C_SRCS    += $(wildcard libs/nxpUSBlib/Drivers/USB/Core/LPC/*.c)
LOCAL_C_SRCS    += $(wildcard libs/nxpUSBlib/Drivers/USB/Core/LPC/HAL/LPC17XX/*.c)
LOCAL_C_SRCS    += $(wildcard libs/nxpUSBlib/Drivers/USB/Core/LPC/DCD/LPC17XX/*.c)
LOCAL_C_SRCS    += $(wildcard libs/nxpUSBlib/Drivers/USB/Core/LPC/DCD/LPC17XX/*.c)
LOCAL_C_SRCS    += $(wildcard libs/BSP/*.c)
LOCAL_C_SRCS    += $(wildcard libs/BSP/LPCXpressoBase_RevB/*.c)
LOCAL_C_SRCS    += $(CMSIS_PATH)/src/core_cm3.c \
				   $(CMSIS_PATH)/src/system_LPC17xx.c
LOCAL_C_SRCS    += $(wildcard $(DRIVER_PATH)/src/*.c)
LOCAL_CPP_SRCS  = $(wildcard *.cpp)
LOCAL_OBJ_FILES = $(LOCAL_C_SRCS:.c=.o) $(LOCAL_CPP_SRCS:.cpp=.o)
OBJECTS = $(patsubst %,$(OBJDIR)/%,$(LOCAL_OBJ_FILES)) $(OBJDIR)/libs/cJSON.o \
		  $(OBJDIR)/lpc1768/startup.o $(OBJDIR)/lpc1768/fault_handlers.s


TARGET_BIN = $(OBJDIR)/$(PROJECT).bin
TARGET_ELF = $(OBJDIR)/$(PROJECT).elf

ifdef DEBUG
CC_FLAGS += -g -ggdb -DDEBUG
else
# TODO re-enable -O2 when we figure out why IsINReady() returns true
# when the stream isn't completely read by the host, and thus leading to
# corruption
CC_FLAGS += -DNDEBUG
endif

ifdef EMULATOR
CC_FLAGS += -DCAN_EMULATOR
endif

ifdef SERIAL
CC_FLAGS += -DSERIAL
endif

all: $(OBJDIR) $(TARGET_BIN)

flash: all
	@openocd -f config/flash.cfg

.s.o:
	$(AS) $(AS_FLAGS) -o $@ $<

$(OBJDIR)/libs/cJSON.o: libs/cJSON/cJSON.c
	mkdir -p $(dir $@)
	$(CC) -c -lm $(CC_FLAGS) $< -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_C_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(OBJDIR)/%.o: %.cpp
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $<


$(TARGET_ELF): $(OBJECTS) $(SYS_OBJECTS)
	$(LD) $(LD_FLAGS) -T$(LINKER_SCRIPT) $(LIBRARY_PATHS) -o $@ $^ \
			$(LIBRARIES) $(LD_SYS_LIBS)

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

$(OBJDIR):
		@mkdir -p $(OBJDIR)
		@mkdir -p $(OBJDIR)/lpc1768
		@mkdir -p $(OBJDIR)/libs/nxpUSBlib/Drivers/USB/Core/LPC/DCD/LPC17XX
		@mkdir -p $(OBJDIR)/libs/nxpUSBlib/Drivers/USB/Core/LPC/HAL/LPC17XX
		@mkdir -p $(OBJDIR)/libs/nxpUSBlib/Drivers/USB/Core/LPC/HCD
		@mkdir -p $(OBJDIR)/libs/CDL/LPC17xxLib/src
		@mkdir -p $(OBJDIR)/$(CMSIS_PATH)/src
		@mkdir -p $(OBJDIR)/libs/BSP/LPCXpressoBase_RevB
