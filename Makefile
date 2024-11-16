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

.PHONY: all test clean build debug pack

all: build ./$(TARGET)

debug: DEBUG:=-DDEBUG

debug: clean build ./$(TARGET)

$(OBJ_DIR)/%.o: %.cpp $(INCLUDE_DIR)/*.h 
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(DEBUG) -c $< -o $@ $(SSLFLAGS)

./$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SSLFLAGS)

build:
	@mkdir -p $(OBJ_DIR)

test: all $(TESTS_DIR)/Makefile
	make -C $(TESTS_DIR)
	$(TESTS_DIR)/$(TESTS_TARGET)

pack: $(INCLUDE_DIR) manual.pdf README.md $(TESTS_DIR) Makefile
	tar -cvf xduric06.tar README.md manual.pdf Makefile src/ tests/ include/

clean: $(TESTS_DIR)/Makefile
	$(RM) $(TARGET)
	$(RM) $(TESTS_DIR)/tests
	$(RM) $(OBJ_DIR)
	$(RM) xduric06.tar
	make clean -C $(TESTS_DIR)