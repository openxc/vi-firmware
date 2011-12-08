ARDUINO_DIR = $(ARDUINO_HOME)
AVR_TOOLS_PATH = $(ARDUINO_DIR)/hardware/pic32/compiler/pic32-tools/bin
ARDUINO_CORE_PATH = $(ARDUINO_DIR)/hardware/pic32/cores/pic32
ARDUINO_LIB_PATH  = $(ARDUINO_SKETCHBOOK)/libraries

CC_NAME = pic32-gcc
CXX_NAME = pic32-g++
AR_NAME = pic32-ar
OBJDUMP_NAME = pic32-objdump
OBJCOPY_NAME = pic32-objcopy

NO_MCU=true
F_CPU=80000000L

TARGET       = cantranslator
BOARD_TAG    = max32
ARDUINO_PORT = /dev/ttyUSB*

ARDUINO_LIBS = chipKITCAN chipKITUSBDevice chipKITUSBDevice/utility

MPROCESSOR = -mprocessor=32MX795F512L
EXTRA_CPPFLAGS = -O2  -mno-smart-io $(MPROCESSOR) -DARDUINO=22 -D_BOARD_MEGA_=
EXTRA_LDFLAGS = $(MPROCESSOR)
NO_GNU99=true

include $(PROJECTS)/arduino.mk/Arduino.mk
