SYMBOLS =
SHELL := /bin/bash

COLORS_AVAILABLE = $(shell command -v tput >/dev/null 2>&1; echo $$?)
ifeq ($(COLORS_AVAILABLE),0)
RED="$${txtbld}$$(tput setaf 1)"
GREEN="$${txtbld}$$(tput setaf 2)"
YELLOW="$${txtbld}$$(tput setaf 3)"
COLOR_RESET=$$(tput sgr0)
endif

# For message printing: pad the right side of the first argument with spaces to
# the number of bytes indicated by the second argument.
space_pad_to = $(shell echo $(1) "                                                      " | head -c$(2))

# Call with some text, and a prefix tag if desired (like [AUTODETECTED]),
show_vi_config_info = $(info - $(call space_pad_to,$(2),4) $(1))

# Call with the name of the variable, a prefix tag if desired (like [AUTODETECTED]),
# and an explanation if desired (like (found in $$PATH)
show_vi_config_variable = $(call show_vi_config_info,$(call space_pad_to,$($(1)),20)= $(1) $(3),$(2))

show_separator = $(info -------------------------)

BASE_TARGET = vi-firmware
OBJDIR_ROOT = build
DEPENDENCIES_FOLDER = ../dependencies
OBJDIR = $(OBJDIR_ROOT)

# 0 or 1
BOOTLOADER ?= 1

PLATFORM ?= "NONE"
SYMBOLS += PLATFORM="\"$(PLATFORM)\""

DEFAULT_ALLOW_RAW_WRITE_USB ?= 1
SYMBOLS += DEFAULT_ALLOW_RAW_WRITE_USB=$(DEFAULT_ALLOW_RAW_WRITE_USB)

DEFAULT_ALLOW_RAW_WRITE_UART ?= 0
SYMBOLS += DEFAULT_ALLOW_RAW_WRITE_UART=$(DEFAULT_ALLOW_RAW_WRITE_UART)

DEFAULT_ALLOW_RAW_WRITE_NETWORK ?= 0
SYMBOLS += DEFAULT_ALLOW_RAW_WRITE_NETWORK=$(DEFAULT_ALLOW_RAW_WRITE_NETWORK)

DEFAULT_ALLOW_RAW_WRITE_BLE ?= 0
SYMBOLS += DEFAULT_ALLOW_RAW_WRITE_BLE=$(DEFAULT_ALLOW_RAW_WRITE_BLE)

DEFAULT_METRICS_STATUS ?= 0
SYMBOLS += DEFAULT_METRICS_STATUS=$(DEFAULT_METRICS_STATUS)

DEFAULT_LOGGING_OUTPUT ?= "BOTH"
SYMBOLS += DEFAULT_LOGGING_OUTPUT=$(DEFAULT_LOGGING_OUTPUT)

DEFAULT_OBD2_BUS ?= 1
SYMBOLS += DEFAULT_OBD2_BUS=$(DEFAULT_OBD2_BUS)

DEFAULT_RECURRING_OBD2_REQUESTS_STATUS ?= 0
SYMBOLS += DEFAULT_RECURRING_OBD2_REQUESTS_STATUS=$(DEFAULT_RECURRING_OBD2_REQUESTS_STATUS)

# JSON or PROTOBUF
DEFAULT_OUTPUT_FORMAT ?= JSON
SYMBOLS += DEFAULT_OUTPUT_FORMAT=$(DEFAULT_OUTPUT_FORMAT)

# ALWAYS_ON, SILENT_CAN or OBD2_IGNITION_CHECK
DEFAULT_POWER_MANAGEMENT ?= SILENT_CAN
SYMBOLS += DEFAULT_POWER_MANAGEMENT=$(DEFAULT_POWER_MANAGEMENT)

DEFAULT_EMULATED_DATA_STATUS ?= 0
SYMBOLS += DEFAULT_EMULATED_DATA_STATUS=$(DEFAULT_EMULATED_DATA_STATUS)

# 0x1 to 0xffff
DEFAULT_USB_PRODUCT_ID ?= 0x1
SYMBOLS += DEFAULT_USB_PRODUCT_ID=$(DEFAULT_USB_PRODUCT_ID)

DEFAULT_CAN_ACK_STATUS ?= 0
SYMBOLS += DEFAULT_CAN_ACK_STATUS=$(DEFAULT_CAN_ACK_STATUS)

ENVIRONMENT_MODE ?= "default_mode"
SYMBOLS += ENVIRONMENT_MODE="\"$(ENVIRONMENT_MODE)\""

# TODO see https://github.com/openxc/vi-firmware/issues/189
# ifeq ($(NETWORK), 1)
# SYMBOLS += __USE_NETWORK__
# endif

# The DEBUG and TRANSMITTER flags override the CAN_ACK_STATUS and
# POWER_MANAGEMENT flags, so these two must come last.

# 0 or 1
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	SYMBOLS += __DEBUG__
	DEFAULT_POWER_MANAGEMENT = ALWAYS_ON
	DEFAULT_CAN_ACK_STATUS = 1
else
	SYMBOLS += NDEBUG
endif

#0 or 1
MSD_ENABLE ?= 0
ifeq ($(MSD_ENABLE), 1)
	SYMBOLS += __MSD_ENABLE__
endif

#15-secs to 86400-secs upper limits will automatically be truncated
DEFAULT_FILE_GENERATE_SECS ?= 180
SYMBOLS += DEFAULT_FILE_GENERATE_SECS=$(DEFAULT_FILE_GENERATE_SECS)
#endif


TRANSMITTER ?= 0
ifeq ($(TRANSMITTER), 1)
	DEFAULT_POWER_MANAGEMENT = ALWAYS_ON
	DEFAULT_USB_PRODUCT_ID = 0x2
endif

TEST_MODE_ONLY ?= 0
ifeq ($(TEST_MODE_ONLY), 1)
	SYMBOLS += __TEST_MODE__
endif





SYMBOLS += PB_FIELD_16BIT

CC_SYMBOLS = $(addprefix -D,$(SYMBOLS))

CJSON_EXISTS = $(shell test -e libs/cJSON/cJSON.h; echo $$?)
ifneq ($(CJSON_EXISTS),0)
$(error cJSON dependency is missing - run "script/bootstrap.sh")
endif



VALID_PLATFORMS = CHIPKIT BLUEBOARD FORDBOARD CROSSCHASM_C5 CROSSCHASM_C5_BT CROSSCHASM_C5_BLE CROSSCHASM_C5_CELLULAR

#for backwards compatibility
ifeq ($(PLATFORM), CROSSCHASM_C5)
override PLATFORM = CROSSCHASM_C5_BT
endif

OBJDIR = build/$(PLATFORM)
LIBS_PATH = libs
TARGET = $(BASE_TARGET)-$(PLATFORM)

CROSSPLATFORM_C_SRCS = $(wildcard *.c)
CROSSPLATFORM_C_SRCS += $(wildcard can/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard interface/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard platform/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard commands/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard util/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard payload/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/cJSON/cJSON.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/AT-commander/atcommander/atcommander.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/emqueue/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/uds-c/deps/isotp-c/deps/bitfield-c/src/**/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/uds-c/deps/isotp-c/src/**/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/uds-c/src/**/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/cmp/cmp.c)
# TODO put this under a "gen" folder separate from objdir
CROSSPLATFORM_C_SRCS += $(LIBS_PATH)/openxc-message-format/libs/nanopb/pb_encode.c \
						$(LIBS_PATH)/openxc-message-format/libs/nanopb/pb_decode.c \
						$(LIBS_PATH)/openxc-message-format/libs/nanopb/pb_common.c
CROSSPLATFORM_C_SRCS += $(LIBS_PATH)/openxc-message-format/gen/cpp/openxc.pb.c
CROSSPLATFORM_CPP_SRCS = $(wildcard *.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard can/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard interface/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard platform/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard commands/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard util/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard payload/*.cpp)

INCLUDE_PATHS = -I. -I$(LIBS_PATH)/cJSON -I$(LIBS_PATH)/emqueue \
				-I$(LIBS_PATH)/AT-commander/atcommander \
				-I$(LIBS_PATH)/openxc-message-format/libs/nanopb \
				-I$(LIBS_PATH)/openxc-message-format/gen/cpp \
				-I$(LIBS_PATH)/uds-c/deps/isotp-c/deps/bitfield-c/src \
				-I$(LIBS_PATH)/uds-c/deps/isotp-c/src \
				-I$(LIBS_PATH)/uds-c/src \
				-I$(LIBS_PATH)/cmp \

CFLAGS_STD = -std=gnu99
CXXFLAGS_STD = -std=gnu++0x

include tests/tests.mk

# This must come after setting the variables like LIBS_PATH, since those are
# used in the included Makefiles
ifeq ($(PLATFORM), CHIPKIT)
include platform/pic32/pic32.mk
else ifeq ($(PLATFORM), CROSSCHASM_C5_BT)
include platform/pic32/pic32.mk
else ifeq ($(PLATFORM), CROSSCHASM_C5_CELLULAR)
# force a modified set of linkers for c5 cellular
# this is necessary to support OTA upgrades
# we only want to change the FLASH erase rules for the bootloader
# all other settings remain the same, and chipKIT-application-COMMON.ld is still used
override LINKER_SCRIPTS  += -T platform/pic32/chipKIT-application-32MX795F512_c5cell.ld
override LINKER_SCRIPTS  += -T $(ARDUINO_CORE_PATH)/chipKIT-application-COMMON.ld
include platform/pic32/pic32.mk
else ifeq ($(PLATFORM), CROSSCHASM_C5_BLE)
include platform/pic32/pic32.mk
else ifeq ($(PLATFORM), FORDBOARD)
include platform/lpc17xx/lpc17xx.mk
else ifeq ($(PLATFORM), BLUEBOARD)
include platform/lpc17xx/lpc17xx.mk
else ifneq ($(PLATFORM), TESTING)
ifdef PLATFORM
$(error "$(PLATFORM) is not a valid build platform - choose from $(VALID_PLATFORMS)")
else
$(error "There is no longer a default PLATFORM - you must \
explicitly set one with each call to `make` or `export PLATFORM=X` \
before running any comands - - choose from $(VALID_PLATFORMS)")
endif
endif

define show_options
	$(call show_vi_config_variable,PLATFORM)
	$(call show_vi_config_variable,BOOTLOADER)
       $(call show_vi_config_variable,ENVIRONMENT_MODE)
	$(call show_vi_config_variable,TEST_MODE_ONLY)
	$(call show_vi_config_variable,DEBUG)
	$(call show_vi_config_variable,MSD_ENABLE)
	$(call show_vi_config_variable,DEFAULT_FILE_GENERATE_SECS)
	$(call show_vi_config_variable,DEFAULT_METRICS_STATUS)
	$(call show_vi_config_variable,DEFAULT_ALLOW_RAW_WRITE_USB)
	$(call show_vi_config_variable,DEFAULT_ALLOW_RAW_WRITE_UART)
	$(call show_vi_config_variable,DEFAULT_ALLOW_RAW_WRITE_NETWORK)
	$(call show_vi_config_variable,DEFAULT_ALLOW_RAW_WRITE_BLE)
	$(call show_vi_config_variable,DEFAULT_LOGGING_OUTPUT)
	$(call show_vi_config_variable,DEFAULT_OUTPUT_FORMAT)
	$(call show_vi_config_variable,DEFAULT_EMULATED_DATA_STATUS)
	$(call show_vi_config_variable,DEFAULT_POWER_MANAGEMENT)
	$(call show_vi_config_variable,DEFAULT_USB_PRODUCT_ID)
	$(call show_vi_config_variable,DEFAULT_CAN_ACK_STATUS)
	$(call show_vi_config_variable,DEFAULT_OBD2_BUS)
	$(call show_vi_config_variable,DEFAULT_RECURRING_OBD2_REQUESTS_STATUS)
	$(call show_separator)
endef


.DEFAULT_GOAL = custom_all
.PHONY: force

.firmware_options: force
	@echo '$(SYMBOLS)' | cmp -s - $@ || echo '$(SYMBOLS)' > $@

display_options:
	$(info Compiling with options:)
	$(call show_options)

custom_all: display_options all
	@echo -n "$(GREEN)Compiled successfully for $(PLATFORM)"
	@if [[ "$(BOOTLOADER)" == "1" ]]; then \
		echo -n " running under a bootloader"; \
	else \
		echo -n " running on bare metal"; \
	fi
	@echo ".$(COLOR_RESET)"
	$(info Compiled with options:)
	$(call show_options)

clean::
	rm -rf $(TEST_OBJDIR)
