SYMBOLS =

GREEN="$${txtbld}$$(tput setaf 2)"
COLOR_RESET=$$(tput sgr0)

BASE_TARGET = cantranslator
OBJDIR_ROOT = build
DEPENDENCIES_FOLDER = ../dependencies
OBJDIR = $(OBJDIR_ROOT)

ifdef DEBUG
SYMBOLS += __DEBUG__
else
SYMBOLS += NDEBUG
endif

ifdef UART
SYMBOLS += __USE_UART__
endif

ifdef ETHERNET
SYMBOLS += __USE_ETHERNET__
endif

CC_SYMBOLS = $(addprefix -D,$(SYMBOLS))

CJSON_EXISTS = $(shell test -e libs/cJSON/cJSON.h; echo $$?)
ifneq ($(CJSON_EXISTS),0)
$(error cJSON dependency is missing - run "script/bootstrap.sh")
endif

# Valid PLATFORM options:
# 	CHIPKIT
# 	BLUEBOARD
# 	FORD
ifndef PLATFORM
PLATFORM = CHIPKIT
endif

ifeq ($(PLATFORM), CHIPKIT)
include pic32/pic32.mk
else
include lpc17xx/lpc17xx.mk
endif

include tests/tests.mk

transmitter: SYMBOLS += TRANSMITTER
transmitter: custom_all

emulator: SYMBOLS += CAN_EMULATOR
emulator: BASE_TARGET = canemulator
emulator: custom_all

.DEFAULT_GOAL = custom_all

custom_all_prefix:
	@echo "Compiling..."

custom_all: custom_all_prefix all
	@echo "$(GREEN)Compiled successfully.$(COLOR_RESET)"

clean::
	rm -rf $(OBJDIR_ROOT)/*
