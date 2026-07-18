GENERATOR := "Visual Studio 18 2026"
ARCH := x64
BUILD_DIR := build
EXE := $(BUILD_DIR)/AudioMixer_artefacts/Debug/AudioMixer.exe

SHELL = cmd

.PHONY: all build release run test clean

all: build

build:
	cmake -S . -B $(BUILD_DIR) -G $(GENERATOR) -A $(ARCH)
	cmake --build $(BUILD_DIR) --config Debug --parallel

release:
	cmake -S . -B $(BUILD_DIR) -G $(GENERATOR) -A $(ARCH)
	cmake --build $(BUILD_DIR) --config Release

run: build
	$(EXE)

test:
	cmake --build $(BUILD_DIR) --config Debug --target AudioMixerTests --parallel
	cd $(BUILD_DIR) && ctest -C Debug --output-on-failure

clean:
	if exist $(BUILD_DIR) rd /s /q $(BUILD_DIR)
