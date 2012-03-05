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
