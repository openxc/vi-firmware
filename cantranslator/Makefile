TARGET       = cantranslator
ifdef DEBUG
BOARD_TAG    = mega_pic32_dbg
else
BOARD_TAG    = mega_pic32
endif

ARDUINO_LIBS = chipKITCAN chipKITUSBDevice chipKITUSBDevice/utility

SERIAL_BAUDRATE = 115200

OSTYPE := $(shell uname)

ifndef ARDUINO_PORT
	ifeq ($(OSTYPE),Darwin)
		ARDUINO_PORT = /dev/tty.usbserial*
	else
		ARDUINO_PORT = /dev/ttyUSB*
	endif
endif

include $(ARDUINO_MAKEFILE_HOME)/chipKIT.mk

TEST_SRC=$(wildcard tests/*.c)
TESTS=$(patsubst %.c,$(OBJDIR)/%,$(TEST_SRC))

TEST_OBJDIR = tests
TEST_LIBS = -lcheck

TESTABLE_OBJ_FILES = canutil.o bitfield.o
TESTABLE_OBJS = $(patsubst %,$(OBJDIR)/%,$(TESTABLE_OBJ_FILES))

.PHONY: test
test: CXX = g++
test: CPPFLAGS = -I. -w -Wall -g -ggdb
test: $(TESTS)
	@sh tests/runtests.sh $(OBJDIR)/$(TEST_OBJDIR)

$(TESTS): $(TEST_SRC) $(TESTABLE_OBJS)
	@mkdir -p $(OBJDIR)/$(TEST_OBJDIR)
	$(CXX) $(CPPFLAGS) $(TESTABLE_OBJS) $< -o $@ $(TEST_LIBS)
