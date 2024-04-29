.PHONY: all clean build release debug

MAKEFLAGS := --jobs=$(shell nproc)

all: clean build

clean:
	@ $(MAKE) -C build/ clean

build:
	@ $(MAKE) -C build/

release:
	@ rm -rf build/*
	@ cmake \
	--no-warn-unused-cli \
	-DCMAKE_BUILD_TYPE:STRING=Release \
	-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
	-DCMAKE_C_COMPILER:FILEPATH=/usr/bin/arm-none-eabi-gcc \
	-DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/arm-none-eabi-g++ \
	-S . \
	-B build

debug:
	@ rm -rf build/*
	@ cmake \
	--no-warn-unused-cli \
	-DCMAKE_BUILD_TYPE:STRING=Debug \
	-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
	-DCMAKE_C_COMPILER:FILEPATH=/usr/bin/arm-none-eabi-gcc \
	-DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/arm-none-eabi-g++ \
	-S . \
	-B build