BOARD_TAG = mega_pic32
TARGET = $(BASE_TARGET)-pic32

ARDUINO_LIBS = chipKITUSBDevice chipKITUSBDevice/utility cJSON
ifdef ETHERNET
ARDUINO_LIBS += chipKITEthernet chipKITEthernet/utility
endif

DEPENDENCIES_MPIDE_DIR = $(DEPENDENCIES_FOLDER)/mpide

OSTYPE := $(shell uname)
ifneq ($(OSTYPE),Darwin)
	OSTYPE := $(shell uname -o)
	ifeq ($(OSTYPE),Cygwin)
		# The compiler expects windows-style paths, but the user typically
		# provides a UNIX style path as the MPIDE_DIR. We could convert it with
		# cygpath, but it's hard to do so reliable and it screws up all of the
		# calls to "test" littered throughout these Makefiles. So, we're going
		# to require MPIDE to be installed in the dependencies directory to we
		# can use a relative path.
		MPIDE_DIR =
	endif
endif

ifdef MPIDE_DIR
MPIDE_EXISTS = $(shell test -d $(MPIDE_DIR); echo $$?)

ifneq ($(MPIDE_EXISTS),0)
MPIDE_DIR = $(DEPENDENCIES_MPIDE_DIR)
endif
else
MPIDE_DIR = $(DEPENDENCIES_MPIDE_DIR)
endif

MPIDE_EXISTS = $(shell test -d $(MPIDE_DIR); echo $$?)
ifneq ($(MPIDE_EXISTS),0)
$(error MPIDE missing - run "script/bootstrap.sh")
endif

ifndef CAN_EMULATOR
ARDUINO_LIBS += chipKITCAN
endif

NO_CORE_MAIN_FUNCTION = 1
SKIP_SUFFIX_CHECK = 1
OBJDIR = build/pic32

SERIAL_BAUDRATE = 115200

OSTYPE := $(shell uname)

ifndef SERIAL_PORT
	# Backwards compatibility with people using old name for this
	ifdef ARDUINO_PORT
		SERIAL_PORT := $(ARDUINO_PORT)
	endif
endif

ifndef SERIAL_PORT
	ifeq ($(OSTYPE),Darwin)
		SERIAL_PORT = /dev/tty.usbserial*
	else
		OSTYPE := $(shell uname -o)
		ifeq ($(OSTYPE),Cygwin)
			SERIAL_PORT = com3
		else
			SERIAL_PORT = /dev/ttyUSB*
		endif
	endif
endif

EXTRA_CPPFLAGS += -G0 -D__PIC32__ $(CC_SYMBOLS)

CHIPKIT_LIBRARY_AGREEMENT_URL = http://www.digilentinc.com/Agreement.cfm?DocID=DSD-0000318

EXPECTED_USB_LIBRARY_PATH = ./libs/chipKITUSBDevice
MICROCHIP_USB_LIBRARY_EXISTS = $(shell test -d $(EXPECTED_USB_LIBRARY_PATH); echo $$?)
ifneq ($(MICROCHIP_USB_LIBRARY_EXISTS),0)
$(error chipKIT USB device library missing - run "script/bootstrap.sh" to download)
endif

ifndef CAN_EMULATOR
EXPECTED_CAN_LIBRARY_PATH = ./libs/chipKITCAN
MICROCHIP_CAN_LIBRARY_EXISTS = $(shell test -d $(EXPECTED_CAN_LIBRARY_PATH); echo $$?)
ifneq ($(MICROCHIP_CAN_LIBRARY_EXISTS),0)
$(error chipKIT CAN library missing - run "script/bootstrap.sh" to download)
endif
endif

ifdef ETHERNET
EXPECTED_ETHERNET_LIBRARY_PATH = ./libs/chipKITEthernet
MICROCHIP_ETHERNET_LIBRARY_EXISTS = $(shell test -d $(EXPECTED_ETHERNET_LIBRARY_PATH); echo $$?)
ifneq ($(MICROCHIP_ETHERNET_LIBRARY_EXISTS),0)
$(error chipKIT Ethernet library missing - run "script/bootstrap.sh" to download)
endif
endif

ARDUINO_MK_EXISTS = $(shell test -e libs/arduino.mk/chipKIT.mk; echo $$?)
ifneq ($(ARDUINO_MK_EXISTS),0)
$(error arduino.mk library missing - run "script/bootstrap.sh")
endif

USER_LIB_PATH = ./libs
ARDUINO_MAKEFILE_HOME = libs/arduino.mk

LOCAL_C_SRCS += $(wildcard pic32/*.c)
LOCAL_CPP_SRCS += $(wildcard pic32/*.cpp)

include $(ARDUINO_MAKEFILE_HOME)/chipKIT.mk

flash: upload
