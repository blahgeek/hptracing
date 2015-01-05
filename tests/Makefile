# Points to the root of Google Test, relative to where this file is.
# Remember to tweak this if you move this file.
GTEST_DIR = ./gtest-1.7.0
# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

BUILD_DIR = build
TARGET = hptest
override ARGS ?= 

CXX = g++ -std=c++1y

SRC_EXT = cpp

# Flags passed to the preprocessor.
# Set Google Test's header directory as a system directory, such that
# the compiler doesn't generate warnings in Google Test headers.
CPPFLAGS = -isystem $(GTEST_DIR)/include
override OPTFLAG ?= -O2

override CXXFLAGS += \
	-ggdb \
	-Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter -Winvalid-pch \
	-Wno-deprecated-register \
	$(CPPFLAGS) $(OPTFLAG)
override V ?= @

CXXSOURCES = $(shell find -L src -name "*.$(SRC_EXT)")
OBJS = $(addprefix $(BUILD_DIR)/,$(CXXSOURCES:.$(SRC_EXT)=.o))
DEPFILES = $(OBJS:.o=.d)


all: $(TARGET)

-include $(DEPFILES)

# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
$(BUILD_DIR)/gtest-all.o: $(GTEST_SRCS_)
	@echo "[cxx] gtest-all"
	$(V)$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc -o $@

$(BUILD_DIR)/gtest_main.o: $(GTEST_SRCS_)
	@echo "[cxx] gtest-main"
	$(V)$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc -o $@

$(BUILD_DIR)/gtest.a: $(BUILD_DIR)/gtest-all.o
	@echo "[ar] gtest"
	$(V)$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/gtest_main.a: $(BUILD_DIR)/gtest-all.o $(BUILD_DIR)/gtest_main.o
	@echo "[ar] gtest_main"
	$(V)$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/%.o: %.$(SRC_EXT) $(GTEST_HEADERS)
	@echo "[cxx] $< ..."
	@mkdir -pv $(dir $@)
	@$(CXX) $(CPPFLAGS) -MM -MT "$@" "$<"  > "$(@:.o=.d)"
	$(V)$(CXX) -c $< -o $@ $(CXXFLAGS)

$(TARGET): $(OBJS) $(BUILD_DIR)/gtest_main.a
	@echo "Linking ..."
	$(V)$(CXX) $^ -o $@ -lpthread $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)/src $(TARGET)

clean-full:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET) $(ARGS) 2>data/test_log

gdb: 
	OPTFLAG=-O0 make -j4
	gdb --args $(TARGET) $(ARGS)

git:
	git add -A
	git commit -a

gprof:
	OPTFLAG='-O3 -pg' LDFLAGS=-pg make -j4

.SUFFIXES:

.PHONY: all clean clean-full run gdb git gprof parser scanner

