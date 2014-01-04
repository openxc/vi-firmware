# When running test cases on the development computer, don't compile with
# embedded libraries
#
OSTYPE := $(shell uname)

TEST_DIR = tests
TEST_OBJDIR = build/$(TEST_DIR)

TEST_SRC=$(wildcard $(TEST_DIR)/*_tests.cpp)
TESTS=$(patsubst %.cpp,$(TEST_OBJDIR)/%.bin,$(TEST_SRC))
TEST_LIBS = -lcheck

NON_TESTABLE_SRCS = handlers.cpp signals.cpp main.cpp vi_firmware.cpp \
		    emulator.cpp platform/platform.cpp

TEST_C_SRCS = $(CROSSPLATFORM_C_SRCS) $(wildcard tests/platform/*.c) \
			  $(LIBS_PATH)/nanopb/pb_decode.c
TEST_CPP_SRCS = $(CROSSPLATFORM_CPP_SRCS) $(wildcard tests/platform/*.cpp)
TEST_CPP_SRCS := $(filter-out $(NON_TESTABLE_SRCS),$(TEST_CPP_SRCS))

TEST_OBJ_FILES = $(TEST_C_SRCS:.c=.o) $(TEST_CPP_SRCS:.cpp=.o)
TEST_OBJS = $(patsubst %,$(TEST_OBJDIR)/%,$(TEST_OBJ_FILES))

GENERATOR = openxc-generate-firmware-code
.PRECIOUS: $(TEST_OBJS) $(TESTS:.bin=.o)

define COMPILE_TEST_TEMPLATE
$1: $3
	@make clean > /dev/null
	$2 make -j4 $4 > /dev/null
	@echo "$$(GREEN)Passed.$$(COLOR_RESET)"
endef

PLATFORMS = FORDBOARD BLUEBOARD CHIPKIT CROSSCHASM_C5
define ALL_PLATFORMS_TEST_TEMPLATE
$(foreach platform, $(PLATFORMS), \
	$(eval $(call COMPILE_TEST_TEMPLATE, $(1)-$(platform)-bootloader,$(2) BOOTLOADER=1 PLATFORM=$(platform), $(3))) \
)
$(foreach platform, $(PLATFORMS), \
	$(eval $(call COMPILE_TEST_TEMPLATE, $(1)-$(platform),$(2) BOOTLOADER=0 PLATFORM=$(platform), $(3))) \
)
$1: $(foreach platform, $(PLATFORMS), $1-$(platform)) $(foreach platform, $(PLATFORMS), $1-$(platform)-bootloader)
endef

test: unit_tests
	@make default_compile_test
	@make debug_compile_test
	@make mapped_compile_test
	@make passthrough_compile_test
	@make emulator_compile_test
	@make stats_compile_test
	@make debug_stats_compile_test
	# TODO see https://github.com/openxc/vi-firmware/issues/189
	# @make network_compile_test
	# @make network_raw_write_compile_test
	@make usb_raw_write_compile_test
	@make bluetooth_raw_write_compile_test
	@make binary_output_compile_test
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
unit_tests: C_FLAGS = $(CC_FLAGS)
unit_tests: CC_SYMBOLS += -D__TESTS__
unit_tests: LDFLAGS = -lm -coverage
unit_tests: LDLIBS = $(TEST_LIBS)
unit_tests: $(TESTS)
	@set -o $(TEST_SET_OPTS) >/dev/null 2>&1
	@export SHELLOPTS
	@sh tests/runtests.sh $(TEST_OBJDIR)/$(TEST_DIR)

$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, default_compile_test, , code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, debug_compile_test, DEBUG=1, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, mapped_compile_test, , mapped_code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, passthrough_compile_test, , copy_passthrough_signals))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, emulator_compile_test, , , emulator))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, stats_compile_test, LOG_STATS=1, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, debug_stats_compile_test, DEBUG=1 LOG_STATS=1, code_generation_test))
# TODO see https://github.com/openxc/vi-firmware/issues/189
#$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, network_compile_test, NETWORK=1, code_generation_test))
#$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, network_raw_write_compile_test, NETWORK_ALLOW_RAW_WRITE=1, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, usb_raw_write_compile_test, USB_ALLOW_RAW_WRITE=1, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, bluetooth_raw_write_compile_test, BLUETOOTH_ALLOW_RAW_WRITE=1, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, binary_output_compile_test, BINARY_OUTPUT=1, code_generation_test))

copy_passthrough_signals:
	@echo "Testing example passthrough config in repo for FORDBOARD..."
	$(GENERATOR) -m passthrough.json > signals.cpp

code_generation_test:
	@make clean
	@echo "Testing code generation from a non-mapped signals file..."
	$(GENERATOR) -m signals.json.example > signals.cpp

mapped_code_generation_test:
	@make clean
	@echo "Testing code generation from a mapped signals file..."
	$(GENERATOR) -m mapped_signal_set.json.example > signals.cpp

COVERAGE_INFO_FILENAME = coverage.info
COVERAGE_INFO_PATH = $(TEST_OBJDIR)/$(COVERAGE_INFO_FILENAME)
coverage:
	@lcov --base-directory . --directory . --zerocounters -q
	@make unit_tests
	@lcov --base-directory . --directory . -c -o $(TEST_OBJDIR)/coverage.info
	@lcov --remove $(COVERAGE_INFO_PATH) "$(LIBS_PATH)/*" -o $(COVERAGE_INFO_PATH)
	@lcov --remove $(COVERAGE_INFO_PATH) "/usr/*" -o $(COVERAGE_INFO_PATH)
	@genhtml -o $(TEST_OBJDIR)/coverage -t "vi-firmware test coverage" --num-spaces 4 $(COVERAGE_INFO_PATH)
	@$(BROWSER) $(TEST_OBJDIR)/coverage/index.html
	@echo "$(GREEN)Coverage information generated in $(TEST_OBJDIR)/coverage/index.html.$(COLOR_RESET)"

$(TEST_OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CC_FLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) $(CC_SYMBOLS) $(ONLY_C_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.bin: $(TEST_OBJDIR)/%.o $(TEST_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(CC_SYMBOLS) $(ONLY_CPP_FLAGS) $(INCLUDE_PATHS) -o $@ $^ $(LDLIBS)
