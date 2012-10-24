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

CC_SYMBOLS = $(addprefix -D,$(SYMBOLS))

ifndef BOARD
BOARD = PIC32
endif

ifeq ($(BOARD), PIC32)
include pic32/pic32.mk
else
include lpc1768/lpc1768.mk
endif

include tests/tests.mk

clean::
	rm -rf $(OBJDIR_ROOT)/*
