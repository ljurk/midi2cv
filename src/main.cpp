/*  https://www.instructables.com/id/Another-MIDI-to-CV-Box-/
 *  Another MIDI2CV Box
 *  MIDI interface for Hz/V and V/oct synths. Connect your favourite keyboard or DAW to the box via MIDI;
 *  the box will in turn control your synth gate On/Off (straight +5V/0V or S-Trig, reversed,
 *  0V/+5V), pitch (variable voltage via DAC output), velocity (0V to +5V, RC filtered) and a MIDI Control
 *  Change variable at your will (0V to +5V, RC filtered).
 *
 *  Connections:
 * (1) MIDI connector: see general online reference
 * (2) DAC MCP4725:
 * SDA pin to A4/SDA (Arduino UNO adn nano)
 * SCL pin to A5/SCL (Arduino UNO adn nano)
 * GND to GND
 * VCC to +5V
 * (3) Outputs:
 * Arduino gate OUT pin (pin 12 by default) to synth gate/trigger IN via 1K Ohm resistor
 * Arduino velocity OUT pin (pin 10 by default) to synth VCA IN via RC filter (1K Ohm, 100uF)
 * Arduino MIDI CC OUT pin (pin 9 by default) to synth VCF IN via RC filter (1K Ohm, 100uF)
 * DAC OUT to synth VCO IN
 *
 * MIDI messages table:
 *    Message                      Status    Data 1               Data 2
 *    Note Off                     8n        Note Number          Velocity
 *    Note On                      9n        Note Number          Velocity
 *    Polyphonic Aftertouch        An        Note Number          Pressure
 *    Control Change               Bn        Controller Number    Data
 *    Program Change               Cn        Program Number       Unused
 *    Channel Aftertouch           Dn        Pressure             Unused
 *    Pitch Wheel                  En        LSB                  MSB
 *
 * Key
 * n is the MIDI Channel Number (0-F)
 * LSB is the Least Significant Byte
 * MSB is the Least Significant Byte
 * There are several different types of controller messages.
 *
 * useful links, random order:
 *  https://en.wikipedia.org/wiki/CV/gate
 *  https://www.instructables.com/id/Send-and-Receive-MIDI-with-Arduino/
 *  http://www.songstuff.com/recording/article/midi_message_format
 *  https://espace-lab.org/activites/projets/en-arduino-processing-midi-data/
 *  https://learn.sparkfun.com/tutorials/midi-shield-hookup-guide/example-2-midi-to-control-voltage
 *  https://provideyourown.com/2011/analogwrite-convert-pwm-to-voltage/
 *  https://www.midi.org/specifications/item/table-3-control-change-messages-data-bytes-2
 *  https://arduino-info.wikispaces.com/Arduino-PWM-Frequency
 *
 *  by Barito, 2017 - 2018
 */
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_MCP4725.h>
#include <MIDI.h>
#include <Encoder.h>

bool debug = true;

#define NUMBER_OF_CC 4
// OLED display TWI address
#define OLED_ADDR   0x3C

Adafruit_SSD1306 display(-1);

//setup encoder
Encoder myEnc = Encoder(3,2);
byte encoderButtonPin = 4;
bool encoderButtonState = false;
bool encoderButtonPressed = false;
int oldEncoderPos = 0;
int newEncoderPos = 0;

//set at your will ...
#define REVERSE_GATE 0 //V-Trig = 0, S-Trig = 1
bool HZV = 1; //set to "0" for V/oct

byte ccMax = 255;
byte midiChannel = 1;

Adafruit_MCP4725 dac;

//true = menu, false = editing
bool mode = true;
//0 = channel, 1 = cc1, 2 = cc2
int editMenu = 0;

//digital
byte gatePin = 10;
//analog
byte velocityPin = 5; //pwm frequency is going to be increased for this in the setup

float outVoltmV;
int velocityOut;
int CCOut;

unsigned long lastUpdateTime = 0;  // the last time the output pin was toggled
unsigned long updateDelay = 200;    // the debounce time; increase if the output flickers

uint16_t dacValue;
//CHANGE IF BY PRESSING "C" ON YOUR KEYBOARD YOU HAVE ANOTHER NOTE OUTPUTTED BY THE SYNTH (HZ/V).
int noteHZVshift = -1;//With external USB hub I had to set this to "-1". With PC USB to "+2".
//CHANGE IF YOU HAVE A DETUNING BETWEEN ADIACENT NOTES/OCTAVES (V/oct).
float VoctLinCoeff = 0.0790;//If your +5V are straight +5.000V this is 1/12 = 0.0833.
//CHANGE TO SHIFT BY OCTAVES (V/oct).
float VoctShift = -2.0;
byte lastNote;
MIDI_CREATE_DEFAULT_INSTANCE();

String midiNumber[17] = {"all", "I", "II", "III", "IV", "V", "VI", "VII", "VII", "IX", "X", "XI", "XII", "XIII", "XIV", "XV", "XVI"};

struct ccInfo
{
    byte pin;
    byte midiCC;
    byte value;
    byte displayPos;
    byte valuePos;
};

ccInfo avaibleCC[NUMBER_OF_CC];

void fillStruct(int index, byte pin, byte midiCC, byte value, byte displayPos, byte valuePos)
{
    avaibleCC[index].pin = pin;
    avaibleCC[index].midiCC = midiCC;
    avaibleCC[index].value = value;
    avaibleCC[index].displayPos = displayPos;
    avaibleCC[index].valuePos = valuePos;
}

void startingAnimation(int waitTime)
{
    for(int i = 0; i < 5; i++) {
        display.drawRect(i *2, i*2, 128-2 * (i*2),32- 2 * (i*2),WHITE);
    }
    display.drawCircle(16, 16, 5, WHITE);
    display.drawCircle(128-16, 16, 5, WHITE);
    display.setRotation(1);
    display.setCursor(14, 25);
    display.print("M");
    display.setCursor(14, 35);
    display.print("I");
    display.setCursor(14, 45);
    display.print("D");
    display.setCursor(14, 55);
    display.print("I");
    display.setCursor(14, 70);
    display.print("2");
    display.setCursor(14, 85);
    display.print("C");
    display.setCursor(14, 95);
    display.print("V");
    display.display();
    delay(waitTime);
}

void updateDisplay(byte activeCC = 0, byte activeValue = 0)
{
    //only every second
    if ((millis() - lastUpdateTime) > updateDelay) {
        lastUpdateTime = millis();

        display.clearDisplay();

        display.setCursor(0,0);
        char symbol = ':';
        if(mode == false && editMenu == 0) {
            symbol = '>';
        } else if(editMenu == 0) {
            symbol = '<';
        }
        String out = "c";
        out += symbol;
        out += String(midiChannel);
        display.print(out);

        display.drawLine(0,10,64,10,WHITE);

        for(byte i = 0; i < NUMBER_OF_CC; i++) {
            display.setCursor(0,avaibleCC[i].displayPos);
            symbol = ':';
            if(mode == false && editMenu == i + 1) {
                symbol = '>';
            }else if(editMenu == i + 1) {
                symbol = '<';
            }
            out = "C" + String(i+1) + symbol + String(avaibleCC[i].midiCC);
            display.print(out);
        }

        byte barWidth = 0;
        for(byte i = 0; i < NUMBER_OF_CC; i++) {
            barWidth = (avaibleCC[i].value * 32) / 255;
            display.fillRect(0, avaibleCC[i].valuePos, barWidth, 5, WHITE);
        }
        display.display();
    }
}

void editValue(int range)
{
    if(editMenu == 0) {
        midiChannel += range;
        if(midiChannel == 17) {
            midiChannel = 0;
        } else if(midiChannel == -1) {
                midiChannel = 16;
        }
    } else {
        for(byte i = 0; i < NUMBER_OF_CC; i++) {
            if(i == editMenu -1) {
                avaibleCC[i].value += range;
                if(avaibleCC[i].value >= ccMax) {
                    avaibleCC[i].value = 0;
                } else if(avaibleCC[i].value <= -1) {
                    avaibleCC[i].value = 255;
                }
            }
        }
    }
}

void handleNoteOn(byte channel, byte note, byte velocity)
{
    lastNote = note;
    //Hz/V; x 1000 because map truncates decimals
    if (HZV) {
        //0.125*1000
        //V/oct; x 1000 because map truncates decimals
        outVoltmV = 125.0*exp(0.0578*(note+noteHZVshift));
    } else {
        outVoltmV = 1000*((note*VoctLinCoeff)+ VoctShift);
    }
    dacValue = constrain(map(outVoltmV, 0, 5000, 0, 4095), 0, 4095);
    dac.setVoltage(dacValue, false);
    if(REVERSE_GATE == 1) {
        digitalWrite(gatePin, LOW);
    } else {
        digitalWrite(gatePin, HIGH);
    }
    velocityOut = map(velocity, 0, 127, 0, 255);
    analogWrite(velocityPin, velocityOut);
}

void handleNoteOff(byte channel, byte note, byte velocity)
{
    if(note == lastNote) {
        dac.setVoltage(0, false);
        if(REVERSE_GATE == 1) {
            digitalWrite(gatePin, HIGH);
        } else {
            digitalWrite(gatePin, LOW);
        }
        analogWrite(velocityPin, 0);
    }
}

void handleControlChange(byte channel, byte number, byte value)
{
    for(byte i = 0; i < NUMBER_OF_CC; i++) {
        if(number == avaibleCC[i].midiCC) {
            avaibleCC[i].value = value;
            CCOut = map(value, 0, 127, 0, 255);
            analogWrite(avaibleCC[i].pin, CCOut);
        }
    }
}

void setup()
{
    if(debug) {
        Serial.print(MIDI_CHANNEL_OMNI);
    }
    lastUpdateTime = millis();
    //For Arduino Uno, Nano, and any other board using ATmega 8, 168 or 328
    //TCCR0B = TCCR0B & B11111000 | B00000001;
    //// D5, D6: set timer 0 divisor to 1 for PWM frequency of 62500.00 Hz
    TCCR1B = (TCCR1B & B11111000) | B00000001;    // D9, D10: set timer 1 divisor to 1 for PWM frequency of 31372.55 Hz
    //TCCR2B = TCCR2B & B11111000 | B00000001;
    //// D3, D11: set timer 2 divisor to 1 for PWM frequency of 31372.55 Hz
    fillStruct(0,  6, 20, 23, 15, 25);
    fillStruct(1,  9, 21, 33, 40, 50);
    fillStruct(2, 11, 22, 77, 65, 75);
    fillStruct(3,  12, 23, 99, 90, 100);

    //setup pins
    pinMode(gatePin, OUTPUT);
    pinMode(velocityPin, OUTPUT);
    for(byte i = 0; i < NUMBER_OF_CC; i++) {
        pinMode(avaibleCC[i].pin, OUTPUT);
    }

    if(REVERSE_GATE == 1){
        //S-Trig
        digitalWrite(gatePin, HIGH);
    } else {
        //V-Trig
        digitalWrite(gatePin, LOW);
    }

    ////setup MIDI handles
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
    MIDI.setHandleControlChange(handleControlChange);

    //// start MIDI and listen to channel "midiChannel"
    if(debug) {
        Serial.begin(9600);
    } else {
        MIDI.begin(midiChannel);
    }
    //// For Adafruit MCP4725A1 the address is 0x62 (default) or 0x63 (ADDR pin tied to VCC)
    //// For MCP4725A0 the address is 0x60 or 0x61
    //// For MCP4725A2 the address is 0x64 or 0x65
    dac.begin(0x62);

    // initialize and clear display
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(1);

    startingAnimation(5000);

    display.setRotation(1);
}

void loop()
{
    newEncoderPos = myEnc.read();
    encoderButtonState = digitalRead(encoderButtonPin);
    //encoder
    if(newEncoderPos != oldEncoderPos) {
        if(newEncoderPos > oldEncoderPos + 2) {
            if(debug) {
                Serial.println("turn right");
            }
            if(mode) {//menu
                if(editMenu != NUMBER_OF_CC) {
                    editMenu++;
                } else {
                    editMenu = 0;
                }
            } else {
                if(encoderButtonState == HIGH) {
                    editValue(20);
                }else{
                    editValue(1);
                }
            }
            oldEncoderPos = newEncoderPos;
        } else if(newEncoderPos < oldEncoderPos - 2) {
            //turn left
            if(mode) {//menu
                if(editMenu != 0) {
                    editMenu--;
                } else {
                    editMenu = NUMBER_OF_CC;
                }
            } else {
                if(encoderButtonState == HIGH) {
                    editValue(-20);
                }else{
                    editValue(-1);
                }
            }
            if(debug) {
                Serial.println("turn left");
            }
            oldEncoderPos = newEncoderPos;
        }
    }
    if(encoderButtonState == HIGH && encoderButtonPressed == false) {
        mode = !mode;
        if(debug) {
            Serial.println("Buttonpress");
            Serial.println(mode);
        }
        encoderButtonPressed = true;
    } else if(encoderButtonState == LOW) {
        encoderButtonPressed = false;
    }
    MIDI.read(midiChannel);
    updateDisplay();
}
