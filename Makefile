BUILD_DIR = build
TARGET = hptracing
override ARGS ?= 

CXX = g++ -std=c++1y

SRC_EXT = cpp

# Flags passed to the preprocessor.
# Set Google Test's header directory as a system directory, such that
# the compiler doesn't generate warnings in Google Test headers.
CPPFLAGS += $(shell pkg-config --libs --cflags eigen3)
override OPTFLAG ?= -O3

LDFLAGS = -framework OpenCL -framework GLUT -framework OpenGL

override CXXFLAGS += \
	-ggdb \
	-Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter -Winvalid-pch \
	-Wno-deprecated-register \
	$(CPPFLAGS) $(OPTFLAG)
override V ?= @

CXXSOURCES = $(shell find -L src -name "*.$(SRC_EXT)")
OBJS = $(addprefix $(BUILD_DIR)/,$(CXXSOURCES:.$(SRC_EXT)=.o))
DEPFILES = $(OBJS:.o=.d)

CL_SOURCES = src/hp/cl_src/kernel.cl src/hp/cl_src/types.h.cl
CL_INCS = $(CL_SOURCES:.cl=.cl.inc)

all: $(TARGET)

-include $(DEPFILES)

$(BUILD_DIR)/src/hp/trace_runner.o: $(CL_INCS)

$(BUILD_DIR)/%.o: %.$(SRC_EXT)
	@echo "[cxx] $< ..."
	@mkdir -pv $(dir $@)
	@$(CXX) $(CPPFLAGS) -MM -MT "$@" "$<"  > "$(@:.o=.d)"
	$(V)$(CXX) -c $< -o $@ $(CXXFLAGS)

%.cl.inc: %.cl
	xxd -i $< $@

$(TARGET): $(OBJS)
	@echo "Linking ..."
	$(V)$(CXX) $^ -o $@ -lpthread $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)/src $(TARGET) $(CL_INCS)

clean-full: clean
	rm -rf $(BUILD_DIR)

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

.SECONDARY: $(CL_SOURCES)

