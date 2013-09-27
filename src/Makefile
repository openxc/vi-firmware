SYMBOLS =
SHELL := /bin/bash

COLORS_AVAILABLE = $(shell command -v tput >/dev/null 2>&1; echo $$?)
ifeq ($(COLORS_AVAILABLE),0)
RED="$${txtbld}$$(tput setaf 1)"
GREEN="$${txtbld}$$(tput setaf 2)"
COLOR_RESET=$$(tput sgr0)
endif


BASE_TARGET = vi-firmware
OBJDIR_ROOT = build
DEPENDENCIES_FOLDER = ../dependencies
OBJDIR = $(OBJDIR_ROOT)

ifdef BENCHTEST
SYMBOLS += __BENCHTEST__
endif

ifdef DEBUG
SYMBOLS += __DEBUG__
else
SYMBOLS += NDEBUG
endif

ifdef LOG_STATS
SYMBOLS += __LOG_STATS__
endif

ifdef NETWORK
SYMBOLS += __USE_NETWORK__
endif

ifndef BOOTLOADER
BOOTLOADER = 1
endif

CC_SYMBOLS = $(addprefix -D,$(SYMBOLS))

CJSON_EXISTS = $(shell test -e libs/cJSON/cJSON.h; echo $$?)
ifneq ($(CJSON_EXISTS),0)
$(error cJSON dependency is missing - run "script/bootstrap.sh")
endif

VALID_PLATFORMS = CHIPKIT BLUEBOARD FORDBOARD CROSSCHASM_C5
ifndef PLATFORM
PLATFORM = CHIPKIT
endif

OBJDIR = build/$(PLATFORM)
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
CROSSPLATFORM_CPP_SRCS = $(wildcard *.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard can/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard interface/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard platform/*.cpp)
CROSSPLATFORM_CPP_SRCS += $(wildcard util/*.cpp)

INCLUDE_PATHS = -I. -I./$(LIBS_PATH)/cJSON -I./$(LIBS_PATH)/emqueue \
				-I./$(LIBS_PATH)/emlist -I./$(LIBS_PATH)/emhashmap \
				-I./$(LIBS_PATH)/AT-commander/atcommander

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

include tests/tests.mk

transmitter: SYMBOLS += TRANSMITTER
transmitter: custom_all

emulator: SYMBOLS += CAN_EMULATOR
emulator: BASE_TARGET = emulator
emulator: custom_all

.DEFAULT_GOAL = custom_all

custom_all_prefix:
	@echo "Compiling for $(PLATFORM)..."

custom_all: custom_all_prefix all
	@echo -n "$(GREEN)Compiled successfully for $(PLATFORM)"
	@if [[ $(BOOTLOADER) == 1 ]]; then \
		echo -n " running under a bootloader"; \
	else \
		echo -n " running on bare metal"; \
	fi
	@echo ".$(COLOR_RESET)"

clean::
	rm -rf $(OBJDIR_ROOT)/*
