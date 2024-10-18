CXX				:= g++
RM				:= rm -rf
CXXFLAGS		:= -std=c++20 -Werror -Wall -Wpedantic 
SSLFLAGS		:= -lssl -lcrypto
TARGET			:= imapcl
TESTS_TARGET 	:= tests
BUILD			:= ./build
OBJ_DIR			:= $(BUILD)/objects
INCLUDE_DIR		:= ./include
TESTS_DIR		:= ./tests
SRC_FILES		:= $(wildcard src/*.cpp)			
OBJECTS 		:= $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

.PHONY: all test clean build debug

all: build ./$(TARGET)

$(OBJ_DIR)/%.o: %.cpp $(INCLUDE_DIR)/*.h 
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(SSLFLAGS)

./$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SSLFLAGS)

build:
	@mkdir -p $(OBJ_DIR)

test: all $(TESTS_DIR)/Makefile
	make -C $(TESTS_DIR)
	$(TESTS_DIR)/$(TESTS_TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) $(TESTS_DIR)/tests
	$(RM) $(OBJ_DIR)
