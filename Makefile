CC = clang++-10
PPFLAGS = -Iinclude 
CFLAGS = -std=c++17
LDFLAGS = `llvm-config-10 --cxxflags --ldflags --libs`

BUILD_DIR := build
TARGET = ${BUILD_DIR}/kronkc

SRC_DIR := src
OBJ_DIR := obj

SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $(TARGET) 

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(PPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir $@

clean:
	rm -rf  $(OBJ) $(TARGET)