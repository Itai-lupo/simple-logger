CC ?= clang
CXX ?= clang++

AR ?= llvm-ar

MKDIR_P ?= mkdir -p

TARGET_SO := logger.so
TARGET_A := logger.a

UTILS_INCLUDE_PATH ?=
UTILS_A ?= 
INCLUDE_DIR := ./include/ $(UTILS_INCLUDE_PATH)
SRC_DIRS := ./src/ 
TESTS_DIR := ./tests

BUILD_DIR ?= ./.build
OUTPUT_DIR ?= ./.output

PCH_PATH := ./include/log.hpp
PCH_OUT := $(BUILD_DIR)/include/log.hpp.pch

rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

turnInfoObjectFile = $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(basename $1)))

SRCS := $(foreach  dir,$(SRC_DIRS),$(call rwildcard,$(dir),*.c*)) 

OBJS :=  $(call turnInfoObjectFile,$(SRCS))  
DEPS := $(OBJS:.o=.d)

EXTRA_DEPENDENCIES := $(PCH_OUT)

INC_FLAGS := $(addprefix -I,$(INCLUDE_DIR))

CXXFLAGS += $(INC_FLAGS)  -MMD -MP -g -pthread -O2 -ggdb3 -Wall -Wextra -Werror -pedantic-errors 
CXXFLAGS +=  -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-c11-extensions 
CXXFLAGS += -fsized-deallocation -g3

CFLAGS ?= -std=c2x 
CPPFLAGS ?= -std=c++23 -Wno-c99-designator -Wno-c99-extensions -Wno-gnu-label-as-value 

LDFLAGS +=  $(UTILS_A)
LDLIBS +=  -lstdc++ -lm

getFileId = $(shell python -c "import json; files = json.load(open('$(BUILD_DIR)/include/files.json', 'r')); print([file[1] for file in files if file[0].endswith('$(1)')][0])" 2>/dev/null)

ifneq ($(MAKECMDGOALS),$(PCH_OUT))
CXXFLAGS += -DFILE_ID=$(call getFileId,$<) 
endif

DEPS := $(BUILD_DIR)/include/log.hpp.d

-include $(DEPS)

$(PCH_OUT): $(PCH_PATH)
	$(MKDIR_P) $(BUILD_DIR)/include
	$(CXX) -c $(INC_FLAGS) $(CPPFLAGS) $(CXXFLAGS) -o $(PCH_OUT) $(PCH_PATH)


$(OUTPUT_DIR)/$(TARGET_SO): $(EXTRA_DEPENDENCIES) $(OBJS) 
	$(MKDIR_P) $(OUTPUT_DIR)
	$(CC) -shared $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)


$(OUTPUT_DIR)/$(TARGET_A): $(EXTRA_DEPENDENCIES) $(OBJS) 
	$(MKDIR_P) $(OUTPUT_DIR)
	$(AR) rcs $@  $(OBJS) 

# c++ source
$(BUILD_DIR)/%.o: %.cpp
	@echo "building file: " $< " with id " $(call getFileId,$<)
	$(MKDIR_P) $(dir $@)
	$(CXX)  $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


#c source
$(BUILD_DIR)/%.o: %.c
	@echo "building file: " $< " with id " $(call getFileId,$<) 
	$(MKDIR_P) $(dir $@)
	$(CC)  $(CFLAGS) $(CXXFLAGS)  -c $< -o $@


get_srcs:
	@echo  $(SRCS)

get_include_paths:
	@echo $(INCLUDE_DIR)

get_pch_path:
	@echo $(PCH_PATH)


ifneq (./$(MAKECMDGOALS),$(PCH_OUT))
# CPPFLAGS += -include-pch $(PCH_OUT) 
endif

all: $(OUTPUT_DIR)/$(TARGET_SO) $(OUTPUT_DIR)/$(TARGET_A)

.PHONY: all clean get_srcs get_include_paths  get_pch_path
