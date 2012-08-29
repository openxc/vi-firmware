GCC_BIN =
PROJECT = cantranslator
SYS_OBJECTS = ./mbed/LPC1768/GCC_ARM/startup_LPC17xx.o \
			  ./mbed/LPC1768/GCC_ARM/cmsis_nvic.o \
			  ./mbed/LPC1768/GCC_ARM/core_cm3.o \
			  ./mbed/LPC1768/GCC_ARM/system_LPC17xx.o
INCLUDE_PATHS = -I. -I./mbed -I./mbed/LPC1768 -I./mbed/LPC1768/GCC_ARM \
				-I./libs/cJSON -I./libs/nxpusblib/Driver \
				-I./libs/CDL/LPC17xxLib/inc
LIBRARY_PATHS = -L./mbed/LPC1768/GCC_ARM
LIBRARIES = -lmbed -lcapi
LINKER_SCRIPT = ./mbed/LPC1768/GCC_ARM/LPC1768.ld
OBJDIR  	  = build

###############################################################################

CC = $(GCC_BIN)arm-none-eabi-gcc
CPP = $(GCC_BIN)arm-none-eabi-g++
CC_FLAGS = -c -fno-common \
		   -fmessage-length=0 -Wall -fno-exceptions \
		   -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections \
		   -Wno-char-subscripts -Wno-unused-but-set-variable
ONLY_C_FLAGS = -std=gnu99
ONLY_CPP_FLAGS = -std=gnu++98
CC_SYMBOLS = -DTARGET_LPC1768 -DTOOLCHAIN_GCC_ARM \
			 -DUSB_DEVICE_ONLY -D__LPC17XX__ 


AS = $(GCC_BIN)arm-none-eabi-as

LD = $(GCC_BIN)arm-none-eabi-g++
LD_FLAGS = -mcpu=cortex-m3 -mthumb -Wl,--gc-sections
LD_SYS_LIBS = -lstdc++ -lsupc++ -lm -lc -lgcc

OBJCOPY = $(GCC_BIN)arm-none-eabi-objcopy

LOCAL_C_SRCS    = $(wildcard *.c)
LOCAL_C_SRCS    += $(wildcard usb/*.c)
LOCAL_CPP_SRCS  = $(wildcard *.cpp)
LOCAL_OBJ_FILES = $(LOCAL_C_SRCS:.c=.o) $(LOCAL_CPP_SRCS:.cpp=.o)
OBJECTS = $(patsubst %,$(OBJDIR)/%,$(LOCAL_OBJ_FILES)) $(OBJDIR)/libs/cJSON.o

TARGET_BIN = $(OBJDIR)/$(PROJECT).bin
TARGET_ELF = $(OBJDIR)/$(PROJECT).elf

ifdef DEBUG
CC_FLAGS += -g -ggdb -DDEBUG
else
CC_FLAGS += -O2 -DNDEBUG
endif

all: $(OBJDIR) $(TARGET_BIN)

$(OBJDIR):
		@mkdir -p $(OBJDIR)
		@mkdir -p $(OBJDIR)/usb

flash: all
	@openocd -f config/flash.cfg


clean:
	rm -rf $(OBJDIR)

.s.o:
	$(AS) $(CC_FLAGS) $(CC_SYMBOLS) -o $@ $<

$(OBJDIR)/libs/cJSON.o: libs/cJSON/cJSON.c
	mkdir -p $(dir $@)
	$(CC) -c -lm $(CC_FLAGS) $< -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_C_FLAGS)   $(INCLUDE_PATHS) -o $@ $<

$(OBJDIR)/%.o: %.cpp
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
					 usbutil.o log.o listener.o serialutil.o
TESTABLE_OBJS = $(addprefix $(OBJDIR)/,$(TESTABLE_OBJ_FILES)) \
				$(OBJDIR)/libs/cJSON.o $(TEST_OBJDIR)/helpers.o \
				$(TEST_OBJDIR)/usbutil_mock.o $(TEST_OBJDIR)/serialutil_mock.o

test: LD = g++
test: CC = g++
test: CPP = g++
test: CC_FLAGS = -I. -c -std=gnu99 -m32 -w -Wall -g -ggdb
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
