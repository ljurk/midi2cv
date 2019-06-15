[![Build Status](https://travis-ci.org/ljurk/midi2cv.svg?branch=master)](https://travis-ci.org/ljurk/midi2cv)

midi2cv
=======
Arduino project to translate midi notes and cc to cv and gate for my mighty modular system

Dependencies
------------

    #Adafruit_MCP4725.h
    platformio lib install 677

    #MIDI.h
    platformio lib install 62

Raspberry as ISP
------
Install required software

    apt-get install arduino arduino-mk
    wget http://project-downloads.drogon.net/gertboard/avrdude_5.10-4_armhf.deb
    sudo dpkg -i avrdude_5.10-4_armhf.deb

Check connection

    avrdude -p m328p -c gpio

Install required libraries

    #Adafruit_MCP4725.h
    sudo git clone https://github.com/adafruit/Adafruit_MCP4725.git /usr/share/arduino/libraries/Adafruit_MCP4725.git

    #MIDI.h
    sudo git clone https://github.com/PaulStoffregen/MIDI.git /usr/share/arduino/libraries/MIDI

Build it

    make

upload

    avrdude -p m328p -c gpio -U flash:w:build-uno/atmega.hex -F
