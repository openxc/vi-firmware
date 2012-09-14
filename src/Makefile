SYMBOLS =

ifdef EMULATOR
SYMBOLS += CAN_EMULATOR
endif

ifdef SERIAL
SYMBOLS += SERIAL
endif

ifdef DEBUG
SYMBOLS += __DEBUG__
else
SYMBOLS += NDEBUG
endif

CC_SYMBOLS = $(addprefix -D,$(SYMBOLS))

ifdef PIC32
include pic32/pic32.mk
else
include lpc1768/lpc1768.mk
endif

include tests/tests.mk

clean:
	rm -rf $(OBJDIR) build-cli
