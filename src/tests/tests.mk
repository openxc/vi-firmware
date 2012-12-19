# When running test cases on the development computer, don't compile with
# embedded libraries

TEST_DIR = tests
TEST_OBJDIR = build/$(TEST_DIR)

TEST_SRC=$(wildcard $(TEST_DIR)/*_tests.cpp)
TESTS=$(patsubst %.cpp,$(TEST_OBJDIR)/%.bin,$(TEST_SRC))
TEST_LIBS = -lcheck
INCLUDE_PATHS += -I. -I./libs/cJSON

TESTABLE_OBJ_FILES = bitfield.o queue.o canutil.o canwrite.o canread.o \
				listener.o libs/cJSON/cJSON.o
TESTABLE_LIB_SRCS = helpers.c usbutil_mock.c serialutil_mock.c \
				canwrite_mock.c log_mock.c ethernetutil_mock.c
TESTABLE_LIB_OBJ_FILES = $(addprefix $(TEST_OBJDIR)/$(TEST_DIR)/, $(TESTABLE_LIB_SRCS:.c=.o))
TESTABLE_OBJS = $(patsubst %,$(TEST_OBJDIR)/%,$(TESTABLE_OBJ_FILES)) \
				$(TESTABLE_LIB_OBJ_FILES)

.PRECIOUS: $(TESTABLE_OBJS) $(TESTS:.bin=.o)

test: LD = g++
test: CC = gcc
test: CPP = g++
test: CC_FLAGS = -I. -c -m32 -w -Wall -Werror -g -ggdb
test: CC_SYMBOLS = -D__TESTS__
test: LDFLAGS = -m32 -lm
test: LDLIBS = $(TEST_LIBS)
test: $(TESTS)
	@sh tests/runtests.sh $(TEST_OBJDIR)/$(TEST_DIR)
	@make code_generation_test

code_generation_test:
	@make clean
	@mkdir -p $(TEST_OBJDIR)
	../generate_code.py --json signals.json.example > $(TEST_OBJDIR)/signals.cpp
	@if [[ -h signals.cpp ]]; then mv -f signals.cpp signals.cpp.bak; fi
	@if [[ -h handlers.cpp ]]; then mv -f handlers.cpp handlers.cpp.bak; fi
	@if [[ -h handlers.h ]]; then mv -f handlers.h handlers.h.bak; fi
	@ln -s $(TEST_OBJDIR)/signals.cpp
	@ln -s handlers.cpp.example handlers.cpp
	@ln -s handlers.h.example handlers.h
	make -j4 && make clean
	@mv signals.cpp.bak signals.cpp
	@mv handlers.cpp.bak handlers.cpp
	@mv handlers.h.bak handlers.h

$(TEST_OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_C_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.bin: $(TEST_OBJDIR)/%.o $(TESTABLE_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(LDLIBS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $^
