TARGET       = cantranslator
BOARD_TAG    = mega_pic32

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
