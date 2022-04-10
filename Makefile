SHELL := bash
PROGRAMS := cpplox

SOURCE_DIR := src

DEBUG_DIR := bin/debug
DEBUG_OBJ_DIR := obj/debug

RELEASE_DIR := bin/release
RELEASE_OBJ_DIR := obj/release

OUTDIRS := $(DEBUG_DIR) $(DEBUG_OBJ_DIR) $(RELEASE_DIR) $(RELEASE_OBJ_DIR)

PROGS_REL := $(addprefix $(RELEASE_DIR)/,$(PROGRAMS))
PROGS_DEBUG := $(addprefix $(DEBUG_DIR)/,$(PROGRAMS))

SOURCES := $(wildcard $(SOURCE_DIR)/*.cpp)
PROG_SOURCES := $(addprefix $(SOURCE_DIR)/,$(addsuffix .cpp,$(PROGRAMS)))
COMMON_SOURCES := $(filter-out $(PROG_SOURCES),$(SOURCES))

OBJFILES_REL := $(patsubst $(SOURCE_DIR)/%.cpp,$(RELEASE_OBJ_DIR)/%.o,$(COMMON_SOURCES))
OBJFILES_DEBUG := $(patsubst $(SOURCE_DIR)/%.cpp,$(DEBUG_OBJ_DIR)/%.o,$(COMMON_SOURCES))

DEPFILES := $(patsubst $(SOURCE_DIR)/%.cpp,obj/%.d,$(SOURCES))

CXXFLAGS := -std=c++2a -Iinc -Wall -Wextra -Werror -MMD -MP
DBGFLAGS := -g
RELFLAGS := -O3
LIBS := fmt
LDFLAGS := $(patsubst %,-l%,$(LIBS))

CC := g++

.PHONY: default all testmake debug release clean dirs test

default: debug 

all:    dirs clean debug release

dirs: 
	@mkdir -p  $(OUTDIRS)

debug:  $(PROGS_DEBUG)

release: $(PROGS_REL)

testmake:
	@echo OBJFILES_REL = $(OBJFILES_REL)
	@echo OBJFILES_DEBUG = $(OBJFILES_DEBUG)
	@echo SRCFILES = $(SOURCES)
	@echo DEPFILES = $(DEPFILES)

clean:
	rm -f $(OBJFILES_REL) $(OBJFILES_DEBUG) $(DEPFILES) $(PROGRAMS)

$(PROGS_REL): $(RELEASE_DIR)%: $(RELEASE_OBJ_DIR)%.o $(OBJFILES_REL)
	mkdir -p $(RELEASE_DIR)
	$(CC)  -Wl,--start-group $^ -Wl,--end-group -o $@ -Wl,--start-group $(LDFLAGS) -Wl,--end-group
#	strip $(PROG_REL)
	@echo "----  created release binary ----"


$(PROGS_DEBUG): $(DEBUG_DIR)%: $(DEBUG_OBJ_DIR)%.o $(OBJFILES_DEBUG)
	mkdir -p $(DEBUG_DIR)
	$(CC)  -Wl,--start-group $^ -Wl,--end-group -o $@ -Wl,--start-group $(LDFLAGS) -Wl,--end-group
	@echo "----  created debug binary ----"

-include $(DEPFILES)

$(RELEASE_OBJ_DIR)/%.o: src/%.cpp
	mkdir -p $(RELEASE_OBJ_DIR)
	$(CC) $(RELFLAGS) $(CXXFLAGS) -MF $(patsubst $(RELEASE_OBJ_DIR)/%.o, obj/%.d,$@) -c $< -o $@

$(DEBUG_OBJ_DIR)/%.o: src/%.cpp
	mkdir -p $(DEBUG_OBJ_DIR)
	$(CC) $(DBGFLAGS) $(CXXFLAGS) -MF $(patsubst $(DEBUG_OBJ_DIR)/%.o, obj/%.d,$@) -c $< -o $@

test: debug
	./bin/debug/cpplox hello.lox

