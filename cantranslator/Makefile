TARGET       = cantranslator

ifdef DEBUG
BOARD_TAG    = mega_pic32_dbg
else
BOARD_TAG    = mega_pic32
endif

ARDUINO_LIBS = chipKITCAN chipKITUSBDevice chipKITUSBDevice/utility cJSON
NO_CORE_MAIN_FUNCTION = 1

SERIAL_BAUDRATE = 115200

OSTYPE := $(shell uname)

ifndef ARDUINO_PORT
	ifeq ($(OSTYPE),Darwin)
		ARDUINO_PORT = /dev/tty.usbserial*
	else
		ARDUINO_PORT = /dev/ttyUSB*
	endif
endif

EXTRA_CXXFLAGS += -G0

include $(ARDUINO_MAKEFILE_HOME)/chipKIT.mk

TEST_OBJDIR = $(OBJDIR)/tests

TEST_SRC=$(wildcard tests/*.cpp)
TESTS=$(patsubst %.cpp,$(OBJDIR)/%,$(TEST_SRC))
TEST_LIBS = -lcheck

TESTABLE_OBJ_FILES = canutil.o canwrite.o canread.o bitfield.o
TESTABLE_OBJS = $(addprefix $(OBJDIR)/,$(TESTABLE_OBJ_FILES))

test: CXX = g++
test: CC = g++
test: CPPFLAGS = -I. -w -Wall -g -ggdb -I$(USER_LIB_PATH)/cJSON \
		-I$(USER_LIB_PATH)/chipKITCAN
test: CXXFLAGS =
test: LDFLAGS =
test: LDLIBS = $(TEST_LIBS)
test: $(TEST_OBJDIR) $(TESTS)
	@sh tests/runtests.sh $(TEST_OBJDIR)

$(TEST_OBJDIR):
	@mkdir -p $(TEST_OBJDIR)

$(OBJDIR)/libs/cJSON.o: $(USER_LIB_PATH)/cJSON/cJSON.c
	mkdir -p $(dir $@)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

# TODO it would be nice to make these implicit rules to pick up any new tests
$(TEST_OBJDIR)/bitfield_tests: $(TEST_OBJDIR)/bitfield_tests.o $(TESTABLE_OBJS) $(OBJDIR)/libs/cJSON.o

$(TEST_OBJDIR)/canutil_tests: $(TEST_OBJDIR)/canutil_tests.o $(TESTABLE_OBJS) $(OBJDIR)/libs/cJSON.o

