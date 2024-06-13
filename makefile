# Set the build directory
BUILD_DIR = build

# Default target
all: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR)

# Generate the Makefile using CMake
$(BUILD_DIR)/Makefile: $(wildcard src/*) CMakeLists.txt
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..

# Clean the build directory
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
