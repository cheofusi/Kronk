CC = g++
PPFLAGS = -Iinclude 
CFLAGS = -std=c++17
LDFLAGS = `llvm-config-9 --cxxflags --ldflags --system-libs --libs core`

BUILD_DIR := build
TARGET = ${BUILD_DIR}/dabble

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
	rm -f  $(OBJFILES) $(TARGET)