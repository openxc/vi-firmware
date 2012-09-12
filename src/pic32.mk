TARGET = cantranslator

BOARD_TAG = mega_pic32

ARDUINO_LIBS = chipKITCAN chipKITUSBDevice chipKITUSBDevice/utility cJSON
NO_CORE_MAIN_FUNCTION = 1
SKIP_SUFFIX_CHECK = 1

SERIAL_BAUDRATE = 115200

OSTYPE := $(shell uname)

ifndef ARDUINO_PORT
	ifeq ($(OSTYPE),Darwin)
		ARDUINO_PORT = /dev/tty.usbserial*
	else
		ARDUINO_PORT = /dev/ttyUSB*
	endif
endif

EXTRA_CXXFLAGS += -G0 -D__PIC32__ $(CC_SYMBOLS)

CHIPKIT_LIBRARY_AGREEMENT_URL = http://www.digilentinc.com/Agreement.cfm?DocID=DSD-0000318
MICROCHIP_CAN_LIBRARY_EXISTS = $(shell test -d libs/chipKITCAN; echo $$?)
ifneq ($(MICROCHIP_CAN_LIBRARY_EXISTS),0)
$(error chipKIT CAN library missing - download separately from $(CHIPKIT_LIBRARY_AGREEMENT_URL) and place at ./libs/chipKITCAN)
endif

MICROCHIP_USB_LIBRARY_EXISTS = $(shell test -d libs/chipKITUSBDevice; echo $$?)
ifneq ($(MICROCHIP_USB_LIBRARY_EXISTS),0)
$(error chipKIT USB device library missing - download separately from $(CHIPKIT_LIBRARY_AGREEMENT_URL) and place at ./libs/chipKITUSBDevice)
endif

USER_LIB_PATH = ./libs
ARDUINO_MAKEFILE_HOME = libs/arduino.mk
include $(ARDUINO_MAKEFILE_HOME)/chipKIT.mk
