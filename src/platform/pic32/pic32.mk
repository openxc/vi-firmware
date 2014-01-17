BOARD_TAG = mega_pic32

ARDUINO_LIBS = chipKITUSBDevice chipKITUSBDevice/utility cJSON emqueue \
			   emhashmap/emlist emhashmap AT-commander/atcommander \
			   nanopb
ifeq ($(NETWORK), 1)
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

ifeq ($(MPIDE_DIR),)
	MPIDE_DIR = $(DEPENDENCIES_MPIDE_DIR)
endif

MPIDE_EXISTS = $(shell test -d $(MPIDE_DIR); echo $$?)
ifneq ($(MPIDE_EXISTS),0)
$(error MPIDE missing from path "$(MPIDE_DIR)" - run "script/bootstrap.sh")
endif

ifndef CAN_EMULATOR
ARDUINO_LIBS += chipKITCAN
endif

NO_CORE_MAIN_FUNCTION = 1
SKIP_SUFFIX_CHECK = 1

MONITOR_BAUDRATE = -b 115200

OSTYPE := $(shell uname)

# Backwards compatibility with people using old name for this
MONITOR_PORT ?= $(SERIAL_PORT)

ifndef MONITOR_PORT
	ifeq ($(OSTYPE),Darwin)
		MONITOR_PORT = /dev/tty.usbserial*
	else
		OSTYPE := $(shell uname -o)
		ifeq ($(OSTYPE),Cygwin)
			MONITOR_PORT = com3
		else
			MONITOR_PORT = /dev/ttyUSB*
		endif
	endif
endif

# The Arduino-Makefile project builds libraries in isolation,
# but the openxc-message-format depends on nanopb - this is a
# little hack to make sure the header files are always
# available
EXTRA_CFLAGS += -G0 -D__PIC32__ -D_BOARD_MEGA_ -D$(PLATFORM) $(CC_SYMBOLS) \
				  -I $(LIBS_PATH)/openxc-message-format/gen/cpp \
				  -I $(LIBS_PATH)/nanopb
EXTRA_CXXFLAGS += $(EXTRA_CFLAGS) -std=gnu++0x

# bump the head up to 32K from the default
EXTRA_LDFLAGS += -Wl,--defsym=_min_heap_size=32768

CHIPKIT_LIBRARY_AGREEMENT_URL = http://www.digilentinc.com/Agreement.cfm?DocID=DSD-0000318

EXPECTED_USB_LIBRARY_PATH = $(LIBS_PATH)/chipKITUSBDevice
MICROCHIP_USB_LIBRARY_EXISTS = $(shell test -d $(EXPECTED_USB_LIBRARY_PATH); echo $$?)
ifneq ($(MICROCHIP_USB_LIBRARY_EXISTS),0)
$(error chipKIT USB device library missing - run "script/bootstrap.sh" to download)
endif

ifndef CAN_EMULATOR
EXPECTED_CAN_LIBRARY_PATH = $(LIBS_PATH)/chipKITCAN
MICROCHIP_CAN_LIBRARY_EXISTS = $(shell test -d $(EXPECTED_CAN_LIBRARY_PATH); echo $$?)
ifneq ($(MICROCHIP_CAN_LIBRARY_EXISTS),0)
$(error chipKIT CAN library missing - run "script/bootstrap.sh" to download)
endif
endif

ifdef NETWORK
EXPECTED_ETHERNET_LIBRARY_PATH = $(LIBS_PATH)/chipKITEthernet
MICROCHIP_ETHERNET_LIBRARY_EXISTS = $(shell test -d $(EXPECTED_ETHERNET_LIBRARY_PATH); echo $$?)
ifneq ($(MICROCHIP_ETHERNET_LIBRARY_EXISTS),0)
$(error chipKIT Network library missing - run "script/bootstrap.sh" to download)
endif
endif

ARDUINO_MK_EXISTS = $(shell test -e $(LIBS_PATH)/arduino.mk/arduino-mk/chipKIT.mk; echo $$?)
ifneq ($(ARDUINO_MK_EXISTS),0)
$(error arduino.mk library missing - run "script/bootstrap.sh")
endif

USER_LIB_PATH = $(LIBS_PATH)
ARDUINO_MAKEFILE_HOME = $(LIBS_PATH)/arduino.mk/arduino-mk

LOCAL_C_SRCS = $(CROSSPLATFORM_C_SRCS) $(wildcard platform/pic32/*.c)
LOCAL_CPP_SRCS = $(CROSSPLATFORM_CPP_SRCS) $(wildcard platform/pic32/*.cpp)

include $(ARDUINO_MAKEFILE_HOME)/chipKIT.mk

flash: upload
