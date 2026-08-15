BUILD_TYPE ?= Release
JOBS ?= 4

ifeq ($(OS),Windows_NT)
CMAKE ?= cmake
BUILD_DIR ?= build-msvc-win
else
CMAKE ?= msvc-x86-cmake
BUILD_DIR ?= build-msvc-linux
endif

.PHONY: all configure build test clean rebuild format

all: build

configure:
	$(CMAKE) -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DSR2AP_BUILD_TESTS=ON

build: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean

rebuild: clean build

format: configure
	$(CMAKE) --build $(BUILD_DIR) --target format-game
