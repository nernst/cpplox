default: debug 

.BLDROOT. := $(shell pwd)

USE_UBSAN := 1
# Note: current implementation leaks on closures, so disable asan for now.
USE_ASAN := 1

SHELL := bash
CC := g++-11
LD := $(CC)

FMT_VERSION := 8.1.1

SOURCE_DIR := src
SOURCES := $(wildcard $(SOURCE_DIR)/*.cpp)

TEST_SOURCE_DIR := tests
TEST_SOURCES := $(wildcard $(TEST_SOURCE_DIR)/*.cpp)

BOOST := $(HOME)/tp/boost/1.81.0
INCLUDES :=
SYSINCLUDES:= $(BOOST)/include external/fmt-$(FMT_VERSION)/include

START_GROUP := 
END_GROUP := 

LIBS :=

ifeq ($(USE_UBSAN)$(USE_ASAN),11) 
	SANITIZERS := -fsanitize=undefined,address
	LIBS += asan ubsan
else ifeq ($(USE_UBSAN),1)
	SANITIZERS := -fsanitize=undefined
	LIBS += ubsan
else ifeq ($(USE_ASAN),1)
	SANITIZERS := -fsantizie=address
	LIBS == asan
endif 

LIBDIRS := /usr/lib/gcc/x86_64-linux-gnu/11/

deps:
	mkdir -p tmp
	wget -O tmp/fmt-$(FMT_VERSION).tar.gz https://github.com/fmtlib/fmt/archive/refs/tags/$(FMT_VERSION).tar.gz
	
	mkdir -p external
	tar -xzf tmp/fmt-$(FMT_VERSION).tar.gz --directory external/
	ln -s $(.BLDROOT.)/external/fmt-$(FMT_VERSION)/src/format.cc src/format.cpp
	rm -rf tmp

deps_clean:
	-unlink src/format.cpp
	-rm -rf external/

external/fmt-$(FMT_VERSION)/include/%.h: src/format.cpp


ifeq ($(OS),Windows_NT)
else
	UNAME_S := $(shell uname -s)
# overrides for MacOS
	ifeq ($(UNAME_S),Darwin)
		SYSINCLUDES += /usr/local/include
		START_GROUP :=
		END_GROUP :=
		LD := $(CC) -v
		LIBS :=
		LIBDIRS += /usr/local/lib
		SYSINCLUDES += 
	endif
endif

INCDIRS := $(addprefix -I,$(INCLUDES)) $(addprefix -isystem,$(SYSINCLUDES))

WARNINGS := -Wall -Wextra -Werror -Wpessimizing-move -Wredundant-move -Wold-style-cast -Woverloaded-virtual

# compiler/linker flags
CXXFLAGS := -std=c++2a -Iinc $(WARNINGS) -MMD -MP $(INCDIRS) -fexceptions $(SANITIZERS) $(EXTRA_CXXFLAGS)

# NOTE: add -DLOX_ENV_TRACE to facilitate env debugging
EXTRA_DBG_FLAGS :=  \
	-DDEBUG_COMPILE \
	-DDEBUG_PRINT_CODE \
	-DDEBUG_TRACE_EXECUTION
# -DLOX_ENV_TRACE

DBGFLAGS := -g -O0 -D_GLIBCXX_ASSERTIONS -fno-omit-frame-pointer $(EXTRA_DBG_FLAGS)
RELFLAGS := -O3
TESTFLAGS := $(DBGFLAGS) -I$(SOURCE_DIR)
LDFLAGS := $(addprefix -l,$(LIBS)) $(addprefix -L,$(LIBDIRS))

# binary targets
PROGRAM := lox
UNITTEST := unittest 

# binary directories
TST_DIR := bin/test
DBG_DIR := bin/debug
REL_DIR := bin/release

# object directories
DBG_OBJ_DIR := obj/debug
REL_OBJ_DIR := obj/release
TST_OBJ_DIR := obj/test

OUTDIRS := $(DBG_DIR) $(DBG_OBJ_DIR) $(REL_DIR) $(REL_OBJ_DIR) $(TST_DIR) $(TST_OBJ_DIR) tmp

PROG_REL := $(REL_DIR)/$(PROGRAM)
PROG_DEBUG := $(DEB_DIR)/$(PROGRAM)

OBJFILES_REL := $(patsubst $(SOURCE_DIR)/%.cpp,$(REL_OBJ_DIR)/%.o,$(SOURCES))
OBJFILES_DBG := $(patsubst $(SOURCE_DIR)/%.cpp,$(DBG_OBJ_DIR)/%.o,$(SOURCES))
OBJFILES_DBG_MAIN := $(DBG_OBJ_DIR)/lox.o
OBJFILES_TST := $(patsubst $(TEST_SOURCE_DIR)/%.cpp,$(TST_OBJ_DIR)/%.o,$(TEST_SOURCES)) $(filter-out $(OBJFILES_DBG_MAIN),$(OBJFILES_DBG))

DEPFILES_REL := $(patsubst $(SOURCE_DIR)/%.cpp,$(REL_OBJ_DIR)/%.d,$(SOURCES))
DEPFILES_DBG := $(patsubst $(SOURCE_DIR)/%.cpp,$(DBG_OBJ_DIR)/%.d,$(SOURCES))
DEPFILES_TST := $(patsubst $(TEST_SOURCE_DIR)/%.cpp,$(TST_OBJ_DIR)/%.d,$(TEST_SOURCES))
DEPFILES := $(DEPFILES_REL) $(DEPFILES_DBG) $(DEPFILES_TST) 

.PHONY: default all testmake debug release clean dirs test unittest


all:    dirs clean debug release unittest

debug:  $(DBG_DIR)/$(PROGRAM) unittest

release: $(REL_DIR)/$(PROGRAM) unittest

# NOTE: to suppress leak detection: `export ASAN_OPTIONS=detect_leaks=0`
unittest: $(TST_DIR)/$(UNITTEST)
	$(TST_DIR)/unittest

testmake:
	@echo OBJFILES_REL = [$(OBJFILES_REL)]
	@echo OBJFILES_DBG = [$(OBJFILES_DBG)]
	@echo OBJFILES_TST = [$(OBJFILES_TST)]
	@echo SOURCES      = [$(SOURCES)]
	@echo TEST_SOURCES = [$(TEST_SOURCES)]
	@echo DEPFILES_REL = [$(DEPFILES_REL)]
	@echo DEPFILES_DBG = [$(DEPFILES_DBG)]
	@echo DEPFILES_TST = [$(DEPFILES_TST)]

clean:
	rm -rf $(OUTDIRS)

$(REL_DIR)/$(PROGRAM): $(REL_DIR)%: $(REL_OBJ_DIR)%.o $(OBJFILES_REL)
	mkdir -p $(REL_DIR)
	$(LD)  $(START_GROUP) $^ $(END_GROUP) -o $@ $(START_GROUP) $(LDFLAGS) $(END_GROUP)
	@echo "----  created release binary ----"


$(DBG_DIR)/$(PROGRAM): $(DBG_DIR)%: $(DBG_OBJ_DIR)%.o $(OBJFILES_DBG)
	mkdir -p $(DBG_DIR)
	$(LD)  $(START_GROUP) $^ $(END_GROUP) -o $@ $(START_GROUP) $(LDFLAGS) $(END_GROUP)
	@echo "----  created debug binary ----"

$(TST_DIR)/$(UNITTEST): $(TST_DIR)%: $(TST_OBJ_DIR)%.o $(OBJFILES_TST)
	mkdir -p $(TST_DIR)
	$(LD)  $(START_GROUP) $^ $(END_GROUP) -o $@ $(START_GROUP) $(LDFLAGS) $(END_GROUP)
	@echo "---- created unittest binary ----"

-include $(DEPFILES)


$(REL_OBJ_DIR)/%.o: src/%.cpp
	mkdir -p $(REL_OBJ_DIR)
	$(CC) $(RELFLAGS) $(CXXFLAGS) -MF $(patsubst $(REL_OBJ_DIR)/%.o, $(REL_OBJ_DIR)/%.d,$@) -c $< -o $@

$(DBG_OBJ_DIR)/%.o: src/%.cpp
	mkdir -p $(DBG_OBJ_DIR)
	$(CC) $(DBGFLAGS) $(CXXFLAGS) -MF $(patsubst $(DBG_OBJ_DIR)/%.o, $(DBG_OBJ_DIR)/%.d,$@) -c $< -o $@

$(TST_OBJ_DIR)/%.o: tests/%.cpp
	@echo "test"
	mkdir -p $(TST_OBJ_DIR)
	$(CC) $(TESTFLAGS) $(CXXFLAGS) -MF $(patsubst $(TST_OBJ_DIR)/%.o, obj/test/%.d,$@) -c $< -o $@

test: unittest

