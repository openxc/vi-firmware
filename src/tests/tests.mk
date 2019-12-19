# When running test cases on the development computer, don't compile with
# embedded libraries
#
OSTYPE := $(shell uname)

TEST_DIR = tests
TEST_OBJDIR = build/$(TEST_DIR)

TEST_SRC=$(wildcard $(TEST_DIR)/*_tests.cpp)
TESTS=$(patsubst %.cpp,$(TEST_OBJDIR)/%.bin,$(TEST_SRC))
TEST_LIBS = -lcheck -lrt -lpthread -lsubunit

NON_TESTABLE_SRCS = signals.cpp main.cpp hardware_tests_main.cpp

TEST_C_SRCS = $(CROSSPLATFORM_C_SRCS) $(wildcard tests/platform/*.c) \
			  $(LIBS_PATH)/openxc-message-format/libs/nanopb/pb_decode.c
TEST_CPP_SRCS = $(wildcard tests/platform/*.cpp) $(CROSSPLATFORM_CPP_SRCS)
TEST_CPP_SRCS := $(filter-out $(NON_TESTABLE_SRCS),$(TEST_CPP_SRCS))

TEST_OBJ_FILES = $(TEST_C_SRCS:.c=.o) $(TEST_CPP_SRCS:.cpp=.o)
TEST_OBJS = $(patsubst %,$(TEST_OBJDIR)/%,$(TEST_OBJ_FILES))

GENERATOR = openxc-generate-firmware-code -s ../examples
EXAMPLE_CONFIG_DIR = ../examples
.PRECIOUS: $(TEST_OBJS) $(TESTS:.bin=.o)

define COMPILE_TEST_TEMPLATE
$1: $3
	@echo -n "$$(YELLOW)Compiling $1...$$(COLOR_RESET)"
	@$2 make clean > /dev/null
	@$2 make -j4 $4 > /dev/null 2>&1
	@echo "$$(GREEN)passed.$$(COLOR_RESET)"
endef

PLATFORMS = FORDBOARD BLUEBOARD CHIPKIT CROSSCHASM_C5_BT CROSSCHASM_C5_CELLULAR CROSSCHASM_C5_BLE
#PLATFORMS = FORDBOARD CROSSCHASM_C5_BT
PLATFORMS_WITH_MSD = CROSSCHASM_C5_BT CROSSCHASM_C5_CELLULAR
#PLATFORMS_WITH_MSD = CROSSCHASM_C5_BT

define ALL_PLATFORMS_TEST_TEMPLATE
$(foreach platform, $(PLATFORMS), \
	$(eval $(call COMPILE_TEST_TEMPLATE, $(1)-$(platform)-bootloader,$(2) BOOTLOADER=1 PLATFORM=$(platform), $(3))) \
)
$(foreach platform, $(PLATFORMS), \
	$(eval $(call COMPILE_TEST_TEMPLATE, $(1)-$(platform),$(2) BOOTLOADER=0 PLATFORM=$(platform), $(3))) \
)
$1: $(foreach platform, $(PLATFORMS), $1-$(platform)) $(foreach platform, $(PLATFORMS), $1-$(platform)-bootloader)
endef

#separate from above, run MSD_ENABLE=1 tests
define MSD_PLATFORMS_TEST_TEMPLATE
$(foreach platform, $(PLATFORMS_WITH_MSD), \
	$(eval $(call COMPILE_TEST_TEMPLATE, $(1)-$(platform)-bootloader,$(2) MSD_ENABLE=1 BOOTLOADER=1 PLATFORM=$(platform), $(3))) \
)
$(foreach platform, $(PLATFORMS_WITH_MSD), \
	$(eval $(call COMPILE_TEST_TEMPLATE, $(1)-$(platform),$(2) MSD_ENABLE=1 BOOTLOADER=0 PLATFORM=$(platform), $(3))) \
)
$1: $(foreach platform, $(PLATFORMS_WITH_MSD), $1-$(platform)) $(foreach platform, $(PLATFORMS_WITH_MSD), $1-$(platform)-bootloader)
endef

test_long: test_short
	# TODO see https://github.com/openxc/vi-firmware/issues/189
	# @make network_compile_test
	# @make network_raw_write_compile_test
	@make usb_raw_write_compile_test
	@make bluetooth_raw_write_compile_test
	@make binary_output_compile_test
	@make messagepack_output_compile_test
	@make emulator_compile_test
	@make msd_emulator_compile_test
	@make stats_compile_test
	@make msd_stats_compile_test
	@make debug_stats_compile_test
	@make msd_mapped_compile_test
	@make msd_passthrough_compile_test
	@make msd_diag_compile_test
	@echo "$(GREEN)All tests passed.$(COLOR_RESET)"

test_short: unit_tests
	@make default_compile_test
	@make msd_default_compile_test
	@make debug_compile_test
	@make mapped_compile_test
	@make passthrough_compile_test
	@make diag_compile_test
test: test_short
	@echo "$(GREEN)All tests passed.$(COLOR_RESET)"

# clang provides many more nice warnings than GCC, so we use it on both Linux
# and OS X
TEST_LD = clang++
TEST_CC = clang
TEST_CXX = clang++

# Guard against \r\n line endings only in Cygwin
ifneq ($(OSTYPE),Darwin)
	OSTYPE := $(shell uname -o)
	ifeq ($(OSTYPE),Cygwin)
		TEST_SET_OPTS = igncr
	endif
endif

CC_SUPRESSED_ERRORS = -Wno-write-strings -Wno-gnu-designator
CXX_SUPRESSED_ERRORS = $(CC_SUPRESSED_ERRORS) -Wno-conversion-null

unit_tests: LD = $(TEST_LD)
unit_tests: CC = $(TEST_CC)
unit_tests: CXX = $(TEST_CXX)
unit_tests: CPPFLAGS = -I/usr/local -c -Wall -Werror -g -ggdb -coverage
unit_tests: CFLAGS = $(CC_SUPRESSED_ERRORS) $(CFLAGS_STD)
unit_tests: CXXFLAGS =  $(CXX_SUPRESSED_ERRORS) $(CXXFLAGS_STD)
unit_tests: LDFLAGS = -lm -coverage
unit_tests: LDLIBS = $(TEST_LIBS)
unit_tests: INCLUDE_PATHS += -I./tests/platform/
unit_tests: $(TESTS)
	@set -o $(TEST_SET_OPTS) >/dev/null 2>&1
	@export SHELLOPTS
	@sh tests/runtests.sh $(TEST_OBJDIR)/$(TEST_DIR)

$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, default_compile_test, DEBUG=0, code_generation_test))
$(eval $(call MSD_PLATFORMS_TEST_TEMPLATE, msd_default_compile_test, DEBUG=0 MSD_ENABLE=1, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, diag_compile_test, DEBUG=0, diagnostic_code_generation_test))
$(eval $(call MSD_PLATFORMS_TEST_TEMPLATE, msd_diag_compile_test, DEBUG=0 MSD_ENABLE=1, diagnostic_code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, debug_compile_test, DEBUG=1, code_generation_test))
#don't do MSD_ENALBE w/ DEBUG
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, mapped_compile_test, DEBUG=0, mapped_code_generation_test))
$(eval $(call MSD_PLATFORMS_TEST_TEMPLATE, msd_mapped_compile_test, DEBUG=0 MSD_ENABLE=1, mapped_code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, passthrough_compile_test, DEBUG=0, copy_passthrough_signals))
$(eval $(call MSD_PLATFORMS_TEST_TEMPLATE, msd_passthrough_compile_test, DEBUG=0 MSD_ENABLE=1, copy_passthrough_signals))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, emulator_compile_test, DEBUG=0 DEFAULT_EMULATED_DATA_STATUS=1, )) #empty emulator
$(eval $(call MSD_PLATFORMS_TEST_TEMPLATE, msd_emulator_compile_test, DEBUG=0 DEFAULT_EMULATED_DATA_STATUS=1 MSD_ENABLE=1, )) #empty emulator
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, stats_compile_test, DEFAULT_METRICS_STATUS=1 DEBUG=0, code_generation_test))
$(eval $(call MSD_PLATFORMS_TEST_TEMPLATE, msd_stats_compile_test, DEFAULT_METRICS_STATUS=1 DEBUG=0 MSD_ENABLE=1, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, debug_stats_compile_test, DEBUG=1 DEFAULT_METRICS_STATUS=1, code_generation_test))
#no more MSD below here - can add later
# TODO see https://github.com/openxc/vi-firmware/issues/189
#$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, network_compile_test, NETWORK=1, code_generation_test))
#$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, network_raw_write_compile_test, DEFAULT_ALLOW_RAW_WRITE_NETWORK=1, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, usb_raw_write_compile_test, DEBUG=0 DEFAULT_ALLOW_RAW_WRITE_USB=0, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, bluetooth_raw_write_compile_test, DEBUG=0 DEFAULT_ALLOW_RAW_WRITE_UART=1, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, binary_output_compile_test, DEBUG=0 DEFAULT_OUTPUT_FORMAT=PROTOBUF, code_generation_test))
$(eval $(call ALL_PLATFORMS_TEST_TEMPLATE, messagepack_output_compile_test, DEBUG=0 DEFAULT_OUTPUT_FORMAT=MESSAGEPACK, code_generation_test))

copy_passthrough_signals:
	@echo "Testing example passthrough config in repo for FORDBOARD..."
	$(GENERATOR) -m $(EXAMPLE_CONFIG_DIR)/passthrough.json > signals.cpp

code_generation_test:
	@make clean
	@echo "Testing code generation with a basic VI config..."
	$(GENERATOR) -m $(EXAMPLE_CONFIG_DIR)/signals.json > signals.cpp

mapped_code_generation_test:
	@make clean
	@echo "Testing code generation with a VI config with mapped signals..."
	$(GENERATOR) -m $(EXAMPLE_CONFIG_DIR)/mapped_signal_set.json > signals.cpp

diagnostic_code_generation_test:
	@make clean
	@echo "Testing code generation with a VI config with diagnostic requests..."
	$(GENERATOR) -m $(EXAMPLE_CONFIG_DIR)/diagnostic.json > signals.cpp

COVERAGE_INFO_FILENAME = coverage.info
COVERAGE_INFO = $(TEST_OBJDIR)/$(COVERAGE_INFO_FILENAME)
COVERAGE_REPORT_HTML = $(TEST_OBJDIR)/coverage/index.html
COBERTURA_COVERAGE = $(TEST_OBJDIR)/coverage.xml
DIFFCOVER_REPORT = $(TEST_OBJDIR)/diffcover.html
MINIMUM_DIFFCOVER_PERCENTAGE = 80

$(COVERAGE_INFO): clean unit_tests
	lcov --gcov-tool llvm-cov --base-directory . --directory . -c -o $@
	lcov --remove $@ "*tests*" --remove $@ "*libs*" --remove $@ "/usr/*" -o $@

$(COVERAGE_REPORT_HTML): $(COVERAGE_INFO)
	genhtml -o $(TEST_OBJDIR)/coverage -t "vi-firmware test coverage" --num-spaces 4 $<

$(COBERTURA_COVERAGE): $(COVERAGE_INFO)
	python ../script/lcov_cobertura.py $< --output $@

$(DIFFCOVER_REPORT): $(COBERTURA_COVERAGE)
	diff-cover $< --compare-branch=origin/next --html-report $@ --fail-under=$(MINIMUM_DIFFCOVER_PERCENTAGE)

diffcover_test: $(DIFFCOVER_REPORT)

coverage: $(COVERAGE_REPORT_HTML)
	@xdg-open $<

diffcover: $(DIFFCOVER_REPORT)
	@xdg-open $<

$(TEST_OBJDIR)/%.o: %.cpp .firmware_options
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CC_SYMBOLS) $(CXXFLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.o: %.c .firmware_options
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CC_SYMBOLS) $(CFLAGS) $(INCLUDE_PATHS) -o $@ $<

$(TEST_OBJDIR)/%.bin: $(TEST_OBJDIR)/%.o $(TEST_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(CC_SYMBOLS) $(CXXFLAGS) $(INCLUDE_PATHS) -o $@ $^ $(LDLIBS)

cppclean:
	cppclean $(INCLUDE_PATHS) --exclude libs --exclude tests .  | grep -v "declared but not defined" | grep -v static
