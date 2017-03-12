GCC_GNU_ON_PATH = #$(shell command -v arm-none-eabi-gcc >/dev/null; echo $$?)

ifneq ($(GCC_GNU_ON_PATH),0)
	GCC_BIN = ../dependencies/gcc-arm-embedded/bin/
endif

OPENOCD_CONF_BASE = ../conf/openocd
CMSIS_PATH = $(LIBS_PATH)/CDL/CMSISv2p00_LPC17xx
DRIVER_PATH = $(LIBS_PATH)/CDL/LPC17xxLib
#INCLUDE_PATHS += -I$(LIBS_PATH)/nxpUSBlib/Drivers \
#				-I$(DRIVER_PATH)/inc -I$(LIBS_PATH)/BSP -I$(CMSIS_PATH)/inc

CC = gcc #$(GCC_BIN)arm-none-eabi-gcc
CXX = g++ #$(GCC_BIN)arm-none-eabi-g++
ASFLAGS = -c
SUPRESSED_ERRORS = -Wno-aggressive-loop-optimizations -Wno-char-subscripts \
				   -Wno-unused-but-set-variable
CPPFLAGS = -c $(CC_SYMBOLS) #-fno-common -fmessage-length=0 -Wall -fno-exceptions \
#		   -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections \
#		   $(SUPRESSED_ERRORS) -Werror -DTOOLCHAIN_GCC_ARM \
#		   -DUSB_DEVICE_ONLY -D__LPC17XX__ -DBOARD=9 $(CC_SYMBOLS)
CFLAGS += $(CFLAGS_STD)
CXXFLAGS += $(CXXFLAGS_STD)

ifeq ($(PLATFORM), BLUEBOARD)
	CPPFLAGS += -DBLUEBOARD
else
	#CPPFLAGS += -DFORDBOARD
endif

AS = as #$(GCC_BIN)arm-none-eabi-as
LD = g++ #$(GCC_BIN)arm-none-eabi-g++
#LDFLAGS = -mcpu=cortex-m3 -mthumb \
		  -Wl,--gc-sections,-Map=$(OBJDIR)/$(BASE_TARGET).map -Llpc17xx
LDFLAGS = -Wl,--gc-sections,-Map=$(OBJDIR)/$(BASE_TARGET).map
LD_SYS_LIBS = -lstdc++ -lrt -lpthread#-lsupc++ -lm -lc -lgcc

OBJCOPY = objcopy #$(GCC_BIN)arm-none-eabi-objcopy

GNU_C_SRCS = $(CROSSPLATFORM_C_SRCS) $(wildcard platform/linux/*.c)
#LIB_C_SRCS += $(wildcard $(LIBS_PATH)/nxpUSBlib/Drivers/USB/Core/*.c)
#LIB_C_SRCS += $(wildcard $(LIBS_PATH)/nxpUSBlib/Drivers/USB/Core/LPC/*.c)
#LIB_C_SRCS += $(wildcard $(LIBS_PATH)/nxpUSBlib/Drivers/USB/Core/LPC/HAL/LPC17XX/*.c)
#LIB_C_SRCS += $(wildcard $(LIBS_PATH)/nxpUSBlib/Drivers/USB/Core/LPC/DCD/LPC17XX/*.c)
#LIB_C_SRCS += $(wildcard $(LIBS_PATH)/nxpUSBlib/Drivers/USB/Core/LPC/DCD/LPC17XX/*.c)
#LIB_C_SRCS += $(wildcard $(LIBS_PATH)/BSP/*.c)
#LIB_C_SRCS += $(wildcard $(LIBS_PATH)/BSP/LPCXpressoBase_RevB/*.c)
#LIB_C_SRCS += $(CMSIS_PATH)/src/core_cm3.c
#LIB_C_SRCS += $(CMSIS_PATH)/src/system_LPC17xx.c
#LIB_C_SRCS += $(wildcard $(DRIVER_PATH)/src/*.c)
GNU_CPP_SRCS = $(CROSSPLATFORM_CPP_SRCS) $(wildcard platform/linux/*.cpp)
#GNU_CPP_SRCS = $(wildcard platform/linux/test.cpp)
GNU_OBJ_FILES = $(GNU_C_SRCS:.c=.o) $(GNU_CPP_SRCS:.cpp=.o) $(LIB_C_SRCS:.c=.o)
#GNU_OBJ_FILES = $(GNU_CPP_SRCS:.cpp=.o)
OBJECTS = $(patsubst %,$(OBJDIR)/%,$(GNU_OBJ_FILES))

TARGET_BIN = $(OBJDIR)/$(TARGET).bin

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
	@echo $(INCLUDE_PATHS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TARGET_BIN): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LD_SYS_LIBS)

clean::
	rm -rf $(OBJDIR)
