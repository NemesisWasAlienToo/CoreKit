SOURCE_DIR:=Source
LIBRARY_DIR=Library
BUILD_DIR=Build
TEST_DIR=Test
SAMPLE_DIR=Sample
TARGET:=CoreKit.elf
TEST_TARGET:=Test_$(TARGET).elf

CPP_SOURCES:=$(shell find $(SOURCE_DIR) -name '*.cpp')
C_SOURCES:=$(shell find $(SOURCE_DIR) -name '*.c')
AS_SOURCES:=$(shell find $(SOURCE_DIR) -name '*.asm')

CPP_TESTS:=$(shell find $(TEST_DIR) -name '*.cpp')
C_TESTS:=$(shell find $(TEST_DIR) -name '*.c')
AS_TESTS:=$(shell find $(TEST_DIR) -name '*.asm')

CPP_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.o)))
C_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
AS_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(AS_SOURCES:.asm=.o)))

TEST_CPP_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(CPP_TESTS:.cpp=.o)))
TEST_C_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(C_TESTS:.c=.o)))
TEST_AS_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(AS_TESTS:.asm=.o)))

OBJECTS:=$(CPP_OBJECTS) $(C_OBJECTS) $(AS_OBJECTS)
TEST_OBJECTS:=$(TEST_CPP_OBJECTS) $(TEST_C_OBJECTS) $(AS_TESTS_OBJECTS)

LIBRARIES:=$(addprefix -I,$(LIBRARY_DIR))

AS:=as
AS_FLAGS:=

CC:=g++
CPP_FLAGS:=-std=c++2a -g -Wall -c $(LIBRARIES)

LINKER:=g++
LINKER_FLAGS:=-Wall -Wpedantic -pedantic -pthread -lssl -lcrypto

RUN_ARGS:=
TEST_ARGS:=

all:$(OBJECTS)
	$(LINKER) $^ -o $(BUILD_DIR)/$(TARGET) $(LINKER_FLAGS)

$(BUILD_DIR)/%.o:$(SOURCE_DIR)/%.cpp Makefile | $(BUILD_DIR)
	$(CC) $< -o $@ $(CPP_FLAGS)

$(BUILD_DIR)/%.o:$(SOURCE_DIR)/%.c Makefile | $(BUILD_DIR)
	$(CC)$< -o $@ $(CPP_FLAGS) 

$(BUILD_DIR)/%.o:$(SOURCE_DIR)/%.asm Makefile | $(BUILD_DIR)
	$(AS) $< -o $@ $(AS_FLAGS)

$(BUILD_DIR):
	mkdir $@

run:
	@$(BUILD_DIR)/$(TARGET) $(RUN_ARGS)

clean:
	rm -Rf $(BUILD_DIR)

test:$(TEST_OBJECTS)
	$(LINKER) $^ -o $(BUILD_DIR)/$(TEST_TARGET) $(LINKER_FLAGS)

$(BUILD_DIR)/%.o:$(TEST_DIR)/%.cpp Makefile | $(BUILD_DIR)
	$(CC) $(CPP_FLAGS) $< -o $@

$(BUILD_DIR)/%.o:$(TEST_DIR)/%.c Makefile | $(BUILD_DIR)
	$(CC) $(CPP_FLAGS) $< -o $@

$(BUILD_DIR)/%.o:$(TEST_DIR)/%.asm Makefile | $(BUILD_DIR)
	$(AS) $(AS_FLAGS) $< -o $@

switch:$(SAMPLE_DIR)/$(Name).cpp
	@echo "Copying $(Name).cpp to Main.cpp"
	@cat $(SAMPLE_DIR)/$(Name).cpp > $(SOURCE_DIR)/Main.cpp

sample:$(SOURCE_DIR)/Main.cpp
	@echo "Sampling to $(Name).cpp"
	@cat $(SOURCE_DIR)/Main.cpp > $(SAMPLE_DIR)/$(Name).cpp