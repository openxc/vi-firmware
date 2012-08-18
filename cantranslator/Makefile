GCC_BIN =
PROJECT = cantranslator
SYS_OBJECTS = ./mbed/LPC1768/GCC_ARM/startup_LPC17xx.o \
			  ./mbed/LPC1768/GCC_ARM/cmsis_nvic.o \
			  ./mbed/LPC1768/GCC_ARM/core_cm3.o \
			  ./mbed/LPC1768/GCC_ARM/system_LPC17xx.o
# TODO why do I have to explicitly include /usr/include? is it not on the
# include path for embedded arm?
INCLUDE_PATHS = -I. -I./mbed -I./mbed/LPC1768 -I./mbed/LPC1768/GCC_ARM \
				-I./cJSON -I/usr/include
LIBRARY_PATHS = -L./mbed/LPC1768/GCC_ARM
LIBRARIES = -lmbed -lcapi
LINKER_SCRIPT = ./mbed/LPC1768/GCC_ARM/LPC1768.ld
OBJDIR  	  = build

###############################################################################

CC= $(GCC_BIN)arm-none-eabi-gcc
CPP = $(GCC_BIN)arm-none-eabi-g++
CC_FLAGS = -c -Os -fno-common -fmessage-length=0 -Wall -fno-exceptions \
		   -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections
ONLY_C_FLAGS = -std=gnu99
ONLY_CPP_FLAGS = -std=gnu++98
CC_SYMBOLS = -DTARGET_LPC1768 -DTOOLCHAIN_GCC_ARM -DNDEBUG


AS = $(GCC_BIN)arm-none-eabi-as

LD = $(GCC_BIN)arm-none-eabi-gcc
LD_FLAGS = -mcpu=cortex-m3 -mthumb -Wl,--gc-sections
LD_SYS_LIBS = -lstdc++ -lsupc++ -lm -lc -lgcc

OBJCOPY = $(GCC_BIN)arm-none-eabi-objcopy

LOCAL_C_SRCS    = $(wildcard *.c)
LOCAL_CPP_SRCS  = $(wildcard *.cpp)
LOCAL_CC_SRCS   = $(wildcard *.cc)
LOCAL_PDE_SRCS  = $(wildcard *.$(SUFFIX))
LOCAL_AS_SRCS   = $(wildcard *.S)
LOCAL_OBJ_FILES = $(LOCAL_C_SRCS:.c=.o) $(LOCAL_CPP_SRCS:.cpp=.o) \
		$(LOCAL_CC_SRCS:.cc=.o) $(LOCAL_PDE_SRCS:.$(SUFFIX)=.o) \
		$(LOCAL_AS_SRCS:.S=.o)
OBJECTS      = $(patsubst %,$(OBJDIR)/%,$(LOCAL_OBJ_FILES))

TARGET_BIN = $(OBJDIR)/$(PROJECT).bin
TARGET_ELF = $(OBJDIR)/$(PROJECT).elf

all: $(TARGET_BIN) | $(OBJDIR)

$(OBJDIR):
		@mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR)

.s.o:
	$(AS)  $(CC_FLAGS) $(CC_SYMBOLS) -o $@ $<

$(OBJDIR)/%.o: %.c
	$(CC)  $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_C_FLAGS)   $(INCLUDE_PATHS) -o $@ $<

$(OBJDIR)/%.o: %.cpp
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $<


$(TARGET_ELF): $(OBJECTS) $(SYS_OBJECTS)
	$(LD) $(LD_FLAGS) -T$(LINKER_SCRIPT) $(LIBRARY_PATHS) -o $@ $^ \
			$(LIBRARIES) $(LD_SYS_LIBS) $(LIBRARIES) $(LD_SYS_LIBS)

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

# For running test cases on the development computer, don't compile with
# embedded libraries
TEST_OBJDIR = $(OBJDIR)/tests

TEST_SRC=$(wildcard tests/*.cpp)
TESTS=$(patsubst %.cpp,$(OBJDIR)/%,$(TEST_SRC))
TEST_LIBS = -lcheck

TESTABLE_OBJ_FILES = canutil.o canwrite.o canread.o bitfield.o
TESTABLE_OBJS = $(addprefix $(OBJDIR)/,$(TESTABLE_OBJ_FILES)) \
				$(OBJDIR)/libs/cJSON.o $(TEST_OBJDIR)/helpers.o

test: CXX = g++
test: CC = g++
test: CPPFLAGS = -I. -m32 -w -Wall -g -ggdb $(SYS_INCLUDES)

test: CXXFLAGS =
test: LDFLAGS = -m32
test: LDLIBS = $(TEST_LIBS)
test: $(TEST_OBJDIR) $(TESTS)
	@sh tests/runtests.sh $(TEST_OBJDIR)

$(TEST_OBJDIR):
	@mkdir -p $(TEST_OBJDIR)

$(OBJDIR)/libs/cJSON.o: $(USER_LIB_PATH)/cJSON/cJSON.c
	mkdir -p $(dir $@)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

# TODO it would be nice to make these implicit rules to pick up any new tests
$(TEST_OBJDIR)/bitfield_tests: $(TEST_OBJDIR)/bitfield_tests.o $(TESTABLE_OBJS)

$(TEST_OBJDIR)/canutil_tests: $(TEST_OBJDIR)/canutil_tests.o $(TESTABLE_OBJS)

$(TEST_OBJDIR)/canread_tests: $(TEST_OBJDIR)/canread_tests.o $(TESTABLE_OBJS)

$(TEST_OBJDIR)/canwrite_tests: $(TEST_OBJDIR)/canwrite_tests.o $(TESTABLE_OBJS)
