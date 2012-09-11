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
include pic32.mk
else
include lpc1768.mk
endif

clean:
	rm -rf $(OBJDIR) build-cli

# For running test cases on the development computer, don't compile with
# embedded libraries
TEST_OBJDIR = $(OBJDIR)/tests

TEST_SRC=$(wildcard tests/*_tests.cpp)
TESTS=$(patsubst %.cpp,$(OBJDIR)/%.bin,$(TEST_SRC))
TEST_LIBS = -lcheck

TESTABLE_OBJ_FILES = canutil.o canwrite.o canread.o bitfield.o queue.o \
					 log.o listener.o
TESTABLE_OBJS = $(addprefix $(OBJDIR)/,$(TESTABLE_OBJ_FILES)) \
				$(OBJDIR)/libs/cJSON.o $(TEST_OBJDIR)/helpers.o \
				$(TEST_OBJDIR)/usbutil_mock.o $(TEST_OBJDIR)/serialutil_mock.o \
				$(TEST_OBJDIR)/canwrite_mock.o

test: LD = g++
test: CC = g++
test: CPP = g++
test: CC_FLAGS = -I. -c -std=gnu99 -m32 -w -Wall -g -ggdb
test: CC_SYMBOLS = -D__TESTS__
test: LDFLAGS = -m32 -lm
test: LDLIBS = $(TEST_LIBS)
test: $(TEST_OBJDIR) $(TESTS)
	@sh tests/runtests.sh $(TEST_OBJDIR)

$(TEST_OBJDIR):
	@mkdir -p $(TEST_OBJDIR)

$(TEST_OBJDIR)/%.o: tests/%.cpp
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.bin: $(TEST_OBJDIR)/%.o $(TESTABLE_OBJS)
	$(LD) $(LDFLAGS) $(LDLIBS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $^

flash: upload
