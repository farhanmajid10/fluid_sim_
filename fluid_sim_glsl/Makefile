# Simple Makefile for LBM Fluid Simulation
# Alternative to CMake for quick builds

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3
LDFLAGS = -lGL -lglfw -ldl -lpthread -lX11

# Directories
SRC_DIR = src
SHADER_DIR = shaders
BUILD_DIR = build
FLGL_DIR = lib/flgl

# Include paths
INCLUDES = -I$(FLGL_DIR)/include \
           -I$(FLGL_DIR)/lib/glad/include \
           -I$(FLGL_DIR)/lib/glfw/include \
           -I$(FLGL_DIR)/lib/glm \
           -Iinclude

# Source files
SOURCES = $(SRC_DIR)/main.cpp
FLGL_SOURCES = $(FLGL_DIR)/src/flgl.cpp \
               $(FLGL_DIR)/src/window.cpp \
               $(FLGL_DIR)/src/shader.cpp \
               $(FLGL_DIR)/src/texture.cpp \
               $(FLGL_DIR)/lib/glad/src/glad.c

# Object files
OBJECTS = $(BUILD_DIR)/main.o \
          $(BUILD_DIR)/flgl.o \
          $(BUILD_DIR)/window.o \
          $(BUILD_DIR)/shader.o \
          $(BUILD_DIR)/texture.o \
          $(BUILD_DIR)/glad.o

# Target executable
TARGET = $(BUILD_DIR)/LBM_FluidSim

# Shader files
SHADERS = $(wildcard $(SHADER_DIR)/*.vert $(SHADER_DIR)/*.frag)
SHADER_COPIES = $(patsubst $(SHADER_DIR)/%, $(BUILD_DIR)/shaders/%, $(SHADERS))

# Default target
all: directories $(TARGET) copy_shaders

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/shaders

# Link the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile main.cpp
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile FLGL sources
$(BUILD_DIR)/flgl.o: $(FLGL_DIR)/src/flgl.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/window.o: $(FLGL_DIR)/src/window.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/shader.o: $(FLGL_DIR)/src/shader.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/texture.o: $(FLGL_DIR)/src/texture.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile GLAD (C file)
$(BUILD_DIR)/glad.o: $(FLGL_DIR)/lib/glad/src/glad.c
	gcc -c $< -o $@ -I$(FLGL_DIR)/lib/glad/include

# Copy shaders to build directory
copy_shaders: $(SHADER_COPIES)

$(BUILD_DIR)/shaders/%: $(SHADER_DIR)/%
	@cp $< $@

# Run the simulation
run: all
	cd $(BUILD_DIR) && ./LBM_FluidSim

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Rebuild everything
rebuild: clean all

.PHONY: all directories copy_shaders run clean rebuild

# Platform-specific adjustments
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    LDFLAGS = -lopengl32 -lglfw3 -lgdi32
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        LDFLAGS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lglfw
    endif
endif

