# Compiler
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -I/usr/local/include
LDFLAGS = -L/usr/local/lib

# Directories
SRC_DIR = src
BUILD_DIR = build
PKG_DB_DIR = /var/lib/lxpkg/db
PKG_CACHE = /var/cache/lxpkg/pkgs

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c) src/toml_parser.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Target executable
TARGET = lxpkg

# Include directories
INCLUDES = -I$(SRC_DIR)

# Libraries
LIBS = -lcurl -ltoml

# Default target
all: $(TARGET)

# Link the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

# Compile the source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create necessary directories
directories:
	@mkdir -p $(PKG_DB_DIR)
	@mkdir -p $(PKG_CACHE)

# Clean up
clean:
	@rm -rf $(BUILD_DIR)/*.o
	@rm -f $(TARGET)

.PHONY: all clean directories

