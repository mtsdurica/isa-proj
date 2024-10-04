CXX				:= g++
RM				:= rm -rf
CXXFLAGS		:= -std=c++20 -Werror -Wall -Wpedantic
TEST_FLAGS		:= -lgtest -lgtest_main -pthread
TARGET			:= imapcl
TESTS_TARGET 	:= tests
BUILD			:= ./build
OBJ_DIR			:= $(BUILD)/objects
INCLUDE_DIR		:= ./include
TESTS_DIR		:= ./tests
SRC_FILES		:= $(wildcard src/*.cpp)			
OBJECTS 		:= $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

.PHONY: all test clean build

all: build ./$(TARGET)

$(OBJ_DIR)/%.o: %.cpp $(INCLUDE_DIR)/*.h 
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

./$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^

build:
	@mkdir -p $(OBJ_DIR)

test: all $(TESTS_DIR)/Makefile
	make -C $(TESTS_DIR)
	$(TESTS_DIR)/$(TESTS_TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) $(TESTS_DIR)/tests
	$(RM) $(OBJ_DIR)
