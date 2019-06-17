ARDUINO_DIR = /usr/share/arduino
BOARD_TAG = uno
ARDUINO_PORT = /dev/ttyAMA0
ARDUINO_LIBS =
OBJDIR=../build
#get name of the directory oft this makefile, becaus Arduino.mk will name the generated hex file like that
THISDIR=$(notdir $(shell pwd))
#include arduino make file if "make" is called
ifeq ($(MAKECMDGOALS),)
include /usr/share/arduino/Arduino.mk
endif

#include arduino make file if "make build" is called
ifeq ($(MAKECMDGOALS),build)
include /usr/share/arduino/Arduino.mk
endif

all: build upload

build:
	@echo "##########build ok##########"

upload:
	@echo "##########start uploading##########"
	avrdude -p m328p -c gpio -U flash:w:$(OBJDIR)/$(THISDIR).hex -F
	@echo "##########upload ok##########"
clean::
	rm -r build-uno
