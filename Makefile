SOURCE_DIR:=Source
LIBRARY_DIR=Library
BUILD_DIR=Build
SAMPLE_DIR=Sample
TARGET:=Socket.elf

CPP_SOURCES:=$(shell find $(SOURCE_DIR) -name '*.cpp')
C_SOURCES:=$(shell find $(SOURCE_DIR) -name '*.c')

CPP_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.o)))
C_OBJECTS:=$(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))

OBJECTS:=$(CPP_OBJECTS) $(C_OBJECTS)

CC:=g++
CPP_FLAGS:=-g -Wall -c -I$(LIBRARY_DIR)
LINKER_FLAGS:=-pthread -Wall

RUN_ARGS:=

all:$(OBJECTS)
	@echo $(OBJECTS)
	$(CC) $(LINKER_FLAGS) $^ -o  $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/%.o:$(SOURCE_DIR)/%.cpp Makefile | $(BUILD_DIR)
	$(CC) $(CPP_FLAGS) $< -o $@

$(BUILD_DIR)/%.o:$(SOURCE_DIR)/%.c Makefile | $(BUILD_DIR)
	$(CC) $(CPP_FLAGS) $< -o $@

$(BUILD_DIR):
	mkdir $@

run:
	@$(BUILD_DIR)/$(TARGET) $(RUN_ARGS)

clean:
	rm -Rf $(BUILD_DIR)

switch:$(SAMPLE_DIR)/$(Name).cpp
	@echo "Copying $(Name).cpp to Main.cpp"
	@cat $(SAMPLE_DIR)/$(Name).cpp > $(SOURCE_DIR)/Main.cpp

sample:$(SOURCE_DIR)/Main.cpp
	@echo "Sampling to $(Name).cpp"
	@cat $(SOURCE_DIR)/Main.cpp > $(SAMPLE_DIR)/$(Name).cpp