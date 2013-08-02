# When running test cases on the development computer, don't compile with
# embedded libraries
#
OSTYPE := $(shell uname)

TEST_DIR = tests
TEST_OBJDIR = build/$(TEST_DIR)

LIBS_PATH = libs
TEST_SRC=$(wildcard $(TEST_DIR)/*_tests.cpp)
TESTS=$(patsubst %.cpp,$(TEST_OBJDIR)/%.bin,$(TEST_SRC))
TEST_LIBS = -lcheck

NON_TESTABLE_SRCS = handlers.cpp signals.cpp main.cpp cantranslator.cpp \
		    canemulator.cpp platform/platform.cpp

TEST_C_SRCS = $(CROSSPLATFORM_C_SRCS) $(wildcard tests/platform/*.c)
TEST_CPP_SRCS = $(CROSSPLATFORM_CPP_SRCS) $(wildcard tests/platform/*.cpp)
TEST_CPP_SRCS := $(filter-out $(NON_TESTABLE_SRCS),$(TEST_CPP_SRCS))

TEST_OBJ_FILES = $(TEST_C_SRCS:.c=.o) $(TEST_CPP_SRCS:.cpp=.o)
TEST_OBJS = $(patsubst %,$(TEST_OBJDIR)/%,$(TEST_OBJ_FILES))

GENERATOR = openxc-generate-firmware-code
.PRECIOUS: $(TEST_OBJS) $(TESTS:.bin=.o)

test: unit_tests
	@make default_pic32_compile_test
	@make chipkit_compile_test
	@make c5_compile_test
	@make lpc17xx_compile_test
	@make ford_test
	@make emulator_test
	@make debug_compile_test
	@make network_compile_test
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
	@make clean
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

network_compile_test: code_generation_test
	@echo -n "Testing build with USE_NETWORK=1 flag..."
	@USE_NETWORK=1 make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

default_pic32_compile_test: code_generation_test
	@echo -n "Testing default platform build (chipKIT) with example vehicle signals..."
	@make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

chipkit_compile_test: code_generation_test
	@echo -n "Testing chipKIT build with example vehicle signals..."
	@PLATFORM=CHIPKIT make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

c5_compile_test: code_generation_test
	@echo -n "Testing CrossChasm C5 build with example vehicle signals..."
	@PLATFORM=CROSSCHASM_C5 make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

lpc17xx_compile_test: code_generation_test
	@echo -n "Testing Blueboard board build with example vehicle signals..."
	@PLATFORM=BLUEBOARD make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

mapped_lpc17xx_compile_test: mapped_code_generation_test
	@echo -n "Testing Blueboard board build with example mapped vehicle signals..."
	@PLATFORM=BLUEBOARD make -j4
	@make clean
	@echo "$(GREEN)passed.$(COLOR_RESET)"

ford_test:
	@echo -n "Testing Ford board build with emulator..."
	@make clean
	@PLATFORM=FORDBOARD make -j4 emulator
	@echo "$(GREEN)passed.$(COLOR_RESET)"

example_signals_test:
	@echo -n "Testing example signals definitions in repo..."
	@make clean
	@cp signals.cpp.example signals.cpp
	@PLATFORM=FORDBOARD make -j4
	@echo "$(GREEN)passed.$(COLOR_RESET)"

code_generation_test:
	@make clean
	@echo -n "Testing code generation from a non-mapped signals file..."
	$(GENERATOR) -m signals.json.example > signals.cpp

mapped_code_generation_test:
	@make clean
	@echo -n "Testing code generation from a mapped signals file..."
	$(GENERATOR) -m mapped_signal_set.json.example > signals.cpp

COVERAGE_INFO_FILENAME = coverage.info
COVERAGE_INFO_PATH = $(TEST_OBJDIR)/$(COVERAGE_INFO_FILENAME)
coverage:
	@lcov --base-directory . --directory . --zerocounters -q
	@make unit_tests
	@lcov --base-directory . --directory . -c -o $(TEST_OBJDIR)/coverage.info
	@lcov --remove $(COVERAGE_INFO_PATH) "$(LIBS_PATH)/*" -o $(COVERAGE_INFO_PATH)
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

$(TEST_OBJDIR)/%.bin: $(TEST_OBJDIR)/%.o $(TEST_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $^ $(LDLIBS)
