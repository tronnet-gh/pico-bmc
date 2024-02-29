.PHONY: all clean build makefsdata

all: clean build

clean:
	$(MAKE) -C build/ clean

build:
	$(MAKE) -C build/