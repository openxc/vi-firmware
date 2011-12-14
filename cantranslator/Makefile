TARGET       = cantranslator
BOARD_TAG    = mega_pic32

ARDUINO_LIBS = chipKITCAN chipKITUSBDevice chipKITUSBDevice/utility

# These are set manually because YAML for Perl isn't always available
MCU=32MX795F512L # chipKIT Max32
F_CPU=80000000L
AVRDUDE_ARD_PROGRAMMER=stk500v2
AVRDUDE_ARD_BAUDRATE=115200

ifndef ARDUINO_PORT
ARDUINO_PORT = /dev/ttyUSB*
endif

include $(ARDUINO_MAKEFILE_HOME)/chipKIT.mk
