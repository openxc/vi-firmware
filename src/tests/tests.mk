# When running test cases on the development computer, don't compile with
# embedded libraries
#
OSTYPE := $(shell uname)

TEST_DIR = tests
TEST_OBJDIR = build/$(TEST_DIR)

TEST_SRC=$(wildcard $(TEST_DIR)/*_tests.cpp)
TESTS=$(patsubst %.cpp,$(TEST_OBJDIR)/%.bin,$(TEST_SRC))
TEST_LIBS = -lcheck
INCLUDE_PATHS += -I. -I./libs/cJSON

TESTABLE_OBJ_FILES = bitfield.o queue.o canutil.o canwrite.o canread.o \
				listener.o libs/cJSON/cJSON.o buffers.o strutil.o usbutil.o \
				serialutil.o ethernetutil.o shared_handlers.o
TESTABLE_LIB_SRCS = usbutil_mock.c serialutil_mock.c \
				canwrite_mock.c log_mock.c ethernetutil_mock.c timer_mock.o
TESTABLE_LIB_OBJ_FILES = $(addprefix $(TEST_OBJDIR)/$(TEST_DIR)/, $(TESTABLE_LIB_SRCS:.c=.o))
TESTABLE_OBJS = $(patsubst %,$(TEST_OBJDIR)/%,$(TESTABLE_OBJ_FILES)) \
				$(TESTABLE_LIB_OBJ_FILES)

.PRECIOUS: $(TESTABLE_OBJS) $(TESTS:.bin=.o)

RED="$${txtbld}$$(tput setaf 1)"
GREEN="$${txtbld}$$(tput setaf 2)"
COLOR_RESET=$$(tput sgr0)

test: unit_tests
	@make pic32_compile_test
	@make lpc17xx_compile_test
	@make ford_test
	@make emulator_test
	@make uart_compile_test
	@make debug_compile_test
	@make ethernet_compile_test
	@echo "$(GREEN)All tests passed.$(COLOR_RESET)"

ifeq ($(OSTYPE),Darwin)
# gcc/g++ are the LLVM versions in OS X, which don't have coverage. must
# explicitly use clang/clang++
LLVM_BIN_FOLDER = $(DEPENDENCIES_FOLDER)/clang+llvm-3.2-x86_64-apple-darwin11/bin
TEST_CPP = $(LLVM_BIN_FOLDER)/clang++
TEST_CC = $(LLVM_BIN_FOLDER)/clang
TEST_LD = $(TEST_CPP)
else
TEST_LD = g++
TEST_CC = gcc
TEST_CPP = g++
endif

# In Linux, expect BROWSER to name the preferred browser binary
ifeq ($(OSTYPE),Darwin)
BROWSER = open
endif

# Guard against \r\n line endings only in Cygwin
ifneq ($(OSTYPE),Darwin)
	OSTYPE := $(shell uname -o)
	ifeq ($(OSTYPE),Cygwin)
		TEST_SET_OPTS = igncr
	endif
endif

unit_tests: LD = $(TEST_LD)
unit_tests: CC = $(TEST_CC)
unit_tests: CPP = $(TEST_CPP)
unit_tests: CC_FLAGS = -I. -c -w -Wall -Werror -g -ggdb -coverage
unit_tests: CC_SYMBOLS = -D__TESTS__
unit_tests: LDFLAGS = -lm -coverage
unit_tests: LDLIBS = $(TEST_LIBS)
unit_tests: $(TESTS)
	@set -o $(TEST_SET_OPTS) >/dev/null 2>&1
	@export SHELLOPTS
	@sh tests/runtests.sh $(TEST_OBJDIR)/$(TEST_DIR)

emulator_test:
	@echo -n "Testing CAN emulator build for chipKIT..."
	@make -j4 emulator
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"
	@echo -n "Testing CAN emulator build for Blueboard ARM board..."
	@PLATFORM=BLUEBOARD make -j4 emulator
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

debug_compile_test: code_generation_test
	@echo -n "Testing build with DEBUG=1 flag..."
	@DEBUG=1 make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

uart_compile_test: code_generation_test
	@echo -n "Testing build with USE_UART=1 flag..."
	@USE_UART=1 make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

ethernet_compile_test: code_generation_test
	@echo -n "Testing build with USE_ETHERNET=1 flag..."
	@USE_ETHERNET=1 make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

pic32_compile_test: code_generation_test
	@echo -n "Testing chipKIT build with example vehicle signals..."
	@make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

lpc17xx_compile_test: code_generation_test
	@echo -n "Testing Blueboard board build with example vehicle signals..."
	@PLATFORM=BLUEBOARD make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

ford_test:
	@echo -n "Testing Ford board build with example vehicle signals..."
	@PLATFORM=FORD make -j4 emulator
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

code_generation_test:
	@make clean
	@mkdir -p $(TEST_OBJDIR)
	../script/generate_code.py --json signals.json.example > $(TEST_OBJDIR)/signals.cpp
    # Ideally we would symlink these files, but symlinks don't work well in Cygwin
	@cp $(TEST_OBJDIR)/signals.cpp signals.cpp
	@rm -f handlers.h handlers.cpp
	@cp handlers.cpp.example handlers.cpp
	@cp handlers.h.example handlers.h

COVERAGE_INFO_FILENAME = coverage.info
COVERAGE_INFO_PATH = $(TEST_OBJDIR)/$(COVERAGE_INFO_FILENAME)
coverage:
	@lcov --base-directory . --directory . --zerocounters -q
	@make unit_tests
	@lcov --base-directory . --directory . -c -o $(TEST_OBJDIR)/coverage.info
	@lcov --remove $(COVERAGE_INFO_PATH) "libs/*" -o $(COVERAGE_INFO_PATH)
	@lcov --remove $(COVERAGE_INFO_PATH) "/usr/*" -o $(COVERAGE_INFO_PATH)
	@genhtml -o $(TEST_OBJDIR)/coverage -t "cantranslator test coverage" --num-spaces 4 $(COVERAGE_INFO_PATH)
	@$(BROWSER) $(TEST_OBJDIR)/coverage/index.html
	@echo "$(GREEN)Coverage information generated in $(TEST_OBJDIR)/coverage/index.html.$(COLOR_RESET)"

$(TEST_OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_C_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.bin: $(TEST_OBJDIR)/%.o $(TESTABLE_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $^ $(LDLIBS)
