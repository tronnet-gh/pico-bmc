#TOPTARGETS := all clean

SUBDIRS := build

all: makefsdata $(SUBDIRS)

makefsdata:
	python3 makefsdata.py

clean: $(SUBDIRS)
	rm -rf htmldata.c

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: all makefsdata clean $(SUBDIRS)