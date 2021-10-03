SOURCE_DIR:=Source
LIBRARY_DIR=Library
BUILD_DIR=Build
TEST_DIR=Test
SAMPLE_DIR=Sample
TARGET:=Socket.elf
TEST_TARGET:=Test_Socket.elf

CPP_SOURCES:=$(shell find $(SOURCE_DIR) -name '*.cpp')
C_SOURCES:=$(shell find $(SOURCE_DIR) -name '*.c')

CPP_TESTS:=$(shell find $(TEST_DIR) -name '*.cpp')
C_TESTS:=$(shell find $(TEST_DIR) -name '*.c')

CPP_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.o)))
C_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))

TEST_CPP_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(CPP_TESTS:.cpp=.o)))
TEST_C_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(C_TESTS:.c=.o)))

OBJECTS:=$(CPP_OBJECTS) $(C_OBJECTS)
TEST_OBJECTS:=$(TEST_CPP_OBJECTS) $(TEST_C_OBJECTS)

CC:=g++
CPP_FLAGS:=-g -Wall -c -I$(LIBRARY_DIR)
LINKER_FLAGS:=-pthread -Wall -Wpedantic -pedantic -lssl -lcrypto

RUN_ARGS:=
TEST_ARGS:=

all:$(OBJECTS)
	$(CC) $^ -o $(BUILD_DIR)/$(TARGET) $(LINKER_FLAGS)

$(BUILD_DIR)/%.o:$(SOURCE_DIR)/%.cpp Makefile | $(BUILD_DIR)
	$(CC) $< -o $@ $(CPP_FLAGS)

$(BUILD_DIR)/%.o:$(SOURCE_DIR)/%.c Makefile | $(BUILD_DIR)
	$(CC)$< -o $@ $(CPP_FLAGS) 

$(BUILD_DIR):
	mkdir $@

run:
	@$(BUILD_DIR)/$(TARGET) $(RUN_ARGS)

clean:
	rm -Rf $(BUILD_DIR)

test:$(TEST_OBJECTS)
	$(CC) $(LINKER_FLAGS) $^ -o  $(BUILD_DIR)/$(TEST_TARGET)

$(BUILD_DIR)/%.o:$(TEST_DIR)/%.cpp Makefile | $(BUILD_DIR)
	$(CC) $(CPP_FLAGS) $< -o $@

$(BUILD_DIR)/%.o:$(TEST_DIR)/%.c Makefile | $(BUILD_DIR)
	$(CC) $(CPP_FLAGS) $< -o $@

switch:$(SAMPLE_DIR)/$(Name).cpp
	@echo "Copying $(Name).cpp to Main.cpp"
	@cat $(SAMPLE_DIR)/$(Name).cpp > $(SOURCE_DIR)/Main.cpp

sample:$(SOURCE_DIR)/Main.cpp
	@echo "Sampling to $(Name).cpp"
	@cat $(SOURCE_DIR)/Main.cpp > $(SAMPLE_DIR)/$(Name).cpp