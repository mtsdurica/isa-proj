CXX 			:= g++
RM				:= rm -rf
CXXFLAGS		:= -std=c++20 -Werror -Wall -Wpedantic
TEST_FLAGS		:= -lgtest -lgtest_main -pthread
TARGET			:= tests 
BUILD			:= ./build
OBJ_DIR			:= $(BUILD)/objects
INCLUDE_DIR		:= ../include
SRC_FILES		:= $(wildcard src/*.cpp)			
OBJECTS 		:= $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

.PHONY: all test clean build

all: build ./$(TARGET)

$(OBJ_DIR)/%.o: %.cpp $(INCLUDE_DIR)/*.h
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) -c $< -o $@

./$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) -o $@ $^

build:
	@mkdir -p $(OBJ_DIR)

clean:
	$(RM) $(TARGET)
	$(RM) $(OBJ_DIR)
