SYMBOLS =

BASE_TARGET = cantranslator
OBJDIR_ROOT = build
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
$(error cJSON dependency is missing - did you run "git submodule init && git submodule update"?)
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

emulator: SYMBOLS += CAN_EMULATOR
emulator: BASE_TARGET = canemulator
emulator: all

transmitter: SYMBOLS += TRANSMITTER
transmitter: all

clean::
	rm -rf $(OBJDIR_ROOT)/*
