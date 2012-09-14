TEST_DIR = tests
# For running test cases on the development computer, don't compile with
# embedded libraries
TEST_OBJDIR = $(OBJDIR)/$(TEST_DIR)

TEST_SRC=$(wildcard $(TEST_DIR)/*_tests.cpp)
TESTS=$(patsubst %.cpp,$(OBJDIR)/%.bin,$(TEST_SRC))
TEST_LIBS = -lcheck

TESTABLE_OBJ_FILES = bitfield.o queue.o canutil.o canwrite.o canread.o \
				listener.o libs/cJSON/cJSON.o
TESTABLE_LIB_OBJ_FILES = $(TEST_DIR)/helpers.o $(TEST_DIR)/usbutil_mock.o \
				$(TEST_DIR)/serialutil_mock.o \
				$(TEST_DIR)/canwrite_mock.o $(TEST_DIR)/log_mock.o
TESTABLE_OBJS = $(patsubst %,$(TEST_OBJDIR)/%,$(TESTABLE_OBJ_FILES)) \
				$(patsubst %,$(OBJDIR)/%,$(TESTABLE_LIB_OBJ_FILES))

test: LD = g++
test: CC = gcc
test: CPP = g++
test: CC_FLAGS = -I. -c -m32 -w -Wall -Werror -g -ggdb
test: CC_SYMBOLS = -D__TESTS__
test: LDFLAGS = -m32 -lm
test: LDLIBS = $(TEST_LIBS)
test: $(TESTS)
	@sh tests/runtests.sh $(TEST_OBJDIR)

$(TEST_OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_C_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.bin: $(TEST_OBJDIR)/%.o $(TESTABLE_OBJS)
	echo $(TESTABLE_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(LDLIBS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $^
