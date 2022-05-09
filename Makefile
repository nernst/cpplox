SHELL := bash
#ifeq ($(OS),Linux)
CC := g++-11
LD := ld

#else
#CC := g++
#endif

SOURCE_DIR := src
SOURCES := $(wildcard $(SOURCE_DIR)/*.cpp)

TEST_SOURCE_DIR := tests
TEST_SOURCES := $(wildcard $(TEST_SOURCE_DIR)/*.cpp)

BOOST := $(HOME)/boost_1_79_0
INCLUDES :=
SYSINCLUDES:= $(BOOST)

START_GROUP := -Wl,--start-group
END_GROUP := -Wl,--end-group

LIBS := fmt
LIBDIRS :=

ifeq ($(OS),Windows_NT)
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		SYSINCLUDES += /usr/local/include
		START_GROUP :=
		END_GROUP :=
		LD := $(CC) -v
		LIBS :=
		LIBDIRS += /usr/local/lib
		SYSINCLUDES += external/fmt-8.1.1/include
		EXTRA_CXXFLAGS += -D_GLIBCXX_USE_CXX11_ABI=0
	endif
endif

INCDIRS := $(addprefix -I,$(INCLUDES)) $(addprefix -isystem,$(SYSINCLUDES))

WARNINGS := -Wall -Wextra -Werror -Wpessimizing-move -Wredundant-move -Wold-style-cast -Woverloaded-virtual

# compiler/linker flags
CXXFLAGS := -std=c++2a -Iinc $(WARNINGS) -MMD -MP $(INCDIRS) -fsanitize=undefined,address $(EXTRA_CXXFLAGS)
DBGFLAGS := -g -O0
RELFLAGS := -O3
TESTFLAGS := $(DBGFLAGS) -I$(SOURCE_DIR)
LDFLAGS := $(addprefix -l,$(LIBS)) $(addprefix -L,$(LIBDIRS)) -fsanitize=undefined,address

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
	$(LD)  $(START_GROUP) $^ $(END_GROUP) -o $@ $(START_GROUP) $(LDFLAGS) $(END_GROUP)
	@echo "----  created release binary ----"


$(DBG_DIR)/$(PROGRAM): $(DBG_DIR)%: $(DBG_OBJ_DIR)%.o $(OBJFILES_DBG)
	mkdir -p $(DBG_DIR)
	$(LD)  $(START_GROUP) $^ $(END_GROUP) -o $@ $(START_GROUP) $(LDFLAGS) $(END_GROUP)
	@echo "----  created debug binary ----"

$(TST_DIR)/$(UNITTEST): $(TST_DIR)%: $(TST_OBJ_DIR)%.o $(OBJFILES_TST) $(DBG_OBJ_DIR)/format.o
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

