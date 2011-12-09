ARDUINO_DIR = $(ARDUINO_HOME)
AVR_TOOLS_PATH = $(ARDUINO_DIR)/hardware/pic32/compiler/pic32-tools/bin
ARDUINO_CORE_PATH = $(ARDUINO_DIR)/hardware/pic32/cores/pic32
ARDUINO_LIB_PATH  = $(ARDUINO_SKETCHBOOK)/libraries
BOARDS_TXT  = $(ARDUINO_DIR)/hardware/pic32/boards.txt

CC_NAME = pic32-gcc
CXX_NAME = pic32-g++
AR_NAME = pic32-ar
OBJDUMP_NAME = pic32-objdump
OBJCOPY_NAME = pic32-objcopy

AVRDUDE = $(ARDUINO_DIR)/hardware/tools/avrdude
AVRDUDE_CONF = $(ARDUINO_DIR)/hardware/tools/avrdude.conf

MCU_FLAG_NAME=mprocessor
EXTRA_CPPFLAGS = -O2  -mno-smart-io -DARDUINO=22 -D_BOARD_MEGA_=

TARGET       = cantranslator
BOARD_TAG    = mega_pic32

ifndef ARDUINO_PORT
ARDUINO_PORT = /dev/ttyUSB*
endif

ARDUINO_LIBS = chipKITCAN chipKITUSBDevice chipKITUSBDevice/utility

include $(ARDUINO_MAKEFILE_HOME)/Arduino.mk
