SYMBOLS =
SHELL := /bin/bash

COLORS_AVAILABLE = $(shell command -v tput >/dev/null 2>&1; echo $$?)
ifeq ($(COLORS_AVAILABLE),0)
RED="$${txtbld}$$(tput setaf 1)"
GREEN="$${txtbld}$$(tput setaf 2)"
COLOR_RESET=$$(tput sgr0)
endif

# For message printing: pad the right side of the first argument with spaces to
# the number of bytes indicated by the second argument.
space_pad_to = $(shell echo $(1) "                                                      " | head -c$(2))

# Call with some text, and a prefix tag if desired (like [AUTODETECTED]),
show_vi_config_info = $(info - $(call space_pad_to,$(2),4) $(1))

# Call with the name of the variable, a prefix tag if desired (like [AUTODETECTED]),
# and an explanation if desired (like (found in $$PATH)
show_vi_config_variable = $(call show_vi_config_info,$(1) = $($(1)) $(3),$(2))

show_separator = $(info -------------------------)

BASE_TARGET = vi-firmware
OBJDIR_ROOT = build
DEPENDENCIES_FOLDER = ../dependencies
OBJDIR = $(OBJDIR_ROOT)

ifeq ($(BENCHTEST), 1)
SYMBOLS += __BENCHTEST__
endif

BINARY_OUTPUT ?= 0
ifeq ($(BINARY_OUTPUT), 1)
SYMBOLS += USE_BINARY_OUTPUT
endif

USB_ALLOW_RAW_WRITE ?= 1

ifeq ($(USB_ALLOW_RAW_WRITE), 1)
SYMBOLS += USB_ALLOW_RAW_WRITE
endif

BLUETOOTH_ALLOW_RAW_WRITE ?= 0
ifeq ($(BLUETOOTH_ALLOW_RAW_WRITE), 1)
SYMBOLS += BLUETOOTH_ALLOW_RAW_WRITE
endif

NETWORK_ALLOW_RAW_WRITE ?= 0
# ifeq ($(NETWORK_ALLOW_RAW_WRITE), 1)
# SYMBOLS += NETWORK_ALLOW_RAW_WRITE
# endif

DEBUG ?= 0
ifeq ($(DEBUG), 1)
SYMBOLS += __DEBUG__
else
SYMBOLS += NDEBUG
endif

UART_LOGGING ?= 0
ifeq ($(UART_LOGGING), 1)
SYMBOLS += __UART_LOGGING__
endif

LOG_STATS ?= 0
ifeq ($(LOG_STATS), 1)
SYMBOLS += __LOG_STATS__
endif

# TODO see https://github.com/openxc/vi-firmware/issues/189
# ifeq ($(NETWORK), 1)
# SYMBOLS += __USE_NETWORK__
# endif

BOOTLOADER ?= 1

SYMBOLS += PB_FIELD_16BIT

CC_SYMBOLS = $(addprefix -D,$(SYMBOLS))

CJSON_EXISTS = $(shell test -e libs/cJSON/cJSON.h; echo $$?)
ifneq ($(CJSON_EXISTS),0)
$(error cJSON dependency is missing - run "script/bootstrap.sh")
endif

VALID_PLATFORMS = CHIPKIT BLUEBOARD FORDBOARD CROSSCHASM_C5
PLATFORM ?= CHIPKIT

OBJDIR = build/$(PLATFORM)
LIBS_PATH = libs
TARGET = $(BASE_TARGET)-$(PLATFORM)

CROSSPLATFORM_C_SRCS = $(wildcard *.c)
CROSSPLATFORM_C_SRCS += $(wildcard can/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard interface/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard platform/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard util/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/cJSON/cJSON.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/AT-commander/atcommander/atcommander.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/emqueue/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/emhashmap/emlist/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/emhashmap/*.c)
CROSSPLATFORM_C_SRCS += $(wildcard $(LIBS_PATH)/bitfield-c/src/**/*.c)
# TODO put this under a "gen" folder separate from objdir
CROSSPLATFORM_C_SRCS += $(LIBS_PATH)/nanopb/pb_encode.c
CROSSPLATFORM_C_SRCS += $(LIBS_PATH)/openxc-message-format/gen/cpp/openxc.pb.c
CROSSPLATFORM_CPP_SRCS = $(wildcard *.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard can/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard interface/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard platform/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard util/*.cpp)

INCLUDE_PATHS = -I. -I$(LIBS_PATH)/cJSON -I$(LIBS_PATH)/emqueue \
				-I$(LIBS_PATH)/emlist -I$(LIBS_PATH)/emhashmap \
				-I$(LIBS_PATH)/AT-commander/atcommander \
				-I$(LIBS_PATH)/nanopb \
				-I$(LIBS_PATH)/openxc-message-format/gen/cpp \
				-I$(LIBS_PATH)/bitfield-c/src

# the PIC32 can't build with gnu99, so we have to leave it out
ONLY_C_FLAGS =
ONLY_CPP_FLAGS = -std=gnu++0x

ifeq ($(PLATFORM), CHIPKIT)
include platform/pic32/pic32.mk
else ifeq ($(PLATFORM), CROSSCHASM_C5)
include platform/pic32/pic32.mk
else ifeq ($(PLATFORM), FORDBOARD)
include platform/lpc17xx/lpc17xx.mk
else ifeq ($(PLATFORM), BLUEBOARD)
include platform/lpc17xx/lpc17xx.mk
else
$(error "$(PLATFORM) is not a valid build platform - choose from $(VALID_PLATFORMS)")
endif

define show_options
	$(call show_vi_config_variable,PLATFORM)
	$(call show_vi_config_variable,BOOTLOADER)
	$(call show_vi_config_variable,DEBUG)
	$(call show_vi_config_variable,UART_LOGGING)
	$(call show_vi_config_variable,BENCHTEST)
	$(call show_vi_config_variable,LOG_STATS)
	$(call show_vi_config_variable,BINARY_OUTPUT)
	$(call show_vi_config_variable,USB_ALLOW_RAW_WRITE)
	$(call show_vi_config_variable,BLUETOOTH_ALLOW_RAW_WRITE)
	$(call show_vi_config_variable,NETWORK_ALLOW_RAW_WRITE)
	$(call show_vi_config_variable,EMULATOR)
	$(call show_vi_config_variable,TRANSMITTER)
	$(call show_separator)
endef

include tests/tests.mk

TRANSMITTER ?= 0
transmitter: SYMBOLS += TRANSMITTER
transmitter: TRANSMITTER = 1
transmitter: custom_all

EMULATOR ?= 0
emulator: SYMBOLS += CAN_EMULATOR
emulator: EMULATOR = 1
emulator: custom_all

.DEFAULT_GOAL = custom_all

custom_all_prefix:
	$(info Compiling with options:)
	$(call show_options)

custom_all: custom_all_prefix all
	@echo -n "$(GREEN)Compiled successfully for $(PLATFORM)"
	@if [[ "$(BOOTLOADER)" == "1" ]]; then \
		echo -n " running under a bootloader"; \
	else \
		echo -n " running on bare metal"; \
	fi
	@echo ".$(COLOR_RESET)"
	$(info Compiled with options:)
	$(call show_options)
