SYMBOLS =

BASE_TARGET = cantranslator
OBJDIR_ROOT = build
OBJDIR = $(OBJDIR_ROOT)

ifdef EMULATOR
SYMBOLS += CAN_EMULATOR
BASE_TARGET = canemulator
endif

ifdef DEBUG
SYMBOLS += __DEBUG__
else
SYMBOLS += NDEBUG
endif

ifdef NO_UART
SYMBOLS += NO_UART
endif

ifdef TRANSMITTER
SYMBOLS += TRANSMITTER
endif

CC_SYMBOLS = $(addprefix -D,$(SYMBOLS))

CJSON_EXISTS = $(shell test -e libs/cJSON/cJSON.h; echo $$?)
ifneq ($(CJSON_EXISTS),0)
$(error cJSON dependency is missing - did you run "git submodule init && git submodule update"?)
endif

# Valid BOARD options:
# 	CHIPKIT
# 	BLUEBOARD
# 	FORD
ifndef BOARD
BOARD = CHIPKIT
endif

ifeq ($(BOARD), CHIPKIT)
include pic32/pic32.mk
else
include lpc17xx/lpc17xx.mk
endif

include tests/tests.mk

clean::
	rm -rf $(OBJDIR_ROOT)/*
