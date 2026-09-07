BUILD_TYPE ?= Release
JOBS ?= 4
PROJECT_VERSION ?= dev
BUILD_DIR ?= build

ifeq ($(OS),Windows_NT)
CMAKE ?= cmake
else
CMAKE ?= msvc-x86-cmake
endif

.PHONY: all configure build test clean rebuild format

all: build

configure:
	$(CMAKE) -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DSR2AP_BUILD_TESTS=ON -DSR2AP_PROJECT_VERSION=${PROJECT_VERSION}

build: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean

rebuild: clean build

format: configure
	$(CMAKE) --build $(BUILD_DIR) --target format-game
