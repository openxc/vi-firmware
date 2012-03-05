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

TEST_CC = gcc
TEST_CPPFLAGS = -I.
TEST_LIBS = -lcheck
TEST_OBJDIR = tests

.PHONY: test
test: $(TESTS)
	@sh tests/runtests.sh $(OBJDIR)/$(TEST_OBJDIR)

$(TESTS): $(TEST_SRC)
	@mkdir -p $(OBJDIR)/$(TEST_OBJDIR)
	$(TEST_CC) $(TEST_CPPFLAGS) $< -o $@ $(TEST_LIBS)

libopenxc.so: CFLAGS+=-fPIC -shared
libopenxc.so: lib
