SHELL := bash
CC := g++-11

SOURCE_DIR := src
TEST_SOURCE_DIR := tests

BOOST := $(HOME)/boost_1_79_0

WARNINGS := -Wall -Wextra -Werror -Wpessimizing-move -Wredundant-move -Wold-style-cast -Woverloaded-virtual

# compiler/linker flags
CXXFLAGS := -std=c++2a -Iinc $(WARNINGS) -MMD -MP -isystem$(BOOST) -fsanitize=undefined,address
DBGFLAGS := -g
RELFLAGS := -O3
TESTFLAGS := $(DBGFLAGS) -I$(SOURCE_DIR)
LIBS := fmt
LDFLAGS := $(patsubst %,-l%,$(LIBS)) -fsanitize=undefined,address

# binary targets
PROGRAM := cpplox
UNITTEST := unittest 

# binary directories
TST_DIR := bin/test
DBG_DIR := bin/debug
REL_DIR := bin/release

# object directories
DBG_OBJ_DIR := obj/debug
REL_OBJ_DIR := obj/release
TST_OBJ_DIR := obj/test

OUTDIRS := $(DBG_DIR) $(DBG_OBJ_DIR) $(REL_DIR) $(REL_OBJ_DIR) $(TST_DIR) $(TST_OBJ_DIR)

PROG_REL := $(REL_DIR)/$(PROGRAM)
PROG_DEBUG := $(DEB_DIR)/$(PROGRAM)

SOURCES := $(wildcard $(SOURCE_DIR)/*.cpp)
TEST_SOURCES := $(wildcard $(TEST_SOURCE_DIR)/*.cpp)

OBJFILES_REL := $(patsubst $(SOURCE_DIR)/%.cpp,$(REL_OBJ_DIR)/%.o,$(SOURCES))
OBJFILES_DBG := $(patsubst $(SOURCE_DIR)/%.cpp,$(DBG_OBJ_DIR)/%.o,$(SOURCES))
OBJFILES_TST := $(patsubst $(TEST_SOURCE_DIR)/%.cpp,$(TST_OBJ_DIR)/%.o,$(TEST_SOURCES))

DEPFILES_REL := $(patsubst $(SOURCE_DIR)/%.cpp,$(REL_OBJ_DIR)/%.d,$(SOURCES))
DEPFILES_DBG := $(patsubst $(SOURCE_DIR)/%.cpp,$(DBG_OBJ_DIR)/%.d,$(SOURCES))
DEPFILES_TST := $(patsubst $(TEST_SOURCE_DIR)/%.cpp,$(TST_OBJ_DIR)/%.d,$(TEST_SOURCES))
DEPFILES := $(DEPFILES_REL) $(DEPFILES_DBG) $(DEPFILES_TST) 

.PHONY: default all testmake debug release clean dirs test unittest

default: debug 

all:    dirs clean debug release unittest

debug:  $(DBG_DIR)/$(PROGRAM) unittest

release: $(REL_DIR)/$(PROGRAM) unittest

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
	$(CC)  -Wl,--start-group $^ -Wl,--end-group -o $@ -Wl,--start-group $(LDFLAGS) -Wl,--end-group
	@echo "----  created release binary ----"


$(DBG_DIR)/$(PROGRAM): $(DBG_DIR)%: $(DBG_OBJ_DIR)%.o $(OBJFILES_DBG)
	mkdir -p $(DBG_DIR)
	$(CC)  -Wl,--start-group $^ -Wl,--end-group -o $@ -Wl,--start-group $(LDFLAGS) -Wl,--end-group
	@echo "----  created debug binary ----"

$(TST_DIR)/$(UNITTEST): $(TST_DIR)%: $(TST_OBJ_DIR)%.o $(OBJFILES_TST)
	mkdir -p $(TST_DIR)
	$(CC)  -Wl,--start-group $^ -Wl,--end-group -o $@ -Wl,--start-group $(LDFLAGS) -Wl,--end-group
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

