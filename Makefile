.PHONY: all clean build makefsdata

all: clean makefsdata build

clean:
	$(MAKE) -C build/ clean
	rm -rf wwwdata.c

makefsdata:
	python3 makefsdata.py

build:
	$(MAKE) -C build/