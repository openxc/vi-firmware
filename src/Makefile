GCC_BIN =
PROJECT = cantranslator
CMSIS_PATH = ./libs/CDL/CMSISv2p00_LPC17xx
DRIVER_PATH = ./libs/CDL/LPC17xxLib
SYS_OBJECTS =
INCLUDE_PATHS = -I. -I./libs/cJSON -I./libs/nxpUSBlib/Drivers \
				-I$(DRIVER_PATH)/inc -I./libs/BSP -I$(CMSIS_PATH)/inc
LIBRARY_PATHS =
LIBRARIES =
LINKER_SCRIPT = LPC1768.ld
OBJDIR  	  = build


###############################################################################

CC = $(GCC_BIN)arm-none-eabi-gcc
CPP = $(GCC_BIN)arm-none-eabi-g++
AS_FLAGS = -c -mcpu=cortex-m3 -mthumb --defsym RAM_MODE=0
CC_FLAGS = -c -fno-common \
		   -fmessage-length=0 -Wall -fno-exceptions \
		   -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections \
		   -Wno-char-subscripts -Wno-unused-but-set-variable
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
		  $(OBJDIR)/startup.o fault_handlers.s


TARGET_BIN = $(OBJDIR)/$(PROJECT).bin
TARGET_ELF = $(OBJDIR)/$(PROJECT).elf

ifdef DEBUG
CC_FLAGS += -g -ggdb -DDEBUG
else
CC_FLAGS += -O2 -DNDEBUG
endif

ifdef EMULATOR
CC_FLAGS += -DCAN_EMULATOR
endif

ifdef SERIAL
CC_FLAGS += -DSERIAL
endif

all: $(OBJDIR) $(TARGET_BIN)

$(OBJDIR):
		@mkdir -p $(OBJDIR)
		@mkdir -p $(OBJDIR)/libs/nxpUSBlib/Drivers/USB/Core/LPC/DCD/LPC17XX
		@mkdir -p $(OBJDIR)/libs/nxpUSBlib/Drivers/USB/Core/LPC/HAL/LPC17XX
		@mkdir -p $(OBJDIR)/libs/nxpUSBlib/Drivers/USB/Core/LPC/HCD
		@mkdir -p $(OBJDIR)/libs/CDL/LPC17xxLib/src
		@mkdir -p $(OBJDIR)/$(CMSIS_PATH)/src
		@mkdir -p $(OBJDIR)/libs/BSP/LPCXpressoBase_RevB

flash: all
	@openocd -f config/flash.cfg


clean:
	rm -rf $(OBJDIR)

.s.o:
	$(AS) $(AS_FLAGS) -o $@ $<

$(OBJDIR)/libs/cJSON.o: libs/cJSON/cJSON.c
	mkdir -p $(dir $@)
	$(CC) -c -lm $(CC_FLAGS) $< -o $@

$(OBJDIR)/%.o: %.c %.h
	$(CC) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_C_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(OBJDIR)/%.o: %.cpp %.h
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $<


$(TARGET_ELF): $(OBJECTS) $(SYS_OBJECTS)
	$(LD) $(LD_FLAGS) -T$(LINKER_SCRIPT) $(LIBRARY_PATHS) -o $@ $^ \
			$(LIBRARIES) $(LD_SYS_LIBS)

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

# For running test cases on the development computer, don't compile with
# embedded libraries
TEST_OBJDIR = $(OBJDIR)/tests

TEST_SRC=$(wildcard tests/*_tests.cpp)
TESTS=$(patsubst %.cpp,$(OBJDIR)/%.bin,$(TEST_SRC))
TEST_LIBS = -lcheck

TESTABLE_OBJ_FILES = canutil.o canwrite.o canread.o bitfield.o queue.o \
					 log.o listener.o
TESTABLE_OBJS = $(addprefix $(OBJDIR)/,$(TESTABLE_OBJ_FILES)) \
				$(OBJDIR)/libs/cJSON.o $(TEST_OBJDIR)/helpers.o \
				$(TEST_OBJDIR)/usbutil_mock.o $(TEST_OBJDIR)/serialutil_mock.o

test: LD = g++
test: CC = g++
test: CPP = g++
test: CC_FLAGS = -I. -c -std=gnu99 -m32 -w -Wall -g -ggdb
test: CC_SYMBOLS =
test: LDFLAGS = -m32 -lm
test: LDLIBS = $(TEST_LIBS)
test: $(TEST_OBJDIR) $(TESTS)
	@sh tests/runtests.sh $(TEST_OBJDIR)

$(TEST_OBJDIR):
	@mkdir -p $(TEST_OBJDIR)

$(TEST_OBJDIR)/%.o: tests/%.cpp
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.bin: $(TEST_OBJDIR)/%.o $(TESTABLE_OBJS)
	$(LD) $(LDFLAGS) $(LDLIBS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $^
